#pragma once

#include "axis.h"
#include "intervalometer_mode.h"


/**
 * @brief Timelapse mode for day/night photography
 *
 * Takes a series of short-exposure photos (camera-timed).
 * Tracking is disabled during timelapse.
 * Supports optional dithering between frames.
 */
class Timelapse : public IntervalometerMode
{
  public:
    Timelapse(uint8_t triggerPin, const Settings& settings);
    ~Timelapse() override = default;

  protected:
    void executeLoop() override;
    const char* getModeName() const override
    {
        return "TIMELAPSE";
    }
};
