#include "neopixel_manager.h"
#include "functions/board_version/board_config.h"

NeoPixelManager::NeoPixelManager() : _pixels(nullptr), _initialized(false), _mutex(nullptr)
{
    _mutex = xSemaphoreCreateMutex();

    // FIXME: add max led count
    for (int i = 0; i < 8; i++)
    {
        _defaultColors[i].r = 255;
        _defaultColors[i].g = 255;
        _defaultColors[i].b = 255;
    }
}

NeoPixelManager::~NeoPixelManager()
{
    if (_pixels != nullptr)
    {
        delete _pixels;
        _pixels = nullptr;
    }

    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
}

NeoPixelManager& NeoPixelManager::getInstance()
{
    static NeoPixelManager instance;
    return instance;
}

bool NeoPixelManager::init()
{
    if (_initialized)
        return true;

    const BoardConfig& cfg = BoardConfigManager::getInstance().getConfig();

    if (!cfg.hasNeoPixelLeds())
    {
        _initialized = false;
        return false;
    }

    _pixels = new Adafruit_NeoPixel(cfg.getNeoPixelCount(), cfg.getNeoPixelPin(),
                                    cfg.getNeoPixelType() + NEO_KHZ800);

    _pixels->begin();
    _pixels->clear();
    _pixels->show();

    _initialized = true;
    return true;
}

bool NeoPixelManager::isAvailable() const
{
    return _initialized && (_pixels != nullptr);
}

void NeoPixelManager::setPixelColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (!isAvailable())
        return;

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        _pixels->setPixelColor(index, _pixels->Color(r, g, b));
        xSemaphoreGive(_mutex);
    }
}

void NeoPixelManager::setDefaultColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    _defaultColors[index].r = r;
    _defaultColors[index].g = g;
    _defaultColors[index].b = b;
}

void NeoPixelManager::turnOnPixel(uint8_t index)
{
    if (!isAvailable() || index >= 8)
        return;

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        uint8_t r = _defaultColors[index].r;
        uint8_t g = _defaultColors[index].g;
        uint8_t b = _defaultColors[index].b;
        _pixels->setPixelColor(index, _pixels->Color(r, g, b));
        xSemaphoreGive(_mutex);
    }
}

void NeoPixelManager::clearPixel(uint8_t index)
{
    if (!isAvailable())
        return;

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        _pixels->setPixelColor(index, 0);
        xSemaphoreGive(_mutex);
    }
}

void NeoPixelManager::show()
{
    if (!isAvailable())
        return;

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        _pixels->show();
        xSemaphoreGive(_mutex);
    }
}

void NeoPixelManager::updatePixel(uint8_t index, bool turnOn)
{
    if (!isAvailable() || index >= 8)
        return;

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        if (turnOn)
        {
            uint8_t r = _defaultColors[index].r;
            uint8_t g = _defaultColors[index].g;
            uint8_t b = _defaultColors[index].b;
            _pixels->setPixelColor(index, _pixels->Color(r, g, b));
        }
        else
            _pixels->setPixelColor(index, 0);

        _pixels->show();
        xSemaphoreGive(_mutex);
    }
}

void NeoPixelManager::updateAllPixels(const uint8_t* states, uint8_t count)
{
    if (!isAvailable())
        return;

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        for (uint8_t i = 0; i < count; i++)
        {
            if (states[i] == HIGH)
            {
                _pixels->setPixelColor(i, _pixels->Color(_defaultColors[i].r, _defaultColors[i].g,
                                                         _defaultColors[i].b));
            }
            else
                _pixels->setPixelColor(i, 0);
        }

        _pixels->show();
        xSemaphoreGive(_mutex);
    }
}

void NeoPixelManager::clearAll()
{
    if (!isAvailable())
        return;

    _pixels->clear();
    _pixels->show();
}
