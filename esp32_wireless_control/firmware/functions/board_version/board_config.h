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
    virtual uint8_t getStatusLed() const = 0;

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
    const char* getBoardName() const override { return "ESP32S3"; }
    bool hasTmcDriverSupport() const override { return true; }
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
