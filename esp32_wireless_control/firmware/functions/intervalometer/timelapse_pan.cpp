#include "timelapse_pan.h"
#include "tracking_rates.h"
#include "uart.h"

TimelapsePan::TimelapsePan(uint8_t triggerPin, const Settings& settings)
    : IntervalometerMode(triggerPin, settings)
{
}

void TimelapsePan::executeLoop()
{
    print_out("=== %s Mode Started ===", getModeName());
    print_out("Settings: %d exposures, pan angle: %.2f degrees, direction: %s", settings.exposures,
              settings.panAngle, settings.panDirection ? "forward" : "reverse");

    // Stop tracking if active (pan mode doesn't use normal tracking)
    if (ra_axis.trackingActive)
    {
        print_out("Stopping tracking for timelapse pan mode");
        ra_axis.stopTracking();
    }

    // Perform pre-delay
    performPreDelay();
    if (abortRequested)
        return;

    // Start continuous pan if angle specified
    if (settings.panAngle > 0.0f)
    {
        if (!startContinuousPan())
        {
            return; // Failed to start pan or aborted
        }
    }

    // Main capture loop
    while (exposuresTaken < settings.exposures && !abortRequested)
    {
        // === CAPTURE STATE ===
        currentState = State::Capture;
        print_out("Capture %d/%d start (pan position: %lld)", exposuresTaken + 1,
                  settings.exposures, ra_axis.getAxisCount());

        // Pause pan during exposure
        if (settings.panAngle > 0.0f && ra_axis.goToTarget)
        {
            pausePan();
        }

        // Trigger camera with short pulse
        triggerOn();
        vTaskDelay(pdMS_TO_TICKS(1000));
        triggerOff();

        currentExposure++;
        exposuresTaken++;
        print_out("Capture %d/%d complete", exposuresTaken, settings.exposures);

        // Resume pan with recalculated speed (if not last exposure)
        if (exposuresTaken < settings.exposures && settings.panAngle > 0.0f)
        {
            if (!resumePan())
            {
                return; // Aborted during resume
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

    // Stop pan when complete
    if (ra_axis.slewActive)
    {
        ra_axis.stopSlew();
    }

    // Disable counter
    if (ra_axis.counterActive)
    {
        ra_axis.counterActive = false;
    }

    print_out("=== %s Mode Complete: %d exposures, final position: %lld ===", getModeName(),
              exposuresTaken, ra_axis.getAxisCount());
}

bool TimelapsePan::startContinuousPan()
{
    print_out("Starting continuous pan");

    // Initialize position tracking
    if (!ra_axis.counterActive)
    {
        print_out("Initializing position tracking to 0");
        ra_axis.resetAxisCount();
        ra_axis.counterActive = true;
    }

    int64_t currentPosition = ra_axis.getAxisCount();
    print_out("Current position: %lld steps", currentPosition);

    // Calculate total angle in arcseconds
    uint64_t arcSecsToMove = uint64_t(3600.0 * settings.panAngle);
    print_out("Total angle: %llu arcseconds (%.2f degrees)", arcSecsToMove, settings.panAngle);

    // Calculate total duration for the entire capture sequence
    uint32_t totalDuration = calculateTotalDuration();
    if (totalDuration == 0)
        totalDuration = 1;
    print_out("Total duration: %u seconds", totalDuration);

    // Calculate arcseconds per second needed
    float arcsecsPerSecond = (float) arcSecsToMove / (float) totalDuration;
    print_out("Required rate: %.2f arcsec/sec", arcsecsPerSecond);

    // Calculate the slew rate needed
    // gotoTarget will calculate steps internally based on the Position delta
    // We just need to determine the microstepping and speed multiplier

    // Determine optimal microstepping mode based on speed requirement
    // startSlew uses TRACKER_MOTOR_MICROSTEPPING/2 for slew microstepping
    uint8_t slewMicrosteps = TRACKER_MOTOR_MICROSTEPPING / 2;

    // Calculate full steps per second needed
    float fullStepsPerSec = arcsecsPerSecond / ARCSEC_PER_STEP;
    // Calculate microstep pulses per second
    float microstepPulsesPerSec = fullStepsPerSec * slewMicrosteps;

    print_out("Required: %.2f full-steps/sec, %.2f microstep-pulses/sec", fullStepsPerSec,
              microstepPulsesPerSec);

    // Calculate speed multiplier
    // slew_rate = (2 * tracking_rate) / speed_multiplier
    // We want: timer_reload = TIMER_APB_CLK_FREQ / (2 * microstepPulsesPerSec)
    // And: timer_reload = tracking_rate / speed_multiplier
    // Therefore: speed_multiplier = (2 * microstepPulsesPerSec * tracking_rate) /
    // TIMER_APB_CLK_FREQ

    uint64_t numerator = (uint64_t) (2.0 * microstepPulsesPerSec * ra_axis.rate.tracking);
    uint64_t speedMultiplier = (numerator + TIMER_APB_CLK_FREQ - 1) / TIMER_APB_CLK_FREQ;

    // Clamp to valid range
    if (speedMultiplier < MIN_CUSTOM_SLEW_RATE)
    {
        speedMultiplier = MIN_CUSTOM_SLEW_RATE;
        print_out("Speed too fast, clamped to min multiplier: %d", MIN_CUSTOM_SLEW_RATE);
        print_out("WARNING: Pan will be slower than requested");
    }
    if (speedMultiplier > MAX_CUSTOM_SLEW_RATE)
    {
        speedMultiplier = MAX_CUSTOM_SLEW_RATE;
        print_out("Speed too slow, clamped to max multiplier: %d", MAX_CUSTOM_SLEW_RATE);
        print_out("WARNING: Pan will be faster than requested");
    }

    uint64_t slewRate = ra_axis.rate.tracking / speedMultiplier;
    print_out("Speed multiplier: %llu, Slew timer reload: %llu", speedMultiplier, slewRate);

    // Calculate actual speed achieved
    float actualPulsesPerSec = (float) TIMER_APB_CLK_FREQ / (2.0 * slewRate);
    float actualArcsecsPerSec = (actualPulsesPerSec / slewMicrosteps) * ARCSEC_PER_STEP;
    print_out("Actual speed: %.2f arcsec/sec (%.2f%% of requested)", actualArcsecsPerSec,
              (actualArcsecsPerSec / arcsecsPerSecond) * 100.0);

    // Convert to Position objects for gotoTarget
    int64_t stepsPerSecondAxis = trackingRates.getStepsPerSecondSolar();
    int64_t currentArcSeconds = ra_axis.getPosition() / stepsPerSecondAxis;
    Position currentPos((int) (currentArcSeconds / 3600), (int) ((currentArcSeconds % 3600) / 60),
                        (float) (currentArcSeconds % 60));

    int64_t targetArcSeconds = settings.panDirection ? currentArcSeconds + (int64_t) arcSecsToMove
                                                     : currentArcSeconds - (int64_t) arcSecsToMove;
    Position targetPos((int) (targetArcSeconds / 3600), (int) ((targetArcSeconds % 3600) / 60),
                       (float) (targetArcSeconds % 60));

    print_out("Position: current=%lld arcsec, target=%lld arcsec", currentArcSeconds,
              targetArcSeconds);

    // Start goto - temporarily boost tracking rate for calculation
    uint64_t savedTracking = ra_axis.rate.tracking;
    ra_axis.rate.tracking = savedTracking * MAX_CUSTOM_SLEW_RATE;
    ra_axis.gotoTarget(slewMicrosteps, slewRate, currentPos, targetPos);
    ra_axis.rate.tracking = savedTracking;

    print_out("Pan started successfully");

    return !abortRequested;
}

void TimelapsePan::pausePan()
{
    print_out("Pausing pan for capture (position: %lld)", ra_axis.getAxisCount());
    ra_axis.stopSlew();
}

bool TimelapsePan::resumePan()
{
    print_out("Resuming pan after capture");

    // Get remaining distance in axis count units (microstep pulses)
    int64_t remainingAxisSteps = ra_axis.getAxisTargetCount() - ra_axis.getAxisCount();

    if (remainingAxisSteps == 0)
    {
        print_out("Pan already complete");
        return true;
    }

    // Calculate elapsed time since start
    TickType_t elapsedTicks = xTaskGetTickCount() - startCaptureTickCount;
    uint32_t elapsedSeconds = pdTICKS_TO_MS(elapsedTicks) / 1000;

    // Calculate remaining time
    uint32_t totalDuration = calculateTotalDuration();
    uint32_t remainingSeconds =
        totalDuration > elapsedSeconds ? (totalDuration - elapsedSeconds) : 1;

    print_out("Remaining: %lld axis-steps, %u seconds", remainingAxisSteps, remainingSeconds);

    // Calculate required microstep pulses per second
    uint64_t absRemSteps = remainingAxisSteps < 0 ? -remainingAxisSteps : remainingAxisSteps;
    float microstepPulsesPerSec = (float) absRemSteps / (float) remainingSeconds;

    print_out("Required: %.2f microstep-pulses/sec", microstepPulsesPerSec);

    // Calculate speed multiplier
    uint64_t numerator = (uint64_t) (2.0 * microstepPulsesPerSec * ra_axis.rate.tracking);
    uint64_t speedMultiplier = (numerator + TIMER_APB_CLK_FREQ - 1) / TIMER_APB_CLK_FREQ;

    // Clamp to valid range
    if (speedMultiplier < MIN_CUSTOM_SLEW_RATE)
    {
        speedMultiplier = MIN_CUSTOM_SLEW_RATE;
        print_out("Speed clamped to min: %d", MIN_CUSTOM_SLEW_RATE);
    }
    if (speedMultiplier > MAX_CUSTOM_SLEW_RATE)
    {
        speedMultiplier = MAX_CUSTOM_SLEW_RATE;
        print_out("Speed clamped to max: %d", MAX_CUSTOM_SLEW_RATE);
    }

    uint64_t newSlewRate = ra_axis.rate.tracking / speedMultiplier;

    // Calculate and log actual achieved speed
    float actualPulsesPerSec = (float) TIMER_APB_CLK_FREQ / (2.0 * newSlewRate);
    print_out("Resuming: slewRate=%llu, multiplier=%llu, actual %.2f pulses/sec", newSlewRate,
              speedMultiplier, actualPulsesPerSec);

    ra_axis.resumeGoto(newSlewRate);

    return !abortRequested;
}
