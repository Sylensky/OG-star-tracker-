/**
 * @file board_version.h
 * @version 0.1.0
 *
 * @section License
 * Copyright (C) 2026, Sylensky
 */

#ifndef BOARD_VERSION_H
#define BOARD_VERSION_H

#include <Arduino.h>
#include <cstdint>

/**
 * @brief Hardware board version enumeration
 */
enum class HardwareVersion : uint8_t
{
    Unknown = 0,
    V2_X,
    V3,
    MaxVersions
};

/**
 * @brief Board version detection using voltage divider
 *
 * Reads ADC value from a voltage divider circuit on IO2 and maps it
 * to a specific hardware version.
 */
class BoardVersion
{
  public:
    /**
     * @brief Get the singleton instance
     */
    static BoardVersion& getInstance();

    /**
     * @brief Initialize the version detection system
     * @param pin GPIO pin for ADC reading
     */
    void init(uint8_t pin);

    /**
     * @brief Detect and return the current hardware version
     * @return Detected hardware version
     */
    HardwareVersion detect();

    /**
     * @brief Get the cached hardware version
     * @return Cached hardware version (Unknown if not yet detected)
     */
    HardwareVersion getVersion() const;

    /**
     * @brief Get version as a string
     * @return Version string (e.g., "v2.2")
     */
    const char* getVersionString() const;

    /**
     * @brief Get the raw ADC value from last reading
     * @return ADC value (0-4095 for 12-bit ADC)
     */
    uint16_t getRawADC() const;

    /**
     * @brief Get the voltage from last reading
     * @return Voltage in millivolts
     */
    uint16_t getVoltage() const;

  private:
    BoardVersion();
    BoardVersion(const BoardVersion&) = delete;
    BoardVersion& operator=(const BoardVersion&) = delete;

    /**
     * @brief Map ADC value to hardware version
     * @param adcValue Raw ADC reading
     * @return Corresponding hardware version
     */
    HardwareVersion mapADCToVersion(uint16_t adcValue);

    /**
     * @brief Read ADC with averaging to reduce noise
     * @param samples Number of samples to average (default 10)
     * @return Averaged ADC value
     */
    uint16_t readADCAveraged(uint8_t samples = 10);

    uint8_t _pin;
    HardwareVersion _version;
    uint16_t _lastADC;
    uint16_t _lastVoltage;
    bool _initialized;

    // ADC to version mapping thresholds (in ADC counts, 12-bit: 0-4095)
    struct VersionThreshold
    {
        uint16_t minADC;
        uint16_t maxADC;
        HardwareVersion version;
    };

    static const VersionThreshold _versionMap[];
    static const uint8_t _versionMapSize;
};

#endif // BOARD_VERSION_H
