#include "long_exposure_still.h"
#include "uart.h"

LongExposureStill::LongExposureStill(uint8_t triggerPin, const Settings& settings)
    : IntervalometerMode(triggerPin, settings)
{
}

void LongExposureStill::executeLoop()
{
    print_out("=== %s Mode Started ===", getModeName());
    print_out("Settings: %d exposures x %ds, delay: %ds", settings.exposures, settings.exposureTime,
              settings.delayTime);

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

        // Trigger camera
        triggerOn();

        // Wait for exposure duration
        if (!waitWithAbortCheck(settings.exposureTime * 1000))
        {
            triggerOff();
            return;
        }

        // End exposure
        triggerOff();
        currentExposure++;
        exposuresTaken++;
        print_out("Capture %d/%d complete", exposuresTaken, settings.exposures);

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
