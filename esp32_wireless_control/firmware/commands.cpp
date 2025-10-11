#include <freertos/FreeRTOS.h>

#include <axis.h>
#include <commands.h>
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
    print_out_tbl(CMD_HELP_STACK);
    print_out_tbl(CMD_HELP_HEAP);
    print_out_tbl(CMD_HELP_RESET);
    print_out_tbl(CMD_GOTO_TARGET_RA);
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
void (*resetFunc)(void) = 0; // declare reset fuction at address 0
static void cmdReset()
{
    // Reset controller
    resetFunc();
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

    print_out("GotoTargetRA called with:");
    print_out("  Current Position: %lld arcseconds", currentRA.arcseconds);
    print_out("  Target Position: %lld arcseconds", targetRA.arcseconds);

    ra_axis.gotoTarget((ra_axis.rate.tracking) / 50, currentRA, targetRA);
}

static void unknownCommand(const char* command)
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
    _term->setDefaultHandler(unknownCommand);
    // Set handler to be run AFTER a command has been handled.
    _term->setPostCommandHandler(postCommandHandler);
    _term->setSerialEcho(true); // Enable Character Echoing
    // Add command callback handlers
    _term->addCommand("?", cmdHelp);
    _term->addCommand("help", cmdHelp);
    _term->addCommand("stack", cmdStackAvailable);
    _term->addCommand("heap", cmdHeapAvailable);
    _term->addCommand("reset", cmdReset);
    _term->addCommand("gotoRA", cmdGotoTargetRA);
}
