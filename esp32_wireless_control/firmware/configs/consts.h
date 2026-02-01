#ifndef _CONSTS_H_
#define _CONSTS_H_

#include "config.h"

#define SIDEREAL_DAY_MS 86164000UL
#define SOLAR_DAY_MS 86400000UL
#define LUNAR_DAY_MS 88253300UL

#define RA_SECONDS_PER_FULL_REV 86400 // 24 hours * 3600 seconds/hour

#define MAX_MICROSTEPS 256UL

#define GEAR_RATIO 101.25

// Timer frequency used for hardware timer (40MHz APB clock)
#define TIMER_APB_CLK_FREQ 40000000UL

// Gear ratio as integers to avoid floating point in enum calculations
#define GEAR_RATIO_NUM 405 // 101.25 * 4 = 405
#define GEAR_RATIO_DEN 4   // Denominator = 4

#if STEPPER_TYPE == STEPPER_0_9
// Define steps per revolution based on stepper type
#define STEPPER_STEPS_PER_REV 400
#else
#define STEPPER_STEPS_PER_REV 200
#endif

#define STEPS_PER_TRACKER_FULL_REV (STEPPER_STEPS_PER_REV * GEAR_RATIO * MAX_MICROSTEPS)

// Steps per tracker full revolution using integer gear ratio (for enum calculations)
#define STEPS_PER_TRACKER_FULL_REV_INT                                                             \
    ((STEPPER_STEPS_PER_REV * GEAR_RATIO_NUM * MAX_MICROSTEPS) / GEAR_RATIO_DEN)

#endif /* _CONSTS_H_ */
