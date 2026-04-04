
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "board_config.h"
#include "board_version.h"
#include "uart.h"

BoardConfigManager::BoardConfigManager() : _config(nullptr), _initialized(false)
{
}

BoardConfigManager::~BoardConfigManager()
{
    if (_config != nullptr)
    {
        delete _config;
        _config = nullptr;
    }
}

BoardConfigManager& BoardConfigManager::getInstance()
{
    static BoardConfigManager instance;
    return instance;
}

void BoardConfigManager::init(uint8_t versionPin)
{
    if (_initialized)
        return;

    BoardVersion::getInstance().init(versionPin);
    HardwareVersion hwVersion = BoardVersion::getInstance().getVersion();

    switch (hwVersion)
    {
        case HardwareVersion::V2_X:
            _config = new BoardConfigV2();
            _initialized = true;
            break;

        case HardwareVersion::V3:
            _config = new BoardConfigV3();
            _initialized = true;
            break;

        case HardwareVersion::Unknown:
        case HardwareVersion::MaxVersions:
        default:
            // SAFETY: Do NOT use fallback configuration for unknown hardware!
            // Using wrong pin configuration could damage the board.
            _config = nullptr;
            _initialized = false;
            print_out(
                "ERROR: Unknown hardware version detected - cannot initialize board configuration");
            while (true)
            {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            break;
    }
}

const BoardConfig& BoardConfigManager::getConfig() const
{
    if (!_initialized || _config == nullptr)
    {
        // SAFETY: Should never happen, but if it does, halt instead of returning invalid config
        // Cannot use print_out here as UART queue may not be initialized yet
        while (true)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    return *_config;
}
