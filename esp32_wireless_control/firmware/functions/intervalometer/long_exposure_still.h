#pragma once

#include "intervalometer_mode.h"

/**
 * @brief Long exposure still photography mode
 *
 * Takes a series of long exposure photos with tracking enabled.
 * Suitable for deep sky astrophotography with guided tracking.
 */
class LongExposureStill : public IntervalometerMode
{
  public:
    LongExposureStill(uint8_t triggerPin, const Settings& settings);
    ~LongExposureStill() override = default;

  protected:
    void executeLoop() override;
    const char* getModeName() const override
    {
        return "LONG_EXPOSURE_STILL";
    }
};
