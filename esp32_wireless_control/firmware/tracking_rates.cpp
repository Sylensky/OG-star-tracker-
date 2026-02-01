#include "tracking_rates.h"
#include "uart.h"

// Include for EEPROM operations (forward declaration in header)
#include "eeprom_manager.h"
#include "tools/heap_monitor.h"

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
    HeapMonitor::log("before-read-tracking-presets");
    print_out("Reading tracking rate presets from EEPROM, bytes read: ");
    print_out("%d", EepromManager::readPresets(TRACKING_RATE_PRESETS_EEPROM_START_LOCATION,
                                               trackingRatePresets));
    HeapMonitor::log("after-read-tracking-presets");
#else
    EepromManager::readPresets(TRACKING_RATE_PRESETS_EEPROM_START_LOCATION, trackingRatePresets);
#endif
}

// Global instance of tracking rates
TrackingRates trackingRates;
