#include "tracking_rates.h"
#include "uart.h"

// Include for EEPROM operations (forward declaration in header)
#include "eeprom_manager.h"

// Calculate tracking rate from period in milliseconds
// Formula: Timer_reload_value = TIMER_APB_CLK_FREQ / timer_interrupts_per_second
// Where timer_interrupts_per_second = steps_per_second * 2 (ISR toggles HIGH/LOW)
uint64_t TrackingRates::calculateTrackingRate(uint64_t period_ms)
{
    // Convert STEPS_PER_TRACKER_FULL_REV_INT from 256 microstepping to 64 microstepping
    uint64_t steps_per_revolution_microstep =
        STEPS_PER_TRACKER_FULL_REV_INT / (MAX_MICROSTEPS / TRACKER_MOTOR_MICROSTEPPING);

    // Calculate steps per second for the given period
    // steps_per_second = steps_per_revolution / period_in_seconds
    // period_in_seconds = period_ms / 1000
    double steps_per_second =
        (double) steps_per_revolution_microstep / ((double) period_ms / 1000.0);

    // ISR creates HIGH/LOW cycle, so we need 2x timer interrupts per stepper step
    double timer_interrupts_per_second = steps_per_second * 2.0;

    // Timer reload value = timer_frequency / timer_interrupts_per_second
    uint64_t timer_reload_value = (uint64_t) (TIMER_APB_CLK_FREQ / timer_interrupts_per_second);

    return timer_reload_value;
}

// Constructor - calculates all tracking rates
TrackingRates::TrackingRates()
{
    sidereal_rate = calculateTrackingRate(SIDEREAL_DAY_MS);
    solar_rate = calculateTrackingRate(SOLAR_DAY_MS);
    lunar_rate = calculateTrackingRate(LUNAR_DAY_MS);
    setRate(TRACKING_RATE); // Set initial rate based on TRACKING_RATE

    for (int i = 0; i < 5; i++)
    {
        // Initialize presets
        trackingRatePresets[i].trackingRateType = TRACKING_SIDEREAL;
        trackingRatePresets[i].customTrackingRate = 0;
    }
}

void TrackingRates::setRate(TrackingRateType type)
{
    switch (type)
    {
        case TRACKING_SIDEREAL:
            current_rate = sidereal_rate;
            break;
        case TRACKING_SOLAR:
            current_rate = solar_rate;
            break;
        case TRACKING_LUNAR:
            current_rate = lunar_rate;
            break;
        case TRACKING_CUSTOM:
            print_out("WARNING: This should not be reached - use setCustomRate()");
            break;
    }
};

void TrackingRates::setCustomRate(uint64_t rate)
{
    current_rate = rate;
};

// Public functions to get steps per second at 256 microstepping
uint64_t TrackingRates::getStepsPerSecondSidereal()
{
    return ((((uint64_t) STEPS_PER_TRACKER_FULL_REV_INT) * 1000ULL) / SIDEREAL_DAY_MS);
}

uint64_t TrackingRates::getStepsPerSecondSolar()
{
    return ((((uint64_t) STEPS_PER_TRACKER_FULL_REV_INT) * 1000ULL) / SOLAR_DAY_MS);
}

uint64_t TrackingRates::getStepsPerSecondLunar()
{
    return ((((uint64_t) STEPS_PER_TRACKER_FULL_REV_INT) * 1000ULL) / LUNAR_DAY_MS);
}

void TrackingRates::saveTrackingRatePreset(uint8_t preset, uint8_t rateType, uint64_t customRate)
{
    if (preset < 5)
    {
        trackingRatePresets[preset].trackingRateType = rateType;
        trackingRatePresets[preset].customTrackingRate = customRate;
    }
}

void TrackingRates::loadTrackingRatePreset(uint8_t preset)
{
    if (preset < 5)
    {
        TrackingRateType type =
            static_cast<TrackingRateType>(trackingRatePresets[preset].trackingRateType);

        if (type == TRACKING_CUSTOM)
            setCustomRate(trackingRatePresets[preset].customTrackingRate);
        else
            setRate(type);
    }
}

void TrackingRates::saveTrackingRatePresetsToEEPROM()
{
#if DEBUG == 1
    print_out("Saving tracking rate presets to EEPROM, bytes written: ");
    print_out("%d", EepromManager::writePresets(TRACKING_RATE_PRESETS_EEPROM_START_LOCATION,
                                                trackingRatePresets));
#else
    EepromManager::writePresets(TRACKING_RATE_PRESETS_EEPROM_START_LOCATION, trackingRatePresets);
#endif
}

void TrackingRates::readTrackingRatePresetsFromEEPROM()
{
#if DEBUG == 1
    print_out("Reading tracking rate presets from EEPROM, bytes read: ");
    print_out("%d", EepromManager::readPresets(TRACKING_RATE_PRESETS_EEPROM_START_LOCATION,
                                               trackingRatePresets));
#else
    EepromManager::readPresets(TRACKING_RATE_PRESETS_EEPROM_START_LOCATION, trackingRatePresets);
#endif
}

void TrackingRates::printTrackingRatePresets()
{
    print_out("Tracking Rate Presets:");
    for (int i = 0; i < 5; i++)
    {
        print_out("Preset %d:", i);
        print_out("  Type: %d", trackingRatePresets[i].trackingRateType);
        print_out("  Custom Rate: %llu", trackingRatePresets[i].customTrackingRate);
    }
}

void TrackingRates::debugTrackingRates()
{
    print_out("\n========================================");
    print_out("TRACKING RATES DEBUG");
    print_out("========================================");
    print_out("Hardware Configuration:");
    print_out("  TRACKER_MOTOR_MICROSTEPPING: %d", TRACKER_MOTOR_MICROSTEPPING);
    print_out("  MAX_MICROSTEPS: %d", MAX_MICROSTEPS);
    print_out("  STEPS_PER_TRACKER_FULL_REV_INT: %llu", (uint64_t) STEPS_PER_TRACKER_FULL_REV_INT);
    print_out("  TIMER_APB_CLK_FREQ: %lu Hz", (unsigned long) TIMER_APB_CLK_FREQ);

    uint64_t steps_per_rev_at_microstep =
        STEPS_PER_TRACKER_FULL_REV_INT / (MAX_MICROSTEPS / TRACKER_MOTOR_MICROSTEPPING);
    print_out("  Steps per rev at %d microstep: %llu", TRACKER_MOTOR_MICROSTEPPING,
              steps_per_rev_at_microstep);

    print_out("\nAstronomical Constants:");
    print_out("  SIDEREAL_DAY_MS: %lu ms", (unsigned long) SIDEREAL_DAY_MS);
    print_out("  SOLAR_DAY_MS:    %lu ms", (unsigned long) SOLAR_DAY_MS);
    print_out("  LUNAR_DAY_MS:    %lu ms", (unsigned long) LUNAR_DAY_MS);

    print_out("\nCalculated Timer Reload Values:");
    print_out("  Sidereal rate: %llu", sidereal_rate);
    print_out("  Solar rate:    %llu", solar_rate);
    print_out("  Lunar rate:    %llu", lunar_rate);

    print_out("\nSteps Per Second:");
    print_out("  Sidereal: %llu steps/sec", getStepsPerSecondSidereal());
    print_out("  Solar:    %llu steps/sec", getStepsPerSecondSolar());
    print_out("  Lunar:    %llu steps/sec", getStepsPerSecondLunar());

    print_out("\nActual Tracking Speeds:");
    double sidereal_hz = (double) TIMER_APB_CLK_FREQ / (double) sidereal_rate;
    double solar_hz = (double) TIMER_APB_CLK_FREQ / (double) solar_rate;
    double lunar_hz = (double) TIMER_APB_CLK_FREQ / (double) lunar_rate;
    print_out("  Sidereal: %.6f interrupts/sec", sidereal_hz);
    print_out("  Solar:    %.6f interrupts/sec", solar_hz);
    print_out("  Lunar:    %.6f interrupts/sec", lunar_hz);

    double sidereal_steps_sec = sidereal_hz / 2.0;
    double solar_steps_sec = solar_hz / 2.0;
    double lunar_steps_sec = lunar_hz / 2.0;
    print_out("  Sidereal: %.6f steps/sec", sidereal_steps_sec);
    print_out("  Solar:    %.6f steps/sec", solar_steps_sec);
    print_out("  Lunar:    %.6f steps/sec", lunar_steps_sec);

    double sidereal_period_sec = (double) steps_per_rev_at_microstep / sidereal_steps_sec;
    double solar_period_sec = (double) steps_per_rev_at_microstep / solar_steps_sec;
    double lunar_period_sec = (double) steps_per_rev_at_microstep / lunar_steps_sec;
    print_out("\nActual Periods Achieved:");
    print_out("  Sidereal: %.2f seconds (target: %lu)", sidereal_period_sec,
              (unsigned long) (SIDEREAL_DAY_MS / 1000));
    print_out("  Solar:    %.2f seconds (target: %lu)", solar_period_sec,
              (unsigned long) (SOLAR_DAY_MS / 1000));
    print_out("  Lunar:    %.2f seconds (target: %lu)", lunar_period_sec,
              (unsigned long) (LUNAR_DAY_MS / 1000));

    double sidereal_error = sidereal_period_sec - (SIDEREAL_DAY_MS / 1000.0);
    double solar_error = solar_period_sec - (SOLAR_DAY_MS / 1000.0);
    double lunar_error = lunar_period_sec - (LUNAR_DAY_MS / 1000.0);
    print_out("\nTracking Errors:");
    print_out("  Sidereal: %+.3f seconds (%+.6f%%)", sidereal_error,
              (sidereal_error / (SIDEREAL_DAY_MS / 1000.0)) * 100);
    print_out("  Solar:    %+.3f seconds (%+.6f%%)", solar_error,
              (solar_error / (SOLAR_DAY_MS / 1000.0)) * 100);
    print_out("  Lunar:    %+.3f seconds (%+.6f%%)", lunar_error,
              (lunar_error / (LUNAR_DAY_MS / 1000.0)) * 100);
    print_out("========================================\n");
}

// Global instance of tracking rates
TrackingRates trackingRates;
