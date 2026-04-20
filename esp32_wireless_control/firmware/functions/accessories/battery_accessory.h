/**
 * @file battery_accessory.h
 * @brief Battery voltage monitor accessory (Epic 0.5.3).
 *
 * Concrete SampledSensor that reads the battery voltage via a 100k/100k
 * resistor divider on ADC pin IO9 (V3 boards). Pin is sourced from
 * BoardConfig so no hardware detail leaks into this class.
 *
 * Voltage reconstruction:
 *   V_adc_mv  = rawAdc × 3300 / 4095
 *   V_batt_mv = V_adc_mv × 2          (100k/100k divider halves V_batt)
 *
 * Charge percent heuristic (Li-ion 3.7 V nominal):
 *   0 % = 3.0 V, 100 % = 4.2 V, linear model, clamped.
 *   Labeled **approximate** in all surfaces (web UI, API, UART).
 *
 * Usage:
 *   // Registration (firmware.ino, after BoardConfig init):
 *   AccessoryRegistry::getInstance().registerAccessory(&BatteryAccessory::getInstance());
 *
 *   // Read (REST handler / console command):
 *   AccessorySnapshot snap = BatteryAccessory::getInstance().getSnapshot();
 */

#ifndef BATTERY_ACCESSORY_H
#define BATTERY_ACCESSORY_H

#include "functions/accessories/accessory.h"

class BatteryAccessory : public SampledSensor
{
  public:
    static BatteryAccessory& getInstance();

    const char* getName() const override
    {
        return "battery";
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
    // Pure conversion helpers – no Arduino deps, host-testable.
    // -----------------------------------------------------------------------

    /**
     * @brief Convert a 12-bit ADC reading to battery voltage in mV.
     *
     * Uses 100k/100k divider model:
     *   V_batt_mv = rawAdc × 6600 / 4095
     */
    static uint32_t rawAdcToVoltageMv(uint16_t rawAdc)
    {
        return (uint32_t) rawAdc * 6600u / 4095u;
    }

    /**
     * @brief Convert battery voltage (mV) to approximate charge percent.
     *
     * Li-ion heuristic: 3.0 V = 0 %, 4.2 V = 100 % (linear), clamped.
     * Result is intentionally approximate; label it so in all user-facing surfaces.
     */
    static uint8_t voltageMvToPercent(uint32_t voltage_mv)
    {
        if (voltage_mv <= 3000u)
            return 0u;
        if (voltage_mv >= 4200u)
            return 100u;
        return static_cast<uint8_t>((voltage_mv - 3000u) * 100u / 1200u);
    }

  private:
    BatteryAccessory() = default;
    bool _initialized = false;
    uint16_t _rawAdc = 0;
};

#endif // BATTERY_ACCESSORY_H
