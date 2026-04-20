/**
 * @file battery_accessory.cpp
 * @brief BatteryAccessory implementation.
 */

#include "functions/accessories/battery_accessory.h"
#include "functions/board_version/board_config.h"
#include "functions/board_version/board_version.h"

#include <Arduino.h>

BatteryAccessory& BatteryAccessory::getInstance()
{
    static BatteryAccessory instance;
    return instance;
}

bool BatteryAccessory::isSupported() const
{
    return BoardConfigManager::getInstance().getConfig().hasBatteryMonitor();
}

bool BatteryAccessory::init()
{
    if (!isSupported())
        return false;

    uint8_t pin = BoardConfigManager::getInstance().getConfig().getBatteryAdcPin();
    analogSetPinAttenuation(pin, ADC_11db); // 0–3.1 V range; max V_adc ≈ 2.1 V at 4.2 V battery
    refresh();
    _initialized = true;
    return true;
}

void BatteryAccessory::refresh()
{
    if (!isSupported())
        return;

    uint8_t pin = BoardConfigManager::getInstance().getConfig().getBatteryAdcPin();

    uint32_t sum = 0;
    for (uint8_t i = 0; i < 10; i++)
        sum += (uint32_t) analogRead(pin);

    _rawAdc = static_cast<uint16_t>(sum / 10u);
}

AccessorySnapshot BatteryAccessory::getSnapshot() const
{
    AccessorySnapshot snap{};
    snap.name = "battery";
    snap.type = AccessoryType::SampledSensor;
    snap.initialized = _initialized;
    snap.supported = isSupported();
    snap.rawAdc = _rawAdc;
    snap.primaryKey = "voltage_mv";
    snap.primaryValue = rawAdcToVoltageMv(_rawAdc);
    snap.secondaryKey = "percent";
    snap.secondaryValue = voltageMvToPercent(snap.primaryValue);
    return snap;
}
