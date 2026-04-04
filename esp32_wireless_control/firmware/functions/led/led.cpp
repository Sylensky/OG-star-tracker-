#include "led.h"
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
        status.init(cfg.getStatusLed());
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
    : _pin(0), _state(LOW), _initialized(false), _isNeoPixel(false), _neoPixelIndex(0)
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

    if (!_isNeoPixel)
    {
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, _state);
    }

    _initialized = true;
}

void LEDBase::toggle()
{
    if (!_initialized)
        return;

    _state = !_state;

    if (!_isNeoPixel)
        digitalWrite(_pin, _state);
}

void LEDBase::on()
{
    if (!_initialized)
        return;

    _state = HIGH;

    if (!_isNeoPixel)
        digitalWrite(_pin, _state);
}

void LEDBase::off()
{
    if (!_initialized)
        return;
    _state = LOW;

    if (!_isNeoPixel)
        digitalWrite(_pin, _state);
}

void LEDBase::set(uint8_t state)
{
    if (!_initialized)
        return;

    _state = state;

    if (!_isNeoPixel)
        digitalWrite(_pin, _state);
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

bool LEDBase::isInitialized() const
{
    return _initialized;
}
