/**
 * @file laser_accessory.h
 * @brief Laser pointer GPIO accessory (Epic 0.5.2).
 *
 * Concrete BinaryActuator that controls a laser pointer via a single GPIO
 * output pin (IO46 on V3 boards). Pin is sourced from BoardConfig so no
 * hardware detail leaks into this class.
 *
 * Usage:
 *   // Registration (firmware.ino, after BoardConfig init):
 *   AccessoryRegistry::getInstance().registerAccessory(&LaserAccessory::getInstance());
 *
 *   // Control (REST handler / console command):
 *   LaserAccessory::getInstance().setState(true);
 *   bool on = LaserAccessory::getInstance().getState();
 */

#ifndef LASER_ACCESSORY_H
#define LASER_ACCESSORY_H

#include "functions/accessories/accessory.h"

class LaserAccessory : public BinaryActuator
{
  public:
    static LaserAccessory& getInstance();

    const char* getName() const override
    {
        return "laser";
    }

    bool isSupported() const override;

    /**
     * @brief Configure the laser GPIO pin as output and set it LOW (off).
     * @return true if supported and pin configured; false if unsupported.
     */
    bool init() override;

    bool isInitialized() const override
    {
        return _initialized;
    }

    /**
     * @brief Drive the laser pin HIGH (on=true) or LOW (on=false).
     * No-op if not initialized.
     */
    void setState(bool on) override;

    bool getState() const override
    {
        return _state;
    }

    AccessorySnapshot getSnapshot() const override;

  private:
    LaserAccessory() = default;
    LaserAccessory(const LaserAccessory&) = delete;
    LaserAccessory& operator=(const LaserAccessory&) = delete;

    bool _initialized = false;
    bool _state = false;
};

#endif // LASER_ACCESSORY_H
