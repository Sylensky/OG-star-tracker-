#include "timelapse_pan.h"
#include "uart.h"

TimelapsePan::TimelapsePan(uint8_t triggerPin, const Settings& settings)
    : IntervalometerMode(triggerPin, settings)
{
}

void TimelapsePan::executeLoop()
{
    print_out("=== %s Mode Started ===", getModeName());
    print_out("Settings: %d exposures, pan angle: %.2f degrees, direction: %s, continuous: %s",
              settings.exposures, settings.panAngle, settings.panDirection ? "forward" : "reverse",
              settings.continuousPan ? "yes" : "no");

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

    // Calculate pan angle (apply direction) and the total duration time
    float totalPanAngle = settings.panDirection ? settings.panAngle : -settings.panAngle;
    int totalDurationTime = settings.preDelay + (settings.exposures * 1) +
                            ((settings.exposures - 1) * settings.delayTime);

    // Start continuous pan if enabled
    if (settings.continuousPan && totalPanAngle != 0.0f)
    {
        currentState = State::Pan;
        uint16_t microstep = 8;

        // Calculate exact speed required to complete pan in totalDurationTime
        // Derived from: currentSlewRate = (2 * rate.tracking) / speed
        //              duration = stepsToMove * 4 * rate.tracking / (speed * TIMER_APB_CLK_FREQ)
        // Solving for speed: speed = stepsToMove * 4 * rate.tracking / (duration *
        // TIMER_APB_CLK_FREQ)
        float absPanAngle = (totalPanAngle < 0) ? -totalPanAngle : totalPanAngle;
        int64_t stepsPerFullRotation =
            STEPS_PER_TRACKER_FULL_REV_INT / (MAX_MICROSTEPS / microstep);
        int64_t stepsToMove = (int64_t) ((absPanAngle / 360.0f) * stepsPerFullRotation + 0.5f);
        uint64_t trackingRate = ra_axis.rate.tracking;

        int maxPanSpeed = MAX_CUSTOM_SLEW_RATE / 4; // 100
        int panSpeed = (int) ((stepsToMove * 4 * trackingRate) /
                              ((uint64_t) totalDurationTime * TIMER_APB_CLK_FREQ));

        print_out("Speed calculation: %.2f deg, %lld steps, %ds => speed=%d", absPanAngle,
                  stepsToMove, totalDurationTime, panSpeed);
        print_out("trackingRate=%llu, stepsPerFullRot=%lld", trackingRate, stepsPerFullRotation);

        if (panSpeed > maxPanSpeed)
            panSpeed = maxPanSpeed;
        else if (panSpeed < MIN_CUSTOM_SLEW_RATE)
            panSpeed = MIN_CUSTOM_SLEW_RATE;

        print_out("Starting continuous pan: %.2f degrees at speed=%d, microstepping=%d",
                  totalPanAngle, panSpeed, microstep);

        // Start continuous pan - it will run throughout the entire sequence
        if (ra_axis.panByDegrees(totalPanAngle, panSpeed, microstep))
            print_out("Continuous pan started successfully");
        else
            print_out("Warning: Continuous pan failed to start");
    }

    // Calculate degrees per interval for incremental panning (if not continuous)
    float degreesPerInterval = 0.0f;
    if (!settings.continuousPan && settings.panAngle > 0.0f && settings.exposures > 1)
    {
        degreesPerInterval = settings.panAngle / (settings.exposures - 1);
        // Apply direction: 0 = reverse (negative), 1 = forward (positive)
        if (!settings.panDirection)
            degreesPerInterval = -degreesPerInterval;

        print_out("Incremental pan: %.2f degrees per interval", degreesPerInterval);
    }

    // Main capture loop
    while (exposuresTaken < settings.exposures && !abortRequested)
    {
        // Check continuous pan is still active (if enabled)
        if (settings.continuousPan && totalPanAngle != 0.0f)
        {
            if (!ra_axis.goToTarget && exposuresTaken < settings.exposures - 1)
                print_out("Warning: Continuous pan stopped unexpectedly");
        }

        // === CAPTURE STATE ===
        currentState = State::Capture;
        print_out("Capture %d/%d start", exposuresTaken + 1, settings.exposures);

        // Trigger camera with short pulse
        triggerOn();
        vTaskDelay(pdMS_TO_TICKS(1000));
        triggerOff();

        currentExposure++;
        exposuresTaken++;
        print_out("Capture %d/%d complete", exposuresTaken, settings.exposures);

        // === PAN STATE (incremental mode only) ===
        // Pan to next position (if not continuous, not last exposure, and pan enabled)
        if (!settings.continuousPan && exposuresTaken < settings.exposures &&
            degreesPerInterval != 0.0f)
        {
            currentState = State::Pan;
            print_out("Pan start: %.2f degrees over %d seconds", degreesPerInterval,
                      settings.delayTime);

            // Use fastest speed with microstep 8 for smooth panning
            // Using 1/8 microstep with 400 slew rate is way too fast for our ISR
            // which results in missed steps and beeping sounds. Avoid it by reducing the speed.
            int panSpeed = MAX_CUSTOM_SLEW_RATE / 4;
            uint16_t microstep = 8;

            print_out("Pan parameters: speed=%d, microstepping=%d", panSpeed, microstep);

            // Start pan - it will run in background during delay
            if (ra_axis.panByDegrees(degreesPerInterval, panSpeed, microstep))
            {
                print_out("Pan started successfully");
            }
            else
            {
                print_out("Warning: Pan failed to start");
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

            // Wait for incremental pan to complete if still active (not continuous mode)
            if (!settings.continuousPan && ra_axis.goToTarget)
            {
                print_out("Waiting for pan to complete...");
                while (ra_axis.goToTarget && !abortRequested)
                {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                print_out("Pan complete");
            }
        }
    }

    // Clean up any remaining pan movement
    if (ra_axis.slewActive || ra_axis.goToTarget)
    {
        if (settings.continuousPan)
        {
            print_out("Waiting for continuous pan to complete...");
            // Wait for continuous pan to finish naturally
            while (ra_axis.goToTarget && !abortRequested)
            {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            print_out("Continuous pan complete");
        }
        else
        {
            // Stop any remaining incremental pan movement
            ra_axis.stopPanByDegrees();
        }
    }

    print_out("=== %s Mode Complete: %d exposures ===", getModeName(), exposuresTaken);
}
