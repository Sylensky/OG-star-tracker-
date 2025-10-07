#include "timelapse.h"
#include "uart.h"

Timelapse::Timelapse(uint8_t triggerPin, const Settings& settings)
    : IntervalometerMode(triggerPin, settings)
{
}

void Timelapse::executeLoop()
{
    print_out("=== %s Mode Started ===", getModeName());
    print_out("Settings: %d exposures, delay: %ds", settings.exposures, settings.delayTime);

    // Stop tracking if active (timelapse doesn't use tracking)
    if (ra_axis.trackingActive)
    {
        print_out("Stopping tracking for timelapse mode");
        ra_axis.stopTracking();
    }

    // Perform pre-delay
    performPreDelay();
    if (abortRequested)
        return;

    // Main capture loop
    while (exposuresTaken < settings.exposures && !abortRequested)
    {
        // === CAPTURE STATE ===
        currentState = State::Capture;
        print_out("Capture %d/%d start", exposuresTaken + 1, settings.exposures);

        // Trigger camera with short pulse (camera controls exposure time)
        triggerOn();

        // Short delay to allow camera to register trigger (1 second should be enough)
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Release trigger immediately (camera continues exposure)
        triggerOff();

        currentExposure++;
        exposuresTaken++;
        print_out("Capture %d/%d triggered", exposuresTaken, settings.exposures);

        // === DITHER STATE (if enabled) ===
        if (settings.dither && exposuresTaken < settings.exposures)
        {
            if (!performDither())
            {
                return; // Aborted during dither
            }
        }

        // === DELAY STATE ===
        if (exposuresTaken < settings.exposures)
        {
            currentState = State::Delay;
            print_out("Delay start (%ds)", settings.delayTime);

            if (!waitWithAbortCheck(settings.delayTime * 1000))
            {
                return;
            }

            print_out("Delay complete");
        }
    }

    print_out("=== %s Mode Complete: %d exposures ===", getModeName(), exposuresTaken);
}
