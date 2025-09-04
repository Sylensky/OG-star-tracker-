#ifndef _TRACKING_RATES_H_
#define _TRACKING_RATES_H_ 1

#include <stdint.h>

#include "configs/config.h"
#include "configs/consts.h"

#if MOTOR_TRACKING_RATE == TRACKING_RATE_BOARD_V2

// Tracking rate enum constants (backward compatible)
enum TrackingRateType
{
    TRACKING_SIDEREAL = 1,
    TRACKING_SOLAR = 2,
    TRACKING_LUNAR = 3,
    TRACKING_CUSTOM = 4,
};

// Tracking rate preset structure
struct TrackingRatePreset
{                                // 12 bytes
    uint8_t trackingRateType;    // 1b (1=sidereal, 2=solar, 3=lunar, 4=custom)
    uint64_t customTrackingRate; // 8b - custom fine-tuned rate (when type=4)
    uint8_t padding[3];          // 3b padding for alignment
};

// Tracking rates class with calculated timer reload values
class TrackingRates
{
  private:
    uint64_t current_rate;
    uint64_t sidereal_rate;
    uint64_t solar_rate;
    uint64_t lunar_rate;

    // Calculate tracking rate from period in milliseconds
    uint64_t calculateTrackingRate(uint64_t period_ms);

  public:
    TrackingRates(); // Constructor calculates all rates

    // Preset management
    TrackingRatePreset trackingRatePresets[5]; // 5 separate tracking rate presets

    uint64_t getRate()
    {
        return current_rate;
    };
    void setRate(TrackingRateType type);
    void setCustomRate(uint64_t rate);

    uint64_t getSiderealRate()
    {
        return sidereal_rate;
    }
    uint64_t getSolarRate()
    {
        return solar_rate;
    }
    uint64_t getLunarRate()
    {
        return lunar_rate;
    }

    uint64_t getStepsPerSecondSidereal();
    uint64_t getStepsPerSecondSolar();
    uint64_t getStepsPerSecondLunar();

    // Preset management methods
    void saveTrackingRatePreset(uint8_t preset, uint8_t rateType, uint64_t customRate);
    void loadTrackingRatePreset(uint8_t preset);
    void saveTrackingRatePresetsToEEPROM();
    void readTrackingRatePresetsFromEEPROM();
};

// Global instance for easy access
extern TrackingRates trackingRates;

#else
#error Unknown tracking rate setting
#endif

#endif /* _TRACKING_RATES_H_ */
