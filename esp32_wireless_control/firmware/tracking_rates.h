#ifndef _TRACKING_RATES_H_
#define _TRACKING_RATES_H_ 1

#include "config.h"
#include <stdint.h>

#if MOTOR_TRACKING_RATE == TRACKING_RATE_BOARD_V2

// Tracking rate enum constants (backward compatible)
enum TrackingRateType
{
    TRACKING_SIDEREAL = 0,
    TRACKING_SOLAR = 1,
    TRACKING_LUNAR = 2
};
// gear ratio 101.25, 1.8deg motor, 16?? msteps, f_cpu@240MHz
//#define TRACKER_MOTOR_MICROSTEPPING 16
//#define STEPS_PER_SECOND_256MICROSTEP	60
//enum trackingRateS
//{
//    TRACKING_SIDEREAL = 15956296, // SIDEREAL (23h,56 min)
//    TRACKING_SOLAR = 16000000,    // SOLAR (24h)
//    TRACKING_LUNAR = 16343204,    // LUNAR (24h, 31 min)
//};

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

    uint64_t getRate()
    {
        return current_rate;
    };
    void setRate(TrackingRateType type);

    uint64_t getStepsPerSecondSidereal();
    uint64_t getStepsPerSecondSolar();
    uint64_t getStepsPerSecondLunar();
};

// Global instance for easy access
extern TrackingRates trackingRates;

#else
#error Unknown tracking rate setting
#endif

#endif /* _TRACKING_RATES_H_ */
