#pragma once

#include "axis.h"
#include "intervalometer_mode.h"

/**
 * @brief Timelapse with panning mode
 *
 * Takes a series of photos while continuously panning across a specified angle.
 * The pan is distributed evenly across the entire capture sequence.
 * Maintains position tracking throughout to ensure accurate panning.
 */
class TimelapsePan : public IntervalometerMode
{
  public:
    TimelapsePan(uint8_t triggerPin, const Settings& settings);
    ~TimelapsePan() override = default;

  protected:
    void executeLoop() override;
    const char* getModeName() const override
    {
        return "TIMELAPSE_PAN";
    }
};
