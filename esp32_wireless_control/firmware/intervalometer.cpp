#include "esp32-hal-gpio.h"
// #include <cstdlib>
#include "intervalometer.h"
#include "uart.h"

void intervalometer_ISA()
{
    intervalometer.nextState = true;
}

Intervalometer::Intervalometer(uint8_t triggerPinArg)
    : intervalometerTimer(2000, intervalometer_ISA)
{ // 2kHz resolution of 0.5 ms

    currentState = INACTIVE;
    currentErrorMessage = ERR_MSG_NONE;
    intervalometerTimer.stop();
    nextState = false;
    intervalometerActive = false;
    // readPresetsFromEEPROM();
    timerStarted = false;
    axisMoving = false;
    total = 0;
    triggerPin = triggerPinArg;
    currentSettings.mode = LONG_EXPOSURE_STILL;
    currentSettings.exposures = 1;
    currentSettings.delayTime = 1;
    currentSettings.preDelay = 1;
    currentSettings.exposureTime = 1;
    currentSettings.panAngle = 0.0;
    currentSettings.panDuration = 0;
    currentSettings.panDirection = true;
    currentSettings.dither = false;
    currentSettings.ditherFrequency = 1;
    currentSettings.enableTracking = false;
    currentSettings.frames = 1;
    currentSettings.pixelSize = 1.0;
    currentSettings.focalLength = 1;
    previousDitherDirection = 0;
}

void Intervalometer::abortCapture()
{
    currentState = INACTIVE;
}

void Intervalometer::startCapture()
{
    currentState = PRE_DELAY;
    intervalometerActive = true;
    startCaptureTickCount = xTaskGetTickCount();
    captureDurationTickCount = pdMS_TO_TICKS(
        (currentSettings.preDelay + currentSettings.exposures * currentSettings.exposureTime +
         (currentSettings.exposures - 1) * currentSettings.delayTime) *
        1000);
}

/* MODES:
LONG_EXPOSURE_STILL: Predelay>(Internal Timed Capture>Dither?>Delay (x exposures)) (with tracking)
LONG_EXPOSURE_MOVIE: (Predelay>(Internal Timed Capture>Dither?>Delay (x exposures))>rewind axis (x
frames)) (with tracking)
DAY_TIME_LAPSE: Predelay>(Camera Capture>delay (x exposures)) (no tracking)
DAY_TIME_LAPSE_PAN: Predelay>Start continuous pan>(Camera Capture>delay (x exposures)) (no tracking,
pans continuously during captures) NIGHT_TIME_LAPSE_PAN: Predelay>(Camera Capture>pan step>delay (x
exposures)) (no tracking, stops for each exposure)
*/

void Intervalometer::run()
{
    switch (currentState)
    {
        case INACTIVE:
            print_out("Intervalometer: INACTIVE");
            ra_axis.stopSlew();
            intervalometerTimer.stop();
            digitalWrite(triggerPin, LOW);
            intervalometerActive = false;
            exposures_taken = 0;
            current_exposure = 0;
            frames_taken = 0;
            timerStarted = false;
            axisMoving = false;
            nextState = false;
            ra_axis.counterActive = false;
            if (!currentSettings.enableTracking)
            {
                ra_axis.stopTracking();
            }
            break;
        case PRE_DELAY:

            if (!timerStarted)
            {
                print_out("Intervalometer: PREDELAY_START");
                // Only stop tracking for DAY modes, not NIGHT_TIME_LAPSE_PAN
                if ((currentSettings.mode == DAY_TIME_LAPSE ||
                     currentSettings.mode == DAY_TIME_LAPSE_PAN) &&
                    ra_axis.trackingActive)
                {
                    ra_axis.stopTracking();
                }
                ra_axis.counterActive = currentSettings.mode == LONG_EXPOSURE_MOVIE ? true : false;
                intervalometerTimer.start(2000 * currentSettings.preDelay, false);
                timerStarted = true;
                intervalometerActive = true;
            }
            if (nextState)
            {
                intervalometerTimer.stop();
                print_out("Intervalometer: PREDELAY_STOP");
                nextState = false;
                timerStarted = false;

                // For DAY_TIME_LAPSE_PAN, start continuous panning after pre-delay
                if (currentSettings.mode == DAY_TIME_LAPSE_PAN && currentSettings.panAngle > 0.0)
                {
                    print_out("Intervalometer: starting continuous pan");
                    print_out("Pan angle: %.2f degrees", currentSettings.panAngle);

                    // Calculate total steps for entire pan
                    uint64_t arcSecsToMove = uint64_t(3600.0 * currentSettings.panAngle);
                    int64_t totalStepsToMove = currentSettings.panDirection
                                                   ? arcSecsToMove / ARCSEC_PER_STEP
                                                   : (arcSecsToMove / ARCSEC_PER_STEP) * -1;

                    print_out("Total steps to move: %lld", totalStepsToMove);

                    // Calculate pan speed based on total duration
                    // If panDuration is set, use it; otherwise calculate from exposures and delays
                    uint32_t totalDuration =
                        currentSettings.panDuration > 0
                            ? currentSettings.panDuration
                            : (currentSettings.exposures * currentSettings.delayTime);

                    print_out("Total duration: %u seconds", totalDuration);

                    // Ensure we have a valid duration to avoid division by zero
                    if (totalDuration == 0)
                    {
                        totalDuration = 1;
                    }

                    // Calculate required slew rate: steps per second
                    // rate.tracking is steps per sidereal second, we need steps per second for our
                    // total move
                    uint64_t stepsPerSecond =
                        (totalStepsToMove < 0 ? -totalStepsToMove : totalStepsToMove) /
                        totalDuration;

                    print_out("Steps per second: %llu", stepsPerSecond);

                    // Convert to tracking rate divisor (tracking rate / x)
                    uint64_t slewRate = stepsPerSecond > 0 ? ra_axis.rate.tracking / stepsPerSecond
                                                           : ra_axis.rate.tracking / 10;

                    print_out("Slew rate: %llu", slewRate);

                    ra_axis.resetAxisCount();
                    ra_axis.setAxisTargetCount(totalStepsToMove);
                    ra_axis.counterActive = true;
                    ra_axis.goToTarget = true;
                    ra_axis.startSlew(slewRate, currentSettings.panDirection);

                    print_out("Pan goto started");
                }

                currentState = CAPTURE;
            }
            break;
        case CAPTURE:
            if (!timerStarted)
            { // nightime modes
                print_out("Intervalometer: capture_start");
                if (currentSettings.mode == LONG_EXPOSURE_MOVIE && !ra_axis.counterActive)
                {
                    ra_axis.resetAxisCount();
                    ra_axis.counterActive = true;
                }
                digitalWrite(triggerPin, HIGH);
                // DAY_TIME_LAPSE, DAY_TIME_LAPSE_PAN use short camera-timed exposures
                // NIGHT_TIME_LAPSE_PAN, LONG_EXPOSURE_STILL, LONG_EXPOSURE_MOVIE use firmware-timed
                // exposures
                if (currentSettings.mode == DAY_TIME_LAPSE ||
                    currentSettings.mode == DAY_TIME_LAPSE_PAN)
                {
                    intervalometerTimer.start(2000, false); // 1 sec should cover day time
                                                            // exposures.
                    vTaskDelay(10);
                    digitalWrite(triggerPin, LOW);
                }
                else
                {
                    // For NIGHT_TIME_LAPSE_PAN and long exposure modes, firmware controls exposure
                    // time
                    intervalometerTimer.start(2000 * currentSettings.exposureTime, false);
                }
                current_exposure++;
                timerStarted = true;
            }
            if (nextState)
            {
                digitalWrite(triggerPin, LOW);
                intervalometerTimer.stop();
                print_out("Intervalometer: capture_end");
                nextState = false;
                timerStarted = false;
                exposures_taken++;
                currentState = currentSettings.dither                         ? DITHER
                               : currentSettings.mode == NIGHT_TIME_LAPSE_PAN ? PAN
                                                                              : DELAY;
            }
            break;
        case DITHER:
            if (exposures_taken % currentSettings.ditherFrequency == 0)
            {
                if (!axisMoving)
                {
                    print_out("Intervalometer: dither_start");
                    axisMoving = true;
                    uint8_t randomDirection = biasedRandomDirection(previousDitherDirection);
                    previousDitherDirection = randomDirection;
                    int16_t stepsToDither =
                        ((random(100 * DITHER_DISTANCE_X10_PIXELS) + 1) / 100.0) *
                        getStepsPerTenPixels();
                    stepsToDither = randomDirection ? stepsToDither : stepsToDither * -1;
                    ra_axis.setAxisTargetCount(stepsToDither + ra_axis.axisCountValue);
                    if (ra_axis.targetCount != ra_axis.axisCountValue)
                    {
                        ra_axis.goToTarget = true;
                        ra_axis.counterActive = true;
                        ra_axis.startSlew(ra_axis.rate.tracking / 3,
                                          randomDirection); // dither at 6 x tracking rate.
                    }
                }
                if (!ra_axis.slewActive)
                {
                    print_out("Intervalometer: dither_end");
                    if (currentSettings.mode != LONG_EXPOSURE_MOVIE)
                    {
                        ra_axis.counterActive = false;
                    }
                    axisMoving = false;
                    currentState = DELAY;
                }
            }
            else
            {
                currentState = DELAY;
            }
            break;
        case PAN:
            // This state is only used for NIGHT_TIME_LAPSE_PAN (step-by-step panning)
            // DAY_TIME_LAPSE_PAN uses continuous panning started in PRE_DELAY
            if (!axisMoving)
            {
                print_out("Intervalometer: pan_step_start");
                axisMoving = true;

                // Only pan if panAngle > 0
                if (currentSettings.panAngle > 0.0)
                {
                    // Temporarily stop tracking during pan movement to avoid interference
                    bool wasTracking = ra_axis.trackingActive;
                    if (wasTracking)
                    {
                        ra_axis.stopTracking();
                    }

                    // Calculate angle per shot (total angle divided by number of exposures)
                    float anglePerShot = currentSettings.panAngle / currentSettings.exposures;
                    uint64_t arcSecsToMove = uint64_t(3600.0 * anglePerShot);
                    int64_t stepsToMove = currentSettings.panDirection
                                              ? arcSecsToMove / ARCSEC_PER_STEP
                                              : (arcSecsToMove / ARCSEC_PER_STEP) * -1;

                    // For NIGHT_TIME_LAPSE_PAN, move incrementally after each exposure
                    int64_t currentTarget = ra_axis.axisCountValue + stepsToMove;
                    ra_axis.setAxisTargetCount(currentTarget);
                    if (ra_axis.targetCount != ra_axis.axisCountValue)
                    {
                        ra_axis.counterActive = true;
                        ra_axis.goToTarget = true;
                        ra_axis.startSlew(ra_axis.rate.tracking / 10,
                                          currentSettings.panDirection); // pan at 20x tracking rate
                    }
                }
            }
            if (!ra_axis.slewActive)
            {
                print_out("Intervalometer: pan_step_end");
                axisMoving = false;
                ra_axis.counterActive = false;
                ra_axis.goToTarget = false;

                // Resume tracking if it was enabled before the pan
                if (currentSettings.enableTracking && !ra_axis.trackingActive)
                {
                    ra_axis.startTracking(ra_axis.rate.tracking, ra_axis.direction.tracking);
                }

                currentState = DELAY;
            }
            break;
        case DELAY:
            if (exposures_taken >= currentSettings.exposures)
            {
                currentState = currentSettings.mode != LONG_EXPOSURE_MOVIE ? INACTIVE
                               : frames_taken < currentSettings.frames     ? REWIND
                                                                           : INACTIVE;
            }
            else
            {
                if (!timerStarted)
                {
                    print_out("Intervalometer: delay_start");
                    intervalometerTimer.start(2000 * currentSettings.delayTime, false);
                    timerStarted = true;
                }
                if (nextState)
                {
                    intervalometerTimer.stop();
                    print_out("Intervalometer: delay_end");
                    nextState = false;
                    timerStarted = false;
                    currentState = CAPTURE;
                }
            }

            break;
        case REWIND:
            if (!axisMoving)
            {
                print_out("Intervalometer: rewind_start");
                axisMoving = true;
                exposures_taken = 0;
                current_exposure = 0;
                frames_taken++;
                ra_axis.setAxisTargetCount(0);
                if (ra_axis.targetCount != ra_axis.axisCountValue)
                {
                    ra_axis.goToTarget = true;
                    ra_axis.startSlew(ra_axis.rate.tracking / 10,
                                      !ra_axis.direction.tracking); // rewind at 20 x tracking rate.
                }
            }
            if (!ra_axis.slewActive)
            {
                print_out("Intervalometer: rewind_end");
                axisMoving = false;
                currentState = CAPTURE;
            }
            break;
    }
}

void Intervalometer::saveSettingsToPreset(uint8_t preset)
{
    presets[preset] = currentSettings;
    savePresetsToEEPPROM();
}

void Intervalometer::readSettingsFromPreset(uint8_t preset)
{
    currentSettings = presets[preset];
}

void Intervalometer::savePresetsToEEPPROM()
{
#if DEBUG == 1
    print_out("writtenBytes: ");
#endif
    print_out("%d", EepromManager::writePresets(PRESETS_EEPROM_START_LOCATION, presets));
}

void Intervalometer::readPresetsFromEEPROM()
{
#if DEBUG == 1
    print_out("readBytes: ");
#endif
    print_out("%d", EepromManager::readPresets(PRESETS_EEPROM_START_LOCATION, presets));
}

uint16_t Intervalometer::getStepsPerTenPixels()
{
    // add 0.5 to round up float to nearest int while casting
    return (int) (((getArcsec_per_pixel() * 10.0) / ARCSEC_PER_STEP) + 0.5);
}

float Intervalometer::getArcsec_per_pixel()
{
    return ((float) currentSettings.pixelSize / currentSettings.focalLength) * 206.265;
}

// when tracker moves left, next time its 5% higher chance tracked will move right
// with this tracker should keep in the middle in average
uint8_t Intervalometer::biasedRandomDirection(uint8_t previous_direction)
{
    // Adjust probabilities based on previous selection
    uint8_t direction_left_bias = previous_direction == 0 ? 45 : 55;
    uint8_t rand_val = random(100); // random number between 0 and 99
    uint8_t randomDirection = rand_val < direction_left_bias ? 0 : 1;
    return randomDirection;
}

Intervalometer intervalometer(INTERV_PIN);
