#include "timelapse_pan.h"
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

    // Calculate degrees per interval if panning
    float degreesPerInterval = 0.0f;
    if (settings.panAngle > 0.0f && settings.exposures > 1)
    {
        degreesPerInterval = settings.panAngle / (settings.exposures - 1);
        // Apply direction (negative = reverse)
        if (!settings.panDirection)
            degreesPerInterval = -degreesPerInterval;

        print_out("Pan: %.2f degrees per interval", degreesPerInterval);
    }

    // Main capture loop
    while (exposuresTaken < settings.exposures && !abortRequested)
    {
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

        // === PAN STATE ===
        // Pan to next position (if not last exposure and pan enabled)
        if (exposuresTaken < settings.exposures && degreesPerInterval != 0.0f)
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

            // Wait for pan to complete if still active
            if (ra_axis.goToTarget)
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

    if (ra_axis.slewActive || ra_axis.goToTarget)
        ra_axis.stopPanByDegrees();

    print_out("=== %s Mode Complete: %d exposures ===", getModeName(), exposuresTaken);
}
