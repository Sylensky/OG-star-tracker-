
#include <Arduino.h>

#include "intervalometer_mode.h"
#include "uart.h"

#include "axis.h"
#include "configs/config.h"
#include "hardwaretimer.h"

IntervalometerMode::IntervalometerMode(uint8_t triggerPin, const Settings& settings)
    : triggerPin(triggerPin), settings(settings), currentState(State::Inactive),
      errorMessage(ERR_MSG_NONE), active(false), abortRequested(false), exposuresTaken(0),
      currentExposure(0), previousDitherDirection(0), startCaptureTickCount(0),
      captureDurationTickCount(0), taskHandle(nullptr)
{
    pinMode(triggerPin, OUTPUT);
    digitalWrite(triggerPin, LOW);
}

IntervalometerMode::~IntervalometerMode()
{
    if (active)
    {
        abortCapture();
        // Wait a bit for task to clean up
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool IntervalometerMode::startCapture()
{
    if (active)
    {
        return false;
    }

    active = true;
    abortRequested = false;
    currentState = State::PreDelay;
    exposuresTaken = 0;
    currentExposure = 0;
    startCaptureTickCount = xTaskGetTickCount();

    uint32_t durationSeconds = calculateTotalDuration();
    captureDurationTickCount = pdMS_TO_TICKS(durationSeconds * 1000);

    // Create FreeRTOS task for this mode
    BaseType_t result = xTaskCreatePinnedToCore(taskWrapper,   // Task function
                                                getModeName(), // Task name
                                                4096,          // Stack size
                                                this,          // Parameter passed to task
                                                1,             // Priority
                                                &taskHandle,   // Task handle
                                                1 // Core ID (run on core 1 with other tasks)
    );

    if (result != pdPASS)
    {
        active = false;
        errorMessage = ERR_MSG_NONE; // Could add a specific error for task creation
        print_out("ERROR: Failed to create task for %s", getModeName());
        return false;
    }

    print_out("Started %s - task created successfully", getModeName());
    return true;
}

void IntervalometerMode::abortCapture()
{
    if (active)
    {
        print_out("Abort requested for %s", getModeName());
        abortRequested = true;
    }
}

void IntervalometerMode::taskWrapper(void* pvParameters)
{
    IntervalometerMode* instance = static_cast<IntervalometerMode*>(pvParameters);

    // Execute the mode-specific loop
    instance->executeLoop();

    // Cleanup and delete task
    instance->cleanup();

    // Delete the task - this frees the heap
    vTaskDelete(nullptr);
}

void IntervalometerMode::cleanup()
{
    print_out("Cleaning up %s", getModeName());

    // Ensure trigger is off
    digitalWrite(triggerPin, LOW);

    // Stop any axis movement
    ra_axis.stopSlew();

    if (ra_axis.slewActive || ra_axis.goToTarget)
    {
        ra_axis.counterActive = false;
        ra_axis.goToTarget = false;
    }

    // Mark as inactive
    active = false;
    currentState = State::Complete;

    print_out("%s complete - %d exposures taken", getModeName(), exposuresTaken);
}

uint32_t IntervalometerMode::calculateTotalDuration() const
{
    uint32_t exposures = settings.exposures ? settings.exposures : 1;
    uint32_t totalExposureTime = exposures * settings.exposureTime;
    uint32_t totalDelays = (exposures > 1) ? ((exposures - 1) * settings.delayTime) : 0;
    uint32_t total = settings.preDelay + totalExposureTime + totalDelays;

    return (total == 0) ? 1 : total;
}

void IntervalometerMode::performPreDelay()
{
    if (settings.preDelay > 0)
    {
        currentState = State::PreDelay;
        print_out("%s: Pre-delay start (%d s)", getModeName(), settings.preDelay);

        if (!waitWithAbortCheck(settings.preDelay * 1000))
        {
            return; // Aborted
        }

        print_out("%s: Pre-delay complete", getModeName());
    }
}

void IntervalometerMode::triggerOn()
{
    digitalWrite(triggerPin, HIGH);
    print_out("Trigger ON");
}

void IntervalometerMode::triggerOff()
{
    digitalWrite(triggerPin, LOW);
    print_out("Trigger OFF");
}

bool IntervalometerMode::waitWithAbortCheck(uint32_t ms)
{
    const uint32_t checkInterval = 100; // Check abort flag every 100ms
    uint32_t elapsed = 0;

    while (elapsed < ms)
    {
        if (abortRequested)
        {
            print_out("Wait aborted");
            return false;
        }

        uint32_t remaining = ms - elapsed;
        uint32_t waitTime = (remaining < checkInterval) ? remaining : checkInterval;

        vTaskDelay(pdMS_TO_TICKS(waitTime));
        elapsed += waitTime;
    }

    return true;
}

bool IntervalometerMode::performDither()
{
    if (!settings.dither)
    {
        return true;
    }

    if (exposuresTaken % settings.ditherFrequency != 0)
    {
        return true; // Not time to dither yet
    }

    currentState = State::Dither;
    print_out("%s: Dither start", getModeName());

    // Ensure counter is active for position tracking
    if (!ra_axis.counterActive)
    {
        ra_axis.resetAxisCount();
        ra_axis.counterActive = true;
    }

    // Calculate random dither direction and distance
    uint8_t randomDirection = biasedRandomDirection(previousDitherDirection);
    previousDitherDirection = randomDirection;

    int16_t stepsToDither =
        ((random(100 * DITHER_DISTANCE_X10_PIXELS) + 1) / 100.0) * getStepsPerTenPixels();
    stepsToDither = randomDirection ? stepsToDither : stepsToDither * -1;

    // Set target and start slew
    ra_axis.setAxisTargetCount(stepsToDither + ra_axis.axisCountValue);

    if (ra_axis.targetCount != ra_axis.axisCountValue)
    {
        ra_axis.goToTarget = true;
        ra_axis.startSlew(ra_axis.rate.tracking / 6, randomDirection);

        // Wait for slew to complete
        while (ra_axis.slewActive && !abortRequested)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    print_out("%s: Dither complete", getModeName());

    return !abortRequested;
}

uint16_t IntervalometerMode::getStepsPerTenPixels() const
{
    return (int) ((getArcsecPerPixel() * 10.0) / ARCSEC_PER_STEP + 0.5);
}

float IntervalometerMode::getArcsecPerPixel() const
{
    return ((float) settings.pixelSize / settings.focalLength) * 206.265;
}

uint8_t IntervalometerMode::biasedRandomDirection(uint8_t previousDirection)
{
    // Bias against repeating the same direction (55/45 split)
    uint8_t directionLeftBias = previousDirection == 0 ? 45 : 55;
    uint8_t randVal = random(100);
    return randVal < directionLeftBias ? 0 : 1;
}
