#include "long_exposure_movie.h"
#include "uart.h"

LongExposureMovie::LongExposureMovie(uint8_t triggerPin, const Settings& settings)
    : IntervalometerMode(triggerPin, settings), framesTaken(0)
{
}

void LongExposureMovie::executeLoop()
{
    print_out("=== %s Mode Started ===", getModeName());
    print_out("Settings: %d frames, %d exposures/frame x %ds, delay: %ds", settings.frames,
              settings.exposures, settings.exposureTime, settings.delayTime);

    // Enable counter for position tracking (required for rewind)
    if (!ra_axis.counterActive)
    {
        ra_axis.resetAxisCount();
        ra_axis.counterActive = true;
    }

    // Perform pre-delay
    performPreDelay();
    if (abortRequested)
        return;

    // Outer loop for frames
    while (framesTaken < settings.frames && !abortRequested)
    {
        print_out("=== Frame %d/%d ===", framesTaken + 1, settings.frames);
        exposuresTaken = 0;

        // Inner loop for exposures within this frame
        while (exposuresTaken < settings.exposures && !abortRequested)
        {
            // === CAPTURE STATE ===
            currentState = State::Capture;
            print_out("Frame %d, Exposure %d/%d start", framesTaken + 1, exposuresTaken + 1,
                      settings.exposures);

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
            print_out("Frame %d, Exposure %d/%d complete", framesTaken + 1, exposuresTaken,
                      settings.exposures);

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

        framesTaken++;
        print_out("Frame %d/%d complete", framesTaken, settings.frames);

        // === REWIND STATE (if not the last frame) ===
        if (framesTaken < settings.frames)
        {
            if (!performRewind())
            {
                return; // Aborted during rewind
            }
        }
    }

    // Disable counter when done
    ra_axis.counterActive = false;

    print_out("=== %s Mode Complete: %d frames, %d total exposures ===", getModeName(), framesTaken,
              framesTaken * settings.exposures);
}

bool LongExposureMovie::performRewind()
{
    currentState = State::Rewind;
    print_out("Rewind start - returning to position 0");

    // Set target to starting position
    ra_axis.setAxisTargetCount(0);

    if (ra_axis.targetCount != ra_axis.axisCountValue)
    {
        ra_axis.goToTarget = true;
        // Rewind at fast speed (20x tracking rate)
        ra_axis.startSlew(ra_axis.rate.tracking / MAX_CUSTOM_SLEW_RATE,
                          !ra_axis.direction.tracking);

        // Wait for slew to complete
        while (ra_axis.slewActive && !abortRequested)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    print_out("Rewind complete - position: %lld", ra_axis.getAxisCount());

    return !abortRequested;
}

uint32_t LongExposureMovie::calculateTotalDuration() const
{
    // Total time = preDelay + (frames * (exposures * exposureTime + (exposures-1) * delayTime))
    uint32_t frames = settings.frames ? settings.frames : 1;
    uint32_t exposures = settings.exposures ? settings.exposures : 1;

    uint32_t timePerFrame = (exposures * settings.exposureTime) +
                            ((exposures > 1) ? ((exposures - 1) * settings.delayTime) : 0);
    uint32_t total = settings.preDelay + (frames * timePerFrame);

    return (total == 0) ? 1 : total;
}
