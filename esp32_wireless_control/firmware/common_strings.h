#ifndef COMMAND_STRINGS_H
#define COMMAND_STRINGS_H

#include <pgmspace.h>

/* clang-format off */
// headline
static const char head_line[] PROGMEM = "******************************************\r\n";
static const char head_line_debug[] PROGMEM = "***************Debug Build****************\r\n";
static const char head_line_version[] PROGMEM = "***     " BUILD_VERSION "  " __DATE__ "  " __TIME__ "    ***\r\n";
static const char head_line_tracker[] PROGMEM = "***         Starting Tracker Up        ***\r\n";

// command line interface
static const char cmd_no_input[] PROGMEM = "No input to send - skipping\r\n";
static const char cmd_insufficient_args[] PROGMEM = "  Unsufficient arguments provided - skipping\r\n";
static const char cmd_unknown_argument[] PROGMEM = "Unknown argument: ";
static const char cmd_unknown_command[] PROGMEM = "Unknown command: ";
static const char cmd_heap_minimum_ever_free[] PROGMEM = "Minimum Heap (Bytes): ";
static const char cmd_heap_free[] PROGMEM = "Free heap (Bytes): ";
static const char cmd_heap_available_args[] PROGMEM = "Available args: all\r\n";
static const char cmd_goto_target_ra_args[] PROGMEM = "Usage: gotoRA <+14° 34' 21.4\"> <+54° 12' 42.3\">\r\n";
static const char cmd_pan_args[] PROGMEM = "Usage: pan <degrees> <speed> [microstep]  (-deg=left, microstep=8,16,32,64)\r\n";

static const char cmd_stack_highwater_uart[] PROGMEM = "Uart stack highwater: ";
static const char cmd_stack_highwater_console[] PROGMEM = "Console stack highwater: ";
static const char cmd_stack_highwater_intervalometer[] PROGMEM = "Intervalometer stack highwater: ";
static const char cmd_stack_highwater_webserver[] PROGMEM = "Webserver stack highwater: ";

static const char cmd_help_title[] PROGMEM = "Serial terminal usage:\r\n";
static const char cmd_help_help[] PROGMEM = "  help or ?                      Print this usage\r\n";
static const char cmd_help_stack[] PROGMEM = "  stack <0..N task>              Print available stack\r\n";
static const char cmd_help_heap[] PROGMEM = "  heap <all>                     Print free heap\r\n";
static const char cmd_help_reset[] PROGMEM = "  reset                          Reset the controller\r\n";
static const char cmd_goto_target_ra[] PROGMEM = "  gotoRA <current> <target>      Goto target RA\r\n";
static const char cmd_help_pan[] PROGMEM = "  pan <+/-deg> <speed> [µstep]   Pan mount (µstep: 8,16,32,64)\r\n";

// task related
static const char tsk_not_avail[] PROGMEM = "task not available\r\n";
static const char tsk_clear_screen[] PROGMEM = "\033c";
static const char tsk_start_uart[] PROGMEM = "started uartTask\r\n";
static const char tsk_start_console[] PROGMEM = "started ConsoleTask\r\n";
static const char tsk_start_intervalometer[] PROGMEM = "started IntervalometerTask\r\n";
static const char tsk_start_webserver[] PROGMEM = "started WebserverTask\r\n";

// general purpose
static const char malloc_failed[] PROGMEM = "Malloc failed!\r\n";
static const char not_supported[] PROGMEM = "not supported on this device\r\n";
static const char str_true[] PROGMEM = "true";
static const char str_false[] PROGMEM = "false";
static const char cr_nl[] PROGMEM = "\r\n";

static const char* const string_table[] = {
    // headline
    head_line,
    head_line_debug,
    head_line_version,
    head_line_tracker,

    // command line interface
    cmd_no_input,
    cmd_insufficient_args,
    cmd_unknown_argument,
    cmd_unknown_command,
    cmd_heap_minimum_ever_free,
    cmd_heap_free,
    cmd_heap_available_args,
    cmd_goto_target_ra_args,
    cmd_pan_args,

    cmd_stack_highwater_uart,
    cmd_stack_highwater_console,
    cmd_stack_highwater_intervalometer,
    cmd_stack_highwater_webserver,

    cmd_help_title,
    cmd_help_help,
    cmd_help_stack,
    cmd_help_heap,
    cmd_help_reset,
    cmd_goto_target_ra,
    cmd_help_pan,

    // task related
    tsk_not_avail,
    tsk_clear_screen,
    tsk_start_uart,
    tsk_start_console,
    tsk_start_intervalometer,
    tsk_start_webserver,

    // general purpose
    malloc_failed,
    not_supported,
    str_true,
    str_false,
    cr_nl,
};
/* clang-format on */

enum pgm_table_index_t
{
    HEAD_LINE,
    HEAD_LINE_DEBUG,
    HEAD_LINE_VERSION,
    HEAD_LINE_TRACKER,

    CMD_NO_INPUT,
    CMD_INSUFFICIENT_ARGS,
    CMD_UNKNOWN_ARGUMENT,
    CMD_UNKNOWN_COMMAND,
    CMD_HEAP_MINIMUM_EVER_FREE,
    CMD_HEAP_FREE,
    CMD_HEAP_AVAILABLE_ARGS,
    CMD_GOTO_TARGET_RA_ARGS,
    CMD_PAN_ARGS,

    CMD_STACK_HIGHWATER_UART,
    CMD_STACK_HIGHWATER_CONSOLE,
    CMD_STACK_HIGHWATER_INTERVALOMETER,
    CMD_STACK_HIGHWATER_WEBSERVER,

    CMD_HELP_TITLE,
    CMD_HELP_HELP,
    CMD_HELP_STACK,
    CMD_HELP_HEAP,
    CMD_HELP_RESET,
    CMD_GOTO_TARGET_RA,
    CMD_HELP_PAN,

    TSK_NOT_AVAIL,
    TSK_CLEAR_SCREEN,
    TSK_START_UART,
    TSK_START_CONSOLE,
    TSK_START_INTERVALOMETER,
    TSK_START_WEBSERVER,

    MALLOC_FAILED,
    NOT_SUPPORTED,
    STR_TRUE,
    STR_FALSE,
    CR_NL,
};

#endif
