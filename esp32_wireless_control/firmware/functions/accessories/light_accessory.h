/**
 * @file light_accessory.h
 * @brief Ambient light sensor accessory (Epic 0.5.4).
 *
 * Concrete SampledSensor that reads ambient light via a 10k pull-up +
 * GL5516 LDR-to-GND voltage divider on ADC pin IO10 (V3 boards).
 *
 * Circuit: 3.3V → 10kΩ → ADC node → GL5516 → GND
 *   - Bright: GL5516 resistance low  → ADC node HIGH → high rawAdc
 *   - Dark:   GL5516 resistance high → ADC node LOW  → low rawAdc
 *
 * Percent normalization:
 *   percent = rawAdc × 100 / 4095, clamped to [0, 100].
 *   0 % = fully dark, 100 % = brightest detectable.
 *
 * Absolute lux is NOT reported (requires calibration data).
 *
 * Usage:
 *   // Registration (firmware.ino, after BoardConfig init):
 *   AccessoryRegistry::getInstance().registerAccessory(&LightAccessory::getInstance());
 *
 *   // Read (REST handler / console command):
 *   AccessorySnapshot snap = LightAccessory::getInstance().getSnapshot();
 */

#ifndef LIGHT_ACCESSORY_H
#define LIGHT_ACCESSORY_H

#include "functions/accessories/accessory.h"

class LightAccessory : public SampledSensor
{
  public:
    static LightAccessory& getInstance();

    const char* getName() const override
    {
        return "light";
    }

    bool isSupported() const override;

    /**
     * @brief Configure ADC pin attenuation and take an initial reading.
     * @return true if supported and init succeeded; false if unsupported.
     */
    bool init() override;

    bool isInitialized() const override
    {
        return _initialized;
    }

    /**
     * @brief Average 10 ADC readings and cache the result.
     * No-op if not initialised.
     */
    void refresh() override;

    uint16_t getRawAdc() const override
    {
        return _rawAdc;
    }

    AccessorySnapshot getSnapshot() const override;

    // -----------------------------------------------------------------------
    // Pure conversion helper – no Arduino deps, host-testable.
    // -----------------------------------------------------------------------

    /**
     * @brief Normalize a 12-bit ADC reading to a light percent [0, 100].
     *
     * Higher rawAdc = brighter (10k pull-up + GL5516 LDR to GND).
     *   percent = rawAdc × 100 / 4095, clamped to [0, 100].
     */
    static uint8_t rawAdcToPercent(uint16_t rawAdc)
    {
        if (rawAdc >= 4095u)
            return 100u;
        return static_cast<uint8_t>((uint32_t) rawAdc * 100u / 4095u);
    }

  private:
    LightAccessory() = default;
    bool _initialized = false;
    uint16_t _rawAdc = 0;
};

#endif // LIGHT_ACCESSORY_H
