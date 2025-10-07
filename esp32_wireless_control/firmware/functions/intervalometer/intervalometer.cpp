#include "intervalometer.h"
#include "configs/config.h"
#include "eeprom_manager.h"
#include "uart.h"


#include "long_exposure_movie.h"
#include "long_exposure_still.h"
#include "timelapse.h"
#include "timelapse_pan.h"

Intervalometer::Intervalometer(uint8_t triggerPinArg)
    : triggerPin(triggerPinArg), currentMode(Mode::LongExposureStill),
      currentErrorMessage(ERR_MSG_NONE), activeMode(nullptr)
{
    currentSettings = Settings();
}

Intervalometer::~Intervalometer()
{
    if (activeMode != nullptr)
    {
        abortCapture();
        vTaskDelay(pdMS_TO_TICKS(200));
        delete activeMode;
        activeMode = nullptr;
    }
}

void Intervalometer::startCapture()
{
    // Don't start if already active
    if (activeMode != nullptr && activeMode->isActive())
    {
        print_out("ERROR: Capture already active");
        return;
    }

    // Clean up any previous instance
    if (activeMode != nullptr)
    {
        delete activeMode;
        activeMode = nullptr;
    }

    // Create new mode instance
    activeMode = createModeInstance();
    if (activeMode == nullptr)
    {
        print_out("ERROR: Failed to create mode instance");
        currentErrorMessage = ERR_MSG_NONE; // Could add specific error
        return;
    }

    // Start the capture
    if (!activeMode->startCapture())
    {
        print_out("ERROR: Failed to start capture");
        delete activeMode;
        activeMode = nullptr;
        return;
    }

    print_out("Capture started successfully");
}

void Intervalometer::abortCapture()
{
    if (activeMode != nullptr)
    {
        activeMode->abortCapture();
    }
}

bool Intervalometer::isActive() const
{
    return (activeMode != nullptr && activeMode->isActive());
}

void Intervalometer::cleanup()
{
    if (activeMode != nullptr && !activeMode->isActive())
    {
        delete activeMode;
        activeMode = nullptr;
    }
}

IntervalometerMode* Intervalometer::createModeInstance()
{
    // Convert settings struct (add mode to it)
    IntervalometerMode::Settings modeSettings = currentSettings;

    switch (currentMode)
    {
        case Mode::LongExposureStill:
            print_out("Creating LongExposureStill mode");
            return new LongExposureStill(triggerPin, modeSettings);

        case Mode::LongExposureMovie:
            print_out("Creating LongExposureMovie mode");
            return new LongExposureMovie(triggerPin, modeSettings);

        case Mode::Timelapse:
            print_out("Creating Timelapse mode");
            return new Timelapse(triggerPin, modeSettings);

        case Mode::TimelapsePan:
            print_out("Creating TimelapsePan mode");
            return new TimelapsePan(triggerPin, modeSettings);

        default:
            print_out("ERROR: Unknown mode: %d", static_cast<int>(currentMode));
            return nullptr;
    }
}

uint16_t Intervalometer::getCurrentExposure() const
{
    return activeMode ? activeMode->getCurrentExposure() : 0;
}

uint16_t Intervalometer::getExposuresTaken() const
{
    return activeMode ? activeMode->getExposuresTaken() : 0;
}

TickType_t Intervalometer::getStartCaptureTickCount() const
{
    return activeMode ? activeMode->getStartCaptureTickCount() : 0;
}

TickType_t Intervalometer::getCaptureDurationTickCount() const
{
    return activeMode ? activeMode->getCaptureDurationTickCount() : 0;
}

IntervalometerMode::State Intervalometer::getState() const
{
    return activeMode ? activeMode->getState() : IntervalometerMode::State::Inactive;
}

void Intervalometer::setSettings(const Settings& settings)
{
    currentSettings = settings;
}

void Intervalometer::saveSettingsToPreset(uint8_t preset)
{
    if (preset >= 10)
    {
        print_out("ERROR: Invalid preset number: %d", preset);
        return;
    }

    // Sync mode field before saving
    currentSettings.mode = static_cast<uint8_t>(currentMode);
    presets[preset] = currentSettings;
    savePresetsToEEPROM();
    print_out("Settings saved to preset %d", preset);
}

void Intervalometer::readSettingsFromPreset(uint8_t preset)
{
    if (preset >= 10)
    {
        print_out("ERROR: Invalid preset number: %d", preset);
        return;
    }

    currentSettings = presets[preset];
    // Restore mode from settings
    currentMode = static_cast<Mode>(currentSettings.mode);
    print_out("Settings loaded from preset %d (mode: %d)", preset, currentSettings.mode);
}

void Intervalometer::savePresetsToEEPROM()
{
#if DEBUG == 1
    print_out("Writing presets to EEPROM...");
#endif
    int written = EepromManager::writePresets(PRESETS_EEPROM_START_LOCATION, presets);
    print_out("Written bytes: %d", written);
}

void Intervalometer::readPresetsFromEEPROM()
{
#if DEBUG == 1
    print_out("Reading presets from EEPROM...");
#endif
    int read = EepromManager::readPresets(PRESETS_EEPROM_START_LOCATION, presets);
    print_out("Read bytes: %d", read);
}
