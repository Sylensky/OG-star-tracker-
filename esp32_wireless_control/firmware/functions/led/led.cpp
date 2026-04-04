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
LEDBase::LEDBase() : _pin(0), _state(LOW), _initialized(false)
{
}

void LEDBase::init(uint8_t pin)
{
    _pin = pin;
    _state = LOW;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, _state);
    _initialized = true;
}

void LEDBase::toggle()
{
    if (!_initialized)
        return;

    _state = !_state;
    digitalWrite(_pin, _state);
}

void LEDBase::on()
{
    if (!_initialized)
        return;

    _state = HIGH;
    digitalWrite(_pin, _state);
}

void LEDBase::off()
{
    if (!_initialized)
        return;

    _state = LOW;
    digitalWrite(_pin, _state);
}

void LEDBase::set(uint8_t state)
{
    if (!_initialized)
        return;

    _state = state;
    digitalWrite(_pin, _state);
}

uint8_t LEDBase::getState() const
{
    return _state;
}

bool LEDBase::isInitialized() const
{
    return _initialized;
}
