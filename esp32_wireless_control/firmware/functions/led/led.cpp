#include "led.h"

// LED Manager implementation
LED::LED()
{
}

LED& LED::getInstance()
{
    static LED instance;
    return instance;
}

// LEDBase implementation
LEDBase::LEDBase()
    : _pin(0), _state(LOW), _brightness(255), _initialized(false), _pwmEnabled(false), _pwmFreq(0),
      _pwmResolution(0)
{
}

void LEDBase::init(uint8_t pin)
{
    _pin = pin;
    _state = LOW;
    _brightness = 255;
    _pwmEnabled = false;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, _state);
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

    if (_pwmEnabled)
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

    if (_pwmEnabled)
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

    if (_pwmEnabled)
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

    if (_pwmEnabled)
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
