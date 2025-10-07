#pragma once

#include "axis.h"
#include "intervalometer_mode.h"

/**
 * @brief Long exposure movie mode
 *
 * Takes multiple frames, each consisting of multiple exposures with tracking.
 * After each frame, rewinds to starting position for next frame.
 * Uses axis counter to track position and enable precise rewinding.
 */
class LongExposureMovie : public IntervalometerMode
{
  public:
    LongExposureMovie(uint8_t triggerPin, const Settings& settings);
    ~LongExposureMovie() override = default;

  protected:
    void executeLoop() override;
    const char* getModeName() const override
    {
        return "LONG_EXPOSURE_MOVIE";
    }
    uint32_t calculateTotalDuration() const override;

  private:
    bool performRewind();
    uint16_t framesTaken;
};
