/**
 * @file laser_accessory.cpp
 * @brief LaserAccessory implementation.
 */

#include "functions/accessories/laser_accessory.h"
#include "functions/board_version/board_config.h"
#include "functions/board_version/board_version.h"

#include <Arduino.h>

LaserAccessory& LaserAccessory::getInstance()
{
    static LaserAccessory instance;
    return instance;
}

bool LaserAccessory::isSupported() const
{
    return BoardConfigManager::getInstance().getConfig().hasLaser();
}

bool LaserAccessory::init()
{
    if (!isSupported())
        return false;

    uint8_t pin = BoardConfigManager::getInstance().getConfig().getLaserPin();
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    _state = false;
    _initialized = true;
    return true;
}

void LaserAccessory::setState(bool on)
{
    if (!_initialized)
        return;

    uint8_t pin = BoardConfigManager::getInstance().getConfig().getLaserPin();
    digitalWrite(pin, on ? HIGH : LOW);
    _state = on;
}

AccessorySnapshot LaserAccessory::getSnapshot() const
{
    AccessorySnapshot snap{};
    snap.name = "laser";
    snap.type = AccessoryType::BinaryActuator;
    snap.initialized = _initialized;
    snap.supported = isSupported();
    snap.state = _state;
    return snap;
}
