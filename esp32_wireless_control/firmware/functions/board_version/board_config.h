/**
 * @file board_config.h
 * @version 0.1.0
 *
 * @section License
 * Copyright (C) 2026, Sylensky
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <cstdint>

/**
 * @brief Base class for board-specific configuration
 *
 * Defines pin mappings and hardware-specific settings that may vary
 * between different board revisions.
 */
class BoardConfig
{
  public:
    virtual ~BoardConfig() = default;

    // Stepper driver pins - RA Axis
    virtual uint8_t getAxis1Step() const = 0;
    virtual uint8_t getAxis1Dir() const = 0;
    virtual uint8_t getSpread1() const = 0;

    // Stepper driver pins - DEC Axis
    // virtual uint8_t getAxis2Step() const = 0;
    // virtual uint8_t getAxis2Dir() const = 0;
    // virtual uint8_t getSpread2() const = 0;

    // Common stepper pins
    virtual uint8_t getRaMs1() const = 0;
    virtual uint8_t getRaMs2() const = 0;
    virtual uint8_t getEn12() const = 0;

    // LED pins
    virtual uint8_t getIntervPin() const = 0;
    virtual uint8_t getStatusLed() const
    {
        return 0;
    }

    // UART pins for TMC drivers
    virtual uint8_t getAxisRx() const = 0;
    virtual uint8_t getAxisTx() const = 0;

    // Board version pin
    virtual uint8_t getBoardVersionPin() const = 0;

    // TMC driver configuration
    virtual float getTmcRSense() const = 0;
    virtual uint8_t getAxis1Addr() const = 0;
    // virtual uint8_t getAxis2Addr() const = 0;

    // Board-specific features
    virtual const char* getBoardName() const = 0;
    virtual bool hasTmcDriverSupport() const = 0;

    // NeoPixel LED hardware features
    virtual bool hasNeoPixelLeds() const
    {
        return false;
    }
    virtual uint8_t getNeoPixelPin() const
    {
        return 0;
    }
    virtual uint8_t getNeoPixelCount() const
    {
        return 0;
    }
    virtual uint32_t getNeoPixelType() const
    {
        return 0x01;
    }

    // NeoPixel LED indices
    virtual uint8_t getNeoPixelStatusIndex() const
    {
        return 0;
    }
    virtual uint8_t getNeoPixelCameraIndex() const
    {
        return 1;
    }
    virtual uint8_t getNeoPixelPowerIndex() const
    {
        return 2;
    }

    // -----------------------------------------------------------------------
    // Accessory subsystem – pin mapping and capability flags
    // 0xFF = pin not available / accessory not supported on this board.
    // -----------------------------------------------------------------------

    /** @brief GPIO output pin for laser pointer control (active HIGH). */
    virtual uint8_t getLaserPin() const
    {
        return 0xFF;
    }

    /** @brief ADC input pin for battery voltage divider (100k/100k on V_batt). */
    virtual uint8_t getBatteryAdcPin() const
    {
        return 0xFF;
    }

    /** @brief ADC input pin for ambient light LDR voltage divider (10k + GL5516). */
    virtual uint8_t getLightAdcPin() const
    {
        return 0xFF;
    }

    virtual bool hasLaser() const
    {
        return getLaserPin() != 0xFF;
    }
    virtual bool hasBatteryMonitor() const
    {
        return getBatteryAdcPin() != 0xFF;
    }
    virtual bool hasLightSensor() const
    {
        return getLightAdcPin() != 0xFF;
    }

  protected:
    BoardConfig() = default;
};

/**
 * @brief Configuration for ESP32 boards
 *
 * Original pin mapping for hardware versions used below 3.
 */
class BoardConfigV2 : public BoardConfig
{
  public:
    BoardConfigV2() = default;
    ~BoardConfigV2() override = default;

    /* clang-format off */
    // Stepper driver pins - RA Axis
    uint8_t getAxis1Step() const override { return 5; }
    uint8_t getAxis1Dir() const override { return 15; }
    uint8_t getSpread1() const override { return 4; }

    // Stepper driver pins - DEC Axis
    // uint8_t getAxis2Step() const override { return 19; }
    // uint8_t getAxis2Dir() const override { return 18; }
    // uint8_t getSpread2() const override { return 21; }

    // Common stepper pins
    uint8_t getRaMs1() const override { return 23; }
    uint8_t getRaMs2() const override { return 22; }
    uint8_t getEn12() const override { return 17; }

    // LED pins
    uint8_t getIntervPin() const override { return 25; }
    uint8_t getStatusLed() const override { return 26; }

    // UART pins for TMC drivers
    uint8_t getAxisRx() const override { return 16; }
    uint8_t getAxisTx() const override { return 19; }

    // Board version pin
    uint8_t getBoardVersionPin() const override { return 2; }

    // TMC driver configuration
    float getTmcRSense() const override { return 0.11f; }
    uint8_t getAxis1Addr() const override { return 0; }
    // uint8_t getAxis2Addr() const override { return 1; }

    // Board-specific features
    const char* getBoardName() const override { return "ESP32"; }
    bool hasTmcDriverSupport() const override { return true; }
    /* clang-format on */
};

/**
 * @brief Configuration for ESP32-S3 boards
 *
 * Updated pin mapping for ESP32-S3 hardware usually used for versions above 3.
 * Modify pin assignments here as needed for the new board layout
 */
class BoardConfigV3 : public BoardConfig
{
  public:
    BoardConfigV3() = default;
    ~BoardConfigV3() override = default;

    /* clang-format off */
    // Stepper driver pins - RA Axis
    uint8_t getAxis1Step() const override { return 5; }
    uint8_t getAxis1Dir() const override { return 4; }
    uint8_t getSpread1() const override { return 17; }

    // Stepper driver pins - DEC Axis
    // uint8_t getAxis2Step() const override { return 19; }
    // uint8_t getAxis2Dir() const override { return 18; }
    // uint8_t getSpread2() const override { return 21; }

    // Common stepper pins
    uint8_t getRaMs1() const override { return 15; }
    uint8_t getRaMs2() const override { return 7; }
    uint8_t getEn12() const override { return 18; }

    // LED pins
    uint8_t getIntervPin() const override { return 21; } // FIXME: is not a LED
    // getStatusLed() not needed - uses NeoPixels on GPIO 45

    // UART pins for TMC drivers
    uint8_t getAxisRx() const override { return 6; }
    uint8_t getAxisTx() const override { return 17; }

    // Board version pin
    uint8_t getBoardVersionPin() const override { return 2; }

    // TMC driver configuration
    float getTmcRSense() const override { return 0.11f; }
    uint8_t getAxis1Addr() const override { return 0; }
    // uint8_t getAxis2Addr() const override { return 1; }

    // Board-specific features
    const char* getBoardName() const override { return "ESP32S3"; }
    bool hasTmcDriverSupport() const override { return true; }

    // NeoPixel LED hardware - 3x WS2812 on GPIO 45
    bool hasNeoPixelLeds() const override { return true; }
    uint8_t getNeoPixelPin() const override { return 45; }
    uint8_t getNeoPixelCount() const override { return 3; }
    uint32_t getNeoPixelType() const override { return 0x52; } // NEO_GRB + NEO_KHZ800

    // NeoPixel indices: [0]=Camera, [1]=Status, [2]=Power
    uint8_t getNeoPixelStatusIndex() const override { return 1; }
    uint8_t getNeoPixelCameraIndex() const override { return 0; }
    uint8_t getNeoPixelPowerIndex() const override { return 2; }

    // Accessory pins (V3 / ESP32-S3 hardware only)
    // Laser:   IO46 – GPIO output, active HIGH
    // Battery: IO9  – ADC input, 100k/100k divider, V_adc = V_batt/2
    // Light:   IO10 – ADC input, 10k pull-up to 3.3V + GL5516 to GND
    uint8_t getLaserPin()      const override { return 46; }
    uint8_t getBatteryAdcPin() const override { return 9;  }
    uint8_t getLightAdcPin()   const override { return 10; }
    /* clang-format on */
};

/**
 * @brief Singleton manager for board configuration
 *
 * Automatically selects the correct configuration based on detected hardware version.
 */
class BoardConfigManager
{
  public:
    /**
     * @brief Get the singleton instance
     */
    static BoardConfigManager& getInstance();

    /**
     * @brief Initialize the config manager with hardware detection
     * Automatically initializes BoardVersion if not already done.
     * @param versionPin GPIO pin for version detection
     */
    void init(uint8_t versionPin);

    /**
     * @brief Get the active board configuration
     */
    const BoardConfig& getConfig() const;

    /**
     * @brief Check if configuration is initialized
     */
    bool isInitialized() const
    {
        return _initialized;
    }

  private:
    BoardConfigManager();
    ~BoardConfigManager();
    BoardConfigManager(const BoardConfigManager&) = delete;
    BoardConfigManager& operator=(const BoardConfigManager&) = delete;

    BoardConfig* _config;
    bool _initialized;
};

/**
 * @brief Convenience accessor for the active board configuration
 */
inline const BoardConfig& boardCfg()
{
    return BoardConfigManager::getInstance().getConfig();
}

#endif // BOARD_CONFIG_H
