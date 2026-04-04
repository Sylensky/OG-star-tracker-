#include "board_version.h"
#include "configs/config.h"

// ADC to version mapping table
// ADC values are for 12-bit resolution (0-4095), 3.3V reference
const BoardVersion::VersionThreshold BoardVersion::_versionMap[] = {
    // Format: {minADC, maxADC, version}
    {0, 1900, HardwareVersion::V2_X},   // < 1.53V (~1900) = baseline v2.x with 100k/100k
    {1901, 4095, HardwareVersion::V3} // > 1.53V = v3 with modified resistor values (ESP32-S3)
};

const uint8_t BoardVersion::_versionMapSize =
    sizeof(BoardVersion::_versionMap) / sizeof(BoardVersion::VersionThreshold);

BoardVersion::BoardVersion()
    : _pin(BOARD_VERSION_PIN), _version(HardwareVersion::Unknown), _lastADC(0), _lastVoltage(0),
      _initialized(false)
{
}

BoardVersion& BoardVersion::getInstance()
{
    static BoardVersion instance;
    return instance;
}

void BoardVersion::init(uint8_t pin)
{
    _pin = pin;
    analogSetAttenuation(ADC_11db); // Full range: 0-3.3V
    analogReadResolution(12);       // 12-bit resolution (0-4095)
    _initialized = true;
    detect();
}

HardwareVersion BoardVersion::detect()
{
    if (!_initialized)
        return HardwareVersion::Unknown;

    // Read ADC with averaging to reduce noise
    _lastADC = readADCAveraged(10);

    // Convert ADC to voltage (in mV)
    // ADC range: 0-4095 maps to 0-3300mV
    _lastVoltage = (_lastADC * 3300) / 4095;

    // Map ADC value to version
    _version = mapADCToVersion(_lastADC);

    return _version;
}

HardwareVersion BoardVersion::mapADCToVersion(uint16_t adcValue)
{
    for (uint8_t i = 0; i < _versionMapSize; i++)
    {
        if (adcValue >= _versionMap[i].minADC && adcValue <= _versionMap[i].maxADC)
        {
            return _versionMap[i].version;
        }
    }

    return HardwareVersion::Unknown;
}

uint16_t BoardVersion::readADCAveraged(uint8_t samples)
{
    if (!_initialized)
    {
        return 0;
    }

    uint32_t sum = 0;

    // Take multiple samples and average
    for (uint8_t i = 0; i < samples; i++)
    {
        sum += analogRead(_pin);
        delayMicroseconds(100); // Small delay between samples
    }

    return sum / samples;
}

HardwareVersion BoardVersion::getVersion() const
{
    return _version;
}

const char* BoardVersion::getVersionString() const
{
    switch (_version)
    {
        case HardwareVersion::V2_X:
            return "v2.x";
        case HardwareVersion::V3:
            return "v3";
        case HardwareVersion::Unknown:
        default:
            return "Unknown";
    }
}

uint16_t BoardVersion::getRawADC() const
{
    return _lastADC;
}

uint16_t BoardVersion::getVoltage() const
{
    return _lastVoltage;
}
