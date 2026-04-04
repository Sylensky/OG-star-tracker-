#include "led.h"
#include "configs/config.h"
#include "functions/board_version/board_config.h"
#include "neopixel_manager.h"
#include "uart.h"

// LED Manager implementation
LED::LED()
{
}

LED& LED::getInstance()
{
    static LED instance;
    return instance;
}

void LED::initAll()
{
    const BoardConfig& cfg = BoardConfigManager::getInstance().getConfig();

    // Initialize NeoPixel manager if board has NeoPixels
    bool hasNeoPixels = NeoPixelManager::getInstance().init();

    if (hasNeoPixels)
    {
        status.setNeoPixelMode(cfg.getNeoPixelStatusIndex(), 0, 255, 0);
        status._initialized = true;
        camera.setNeoPixelMode(cfg.getNeoPixelCameraIndex(), 255, 0, 0);
        camera._initialized = true;
        power.setNeoPixelMode(cfg.getNeoPixelPowerIndex(), 0, 0, 255);
        power._initialized = true;
    }
    else
    {
        status.initPWM(cfg.getStatusLed(), LEDC_FREQ, LEDC_RESOLUTION);
        status.setBrightness(STATUS_LED_BRIGHTNESS);
        camera._initialized = false;
        power._initialized = false;
    }

    // FIXME: Trigger LED is always a regular GPIO
    trigger.init(cfg.getIntervPin());
}

void LED::updateAll()
{
    const BoardConfig& cfg = BoardConfigManager::getInstance().getConfig();

    if (!cfg.hasNeoPixelLeds())
        return;

    NeoPixelManager& npm = NeoPixelManager::getInstance();
    if (!npm.isAvailable())
        return;

    // FIXME: add max led count
    uint8_t states[3] = {0};
    states[status._neoPixelIndex] = status._state;
    states[camera._neoPixelIndex] = camera._state;
    states[power._neoPixelIndex] = power._state;
    npm.updateAllPixels(states, 3);
}

// LEDBase implementation
LEDBase::LEDBase()
    : _pin(0), _state(LOW), _brightness(255), _initialized(false), _pwmEnabled(false), _pwmFreq(0),
      _pwmResolution(0), _isNeoPixel(false), _neoPixelIndex(0)
{
}

void LEDBase::setNeoPixelMode(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    _isNeoPixel = true;
    _neoPixelIndex = index;

    // Set default color in NeoPixelManager
    NeoPixelManager::getInstance().setDefaultColor(index, r, g, b);
}

void LEDBase::init(uint8_t pin)
{
    _pin = pin;
    _state = LOW;
    _brightness = 255;
    _pwmEnabled = false;

    if (!_isNeoPixel)
    {
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, _state);
    }

    _initialized = true;
}

bool LEDBase::initPWM(uint8_t pin, uint32_t freq, uint8_t resolution)
{
    _pin = pin;
    _state = LOW;
    _brightness = 255;
    _pwmFreq = freq;
    _pwmResolution = resolution;

    bool result = ledcAttach(_pin, freq, resolution);
    if (!result)
    {
        Serial.println("Failed to attach LEDC to LED pin");
        _initialized = false;
        _pwmEnabled = false;
        return false;
    }

    _pwmEnabled = true;
    _initialized = true;
    ledcWrite(_pin, 0); // Start with LED off
    return true;
}

void LEDBase::toggle()
{
    if (!_initialized)
        return;

    _state = !_state;

    // Priority: NeoPixel > PWM > GPIO
    if (_isNeoPixel)
    {
        // NeoPixel handled by LED::updateAll()
    }
    else if (_pwmEnabled)
    {
        ledcWrite(_pin, _state ? _brightness : 0);
    }
    else
    {
        digitalWrite(_pin, _state);
    }
}

void LEDBase::on()
{
    if (!_initialized)
        return;

    _state = HIGH;

    // Priority: NeoPixel > PWM > GPIO
    if (_isNeoPixel)
    {
        // NeoPixel handled by LED::updateAll()
    }
    else if (_pwmEnabled)
    {
        ledcWrite(_pin, _brightness);
    }
    else
    {
        digitalWrite(_pin, _state);
    }
}

void LEDBase::off()
{
    if (!_initialized)
        return;
    _state = LOW;

    // Priority: NeoPixel > PWM > GPIO
    if (_isNeoPixel)
    {
        // NeoPixel handled by LED::updateAll()
    }
    else if (_pwmEnabled)
    {
        ledcWrite(_pin, 0);
    }
    else
    {
        digitalWrite(_pin, _state);
    }
}

void LEDBase::set(uint8_t state)
{
    if (!_initialized)
        return;

    _state = state;

    // Priority: NeoPixel > PWM > GPIO
    if (_isNeoPixel)
    {
        // NeoPixel handled by LED::updateAll()
    }
    else if (_pwmEnabled)
    {
        ledcWrite(_pin, state ? _brightness : 0);
    }
    else
    {
        digitalWrite(_pin, _state);
    }
}

void LEDBase::setBrightness(uint8_t brightness)
{
    if (!_initialized)
        return;

    _brightness = brightness;

    // Only apply brightness if PWM is enabled and LED is on
    if (_pwmEnabled && _state == HIGH)
    {
        ledcWrite(_pin, _brightness);
    }
}

void LEDBase::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    if (!_initialized || !_isNeoPixel)
        return;

    NeoPixelManager& npm = NeoPixelManager::getInstance();
    if (!npm.isAvailable())
        return;

    // Update the default color and apply immediately
    npm.setDefaultColor(_neoPixelIndex, r, g, b);
    npm.setPixelColor(_neoPixelIndex, r, g, b);
    npm.show();
    _state = (r > 0 || g > 0 || b > 0) ? HIGH : LOW;
}

uint8_t LEDBase::getState() const
{
    return _state;
}

uint8_t LEDBase::getBrightness() const
{
    return _brightness;
}

bool LEDBase::isInitialized() const
{
    return _initialized;
}

bool LEDBase::isPWMEnabled() const
{
    return _pwmEnabled;
}
