#pragma once

#include <cstdint>

#include "error.h"
#include "intervalometer_mode.h"

/**
 * @brief Main Intervalometer manager class
 *
 * This class acts as a factory and manager for different intervalometer modes.
 * It creates the appropriate mode instance based on settings and manages its lifecycle.
 * Maintains backward compatibility with the existing API for firmware integration.
 */
class Intervalometer
{
  public:
    enum class Mode : uint8_t
    {
        LongExposureStill,
        LongExposureMovie,
        Timelapse,
        TimelapsePan,
        MaxModes
    };

    using Settings = IntervalometerMode::Settings;
    using State = IntervalometerMode::State;

    explicit Intervalometer(uint8_t triggerPinArg);
    ~Intervalometer();

    /**
     * @brief Start capture with current settings
     * Creates the appropriate mode instance and starts its task
     */
    void startCapture();

    /**
     * @brief Abort current capture
     */
    void abortCapture();

    /**
     * @brief Check if capture is active
     */
    bool isActive() const;

    /**
     * @brief Cleanup completed mode instance
     */
    void cleanup();

    // EEPROM preset management
    void readPresetsFromEEPROM();
    void saveSettingsToPreset(uint8_t preset);
    void readSettingsFromPreset(uint8_t preset);

    // Getters
    uint16_t getCurrentExposure() const;
    uint16_t getExposuresTaken() const;
    TickType_t getStartCaptureTickCount() const;
    TickType_t getCaptureDurationTickCount() const;
    IntervalometerMode::State getState() const;
    const Settings& getSettings() const
    {
        return currentSettings;
    }
    ErrorMessage getErrorMessage() const
    {
        return currentErrorMessage;
    }

    // Setters
    void setSettings(const Settings& settings);
    void setErrorMessage(ErrorMessage error)
    {
        currentErrorMessage = error;
    }

    /**
     * @brief Get current mode
     */
    Mode getMode() const
    {
        return currentMode;
    }

    /**
     * @brief Set current mode
     */
    void setMode(Mode mode)
    {
        currentMode = mode;
    }

  private:
    /**
     * @brief Create mode instance based on current settings
     */
    IntervalometerMode* createModeInstance();

    /**
     * @brief Save presets to EEPROM
     */
    void savePresetsToEEPROM();

    // Configuration
    uint8_t triggerPin;
    Mode currentMode;
    Settings currentSettings;
    Settings presets[10];
    ErrorMessage currentErrorMessage;

    // Current active mode instance (null when inactive)
    IntervalometerMode* activeMode;
};
