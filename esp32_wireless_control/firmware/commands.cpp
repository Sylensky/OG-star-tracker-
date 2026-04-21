#include <freertos/FreeRTOS.h>

#include <axis.h>
#include <commands.h>
#include <configs/config.h>
#include <functions/accessories/accessory_registry.h>
#include <functions/accessories/battery_accessory.h>
#include <functions/accessories/laser_accessory.h>
#include <functions/accessories/light_accessory.h>
#include <functions/board_version/board_config.h>
#include <functions/board_version/board_version.h>
#include <functions/led/led.h>
#include <functions/led/neopixel_manager.h>
#include <uart.h>

SerialTerminal* _term;

extern QueueHandle_t msgq;

enum task_names : uint8_t
{
    UART_TASK,
    CONSOLE_TASK,
    INTERVALOMETER_TASK,
    WEBSERVER_TASK,
};

static void cmdHelp()
{
    // Print usage
    print_out_tbl(CMD_HELP_TITLE);
    print_out_tbl(CMD_HELP_HELP);
    print_out_tbl(CMD_HELP_VERSION);
    print_out_tbl(CMD_HELP_STACK);
    print_out_tbl(CMD_HELP_HEAP);
    print_out_tbl(CMD_HELP_RESET);
    print_out_tbl(CMD_GOTO_TARGET_RA);
    print_out_tbl(CMD_HELP_PAN);
    print_out_tbl(CMD_HELP_LED);
    print_out_tbl(CMD_HELP_ACCESSORY);
    print_out_tbl(CMD_HELP_LASER);
    print_out_tbl(CMD_HELP_BATTERY);
    print_out_tbl(CMD_HELP_LIGHT);
}

static void cmdVersion()
{
    print_out("Software Version: %s", BUILD_VERSION);
    print_out("Build Date: %s %s", __DATE__, __TIME__);
    print_out("Hardware Version: %s", BoardVersion::getInstance().getVersionString());
    print_out("Board Name: %s", BoardConfigManager::getInstance().getConfig().getBoardName());
    print_out("ADC Value: %d", BoardVersion::getInstance().getRawADC());
    print_out("Voltage: %d mV", BoardVersion::getInstance().getVoltage());
}

static uint16_t get_stack_high_water(const char* task_name)
{
    TaskHandle_t task = xTaskGetHandle(task_name);
    uint16_t stack_size = 0;

    if (task == NULL)
        print_out_tbl(TSK_NOT_AVAIL);
    else
    {
        configASSERT(task);
        stack_size = uxTaskGetStackHighWaterMark(task);
    }
    return stack_size;
}

static void cmdStackAvailable()
{
    // Print available stack
    int argIndex = 0;
    char* arg;

    // Print arguments
    for (argIndex = 0; argIndex < 8; argIndex++)
    {
        arg = _term->getNext();
        if (arg != NULL)
        {
            switch (atoi(arg))
            {
                case UART_TASK:
                    print_out_tbl(CMD_STACK_HIGHWATER_UART);
                    print_out("%d Bytes", get_stack_high_water("uart"));
                    break;
                case CONSOLE_TASK:
                    print_out_tbl(CMD_STACK_HIGHWATER_CONSOLE);
                    print_out("%d Bytes", get_stack_high_water("console"));
                    break;
                case INTERVALOMETER_TASK:
                    print_out_tbl(CMD_STACK_HIGHWATER_INTERVALOMETER);
                    print_out("%d Bytes", get_stack_high_water("intervalometer"));
                    break;
                case WEBSERVER_TASK:
                    print_out_tbl(CMD_STACK_HIGHWATER_WEBSERVER);
                    print_out("%d Bytes", get_stack_high_water("webserver"));
                    break;
                default:
                    print_out("Task %d not found", atoi(arg));
                    break;
            }
        }
        else
        {
            break;
        }
    }
}

static void cmdHeapAvailable()
{
    // Print heap usage
    char* arg;

    arg = _term->getNext();

    if (arg != NULL)
    {
        if (strcmp(arg, "all") == 0)
        {
            print_out_tbl(CMD_HEAP_MINIMUM_EVER_FREE);
            print_out("%d", ESP.getMinFreeHeap());
        }
        else
        {
            print_out_tbl(CMD_UNKNOWN_ARGUMENT);
            print_out("%s", arg);
            print_out_tbl(CMD_HEAP_AVAILABLE_ARGS);
        }
    }
    else
    {
        print_out_tbl(CMD_HEAP_FREE);
        print_out("%d", ESP.getFreeHeap());
    }
}

/**
 * Software reset
 */
extern void(systemShutdown)(void);
static void cmdReset()
{
    // Reset controller
    systemShutdown();
}

static Position parsePositionFromArgs(SerialTerminal* term)
{
    int degrees = 0, minutes = 0;
    float seconds = 0.0f;

    const char* degreesStr = term->getNext();
    if (degreesStr)
        degrees = atoi(degreesStr);
    else
    {
        print_out("Error: Missing degrees for position.");
        return Position();
    }

    const char* minutesStr = term->getNext();
    if (minutesStr)
        minutes = atoi(minutesStr);
    else
    {
        print_out("Error: Missing minutes for position.");
        return Position();
    }

    const char* secondsStr = term->getNext();
    if (secondsStr)
        seconds = atof(secondsStr);
    else
    {
        print_out("Error: Missing seconds for position.");
        return Position();
    }

    return Position(degrees, minutes, seconds);
}

static void cmdGotoTargetRA()
{
    Position currentRA = parsePositionFromArgs(_term);
    if (currentRA.arcseconds == 0 && _term->getNext() == NULL)
    {
        print_out_tbl(CMD_GOTO_TARGET_RA_ARGS);
        return;
    }

    Position targetRA = parsePositionFromArgs(_term);
    if (targetRA.arcseconds == 0 && _term->getNext() == NULL)
    {
        print_out_tbl(CMD_GOTO_TARGET_RA_ARGS);
        return;
    }

    bool hemisphereDirection = ra_axis.direction.tracking;

    print_out("GotoTargetRA called with:");
    print_out("  Current Position: %lld arcseconds", currentRA.arcseconds);
    print_out("  Target Position: %lld arcseconds", targetRA.arcseconds);
    print_out("  Hemisphere direction: %d", hemisphereDirection);

    ra_axis.gotoTarget(TRACKER_MOTOR_MICROSTEPPING / 2, (ra_axis.rate.tracking) / 50, currentRA,
                       targetRA, hemisphereDirection);
}

static void cmdPan()
{
    const char* degreesStr = _term->getNext();
    if (!degreesStr)
    {
        print_out_tbl(CMD_PAN_ARGS);
        return;
    }

    const char* speedStr = _term->getNext();
    if (!speedStr)
    {
        print_out_tbl(CMD_PAN_ARGS);
        return;
    }

    const char* steppingStr = _term->getNext();

    float degrees = atof(degreesStr);
    int speed = atoi(speedStr);
    if (speed < MIN_CUSTOM_SLEW_RATE || speed > MAX_CUSTOM_SLEW_RATE)
    {
        print_out("Error: Invalid speed '%s'. Must be between %d and %d.", speedStr,
                  MIN_CUSTOM_SLEW_RATE, MAX_CUSTOM_SLEW_RATE);
        print_out_tbl(CMD_PAN_ARGS);
        return;
    }

    uint16_t microstep = TRACKER_MOTOR_MICROSTEPPING / 2;
    if (steppingStr)
    {
        microstep = atoi(steppingStr);
        if (microstep != 8 && microstep != 16 && microstep != 32 && microstep != 64)
        {
            print_out("Error: Invalid microstepping '%s'. Must be 8, 16, 32, or 64.", steppingStr);
            print_out_tbl(CMD_PAN_ARGS);
            return;
        }
    }

    print_out("Pan command:");
    print_out("  Angle: %.2f degrees (%s)", degrees, (degrees < 0) ? "left" : "right");
    print_out("  Speed: %d", speed);
    print_out("  Microstepping: %d", microstep);

    bool success = ra_axis.panByDegrees(degrees, speed, microstep);

    if (success)
    {
        print_out("Panning started successfully.");
    }
    else
    {
        if (ra_axis.slewActive || ra_axis.goToTarget)
            print_out("Error: Mount is already slewing. Stop current slew first.");
        else if (degrees == 0.0f)
            print_out("Error: Degrees cannot be zero.");
        else
            print_out("Panning failed to start.");
    }
}

static void cmdLed()
{
    const char* indexStr = _term->getNext();
    if (!indexStr)
    {
        print_out_tbl(CMD_LED_ARGS);
        return;
    }

    const char* stateStr = _term->getNext();
    if (!stateStr)
    {
        print_out_tbl(CMD_LED_ARGS);
        return;
    }

    int index = atoi(indexStr);

    bool turnOn = false;
    if (strcmp(stateStr, "on") == 0 || strcmp(stateStr, "1") == 0)
    {
        turnOn = true;
    }
    else if (strcmp(stateStr, "off") == 0 || strcmp(stateStr, "0") == 0)
    {
        turnOn = false;
    }
    else
    {
        print_out("Error: Invalid state '%s'. Must be 'on' or 'off'.", stateStr);
        print_out_tbl(CMD_LED_ARGS);
        return;
    }

    const BoardConfig& cfg = BoardConfigManager::getInstance().getConfig();
    if (!cfg.hasNeoPixelLeds())
    {
        print_out("Error: NeoPixel LEDs not available on this board.");
        return;
    }

    NeoPixelManager& npm = NeoPixelManager::getInstance();
    if (!npm.isAvailable())
    {
        print_out("Error: NeoPixel manager not initialized.");
        return;
    }

    if (turnOn)
    {
        npm.turnOnPixel(index);
        print_out("LED %d turned ON", index);
    }
    else
    {
        npm.clearPixel(index);
        print_out("LED %d turned OFF", index);
    }
    npm.show();
}

static void cmdAccessory()
{
    const char* nameArg = _term->getNext();
    if (nameArg == nullptr)
    {
        AccessoryRegistry::getInstance().printSnapshot();
        return;
    }

    if (strcmp(nameArg, "laser") == 0)
    {
        if (!LaserAccessory::getInstance().isSupported())
        {
            print_out("laser: not supported on this board");
            return;
        }
        const char* action = _term->getNext();
        if (action == nullptr)
        {
            AccessorySnapshot snap = LaserAccessory::getInstance().getSnapshot();
            print_out("laser: %s", snap.state ? "on" : "off");
            return;
        }
        bool on;
        if (strcmp(action, "on") == 0)
            on = true;
        else if (strcmp(action, "off") == 0)
            on = false;
        else if (strcmp(action, "toggle") == 0)
            on = !LaserAccessory::getInstance().getState();
        else
        {
            print_out_tbl(CMD_LASER_ARGS);
            return;
        }
        LaserAccessory::getInstance().setState(on);
        print_out("laser: %s", on ? "on" : "off");
        return;
    }

    if (strcmp(nameArg, "battery") == 0)
    {
        if (!BatteryAccessory::getInstance().isSupported())
        {
            print_out("battery: not supported on this board");
            return;
        }
        AccessorySnapshot snap = BatteryAccessory::getInstance().getSnapshot();
        uint32_t vAdc_mv = (uint32_t) snap.rawAdc * 3300u / 4095u;
        print_out("battery: rawAdc=%u  pin=%lu mV  voltage=%lu mV  ~%u%% (approx)",
                  (unsigned) snap.rawAdc, (unsigned long) vAdc_mv,
                  (unsigned long) snap.primaryValue, (unsigned) snap.secondaryValue);
        return;
    }

    if (strcmp(nameArg, "light") == 0)
    {
        if (!LightAccessory::getInstance().isSupported())
        {
            print_out("light: not supported on this board");
            return;
        }
        AccessorySnapshot snap = LightAccessory::getInstance().getSnapshot();
        print_out("light: rawAdc=%u  ~%u%% (normalized)", (unsigned) snap.rawAdc,
                  (unsigned) snap.primaryValue);
        return;
    }

    print_out_tbl(CMD_ACCESSORY_ARGS);
}

static void cmdUnknownCommand(const char* command)
{
    // Print unknown command
    print_out_tbl(CMD_UNKNOWN_COMMAND);
    print_out("%s", command);
}

static void postCommandHandler()
{
    // Print '> ' for a primitive user UI
    print_out_nonl("> ");
}

void setup_terminal(SerialTerminal* term)
{
    // Initialize terminal
    _term = term;

    // Set default handler for unknown commands
    _term->setDefaultHandler(cmdUnknownCommand);
    // Set handler to be run AFTER a command has been handled.
    _term->setPostCommandHandler(postCommandHandler);
    _term->setSerialEcho(true); // Enable Character Echoing
    // Add command callback handlers
    _term->addCommand("?", cmdHelp);
    _term->addCommand("help", cmdHelp);
    _term->addCommand("version", cmdVersion);
    _term->addCommand("stack", cmdStackAvailable);
    _term->addCommand("heap", cmdHeapAvailable);
    _term->addCommand("reset", cmdReset);
    _term->addCommand("gotoRA", cmdGotoTargetRA);
    _term->addCommand("pan", cmdPan);
    _term->addCommand("led", cmdLed);
    _term->addCommand("accessory", cmdAccessory);
}
