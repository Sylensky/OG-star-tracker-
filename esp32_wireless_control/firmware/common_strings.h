#ifndef COMMAND_STRINGS_H
#define COMMAND_STRINGS_H

/* clang-format off */
// headline
static const char head_line[] = "******************************************\r\n";
static const char head_line_debug[] = "***************Debug Build****************\r\n";
static const char head_line_version[] = "***     " BUILD_VERSION "  " __DATE__ "  " __TIME__ "    ***\r\n";
static const char head_line_tracker[] = "***         Starting Tracker Up        ***\r\n";

// command line interface
static const char cmd_no_input[] = "No input to send - skipping\r\n";
static const char cmd_insufficient_args[] = "  Unsufficient arguments provided - skipping\r\n";
static const char cmd_unknown_argument[] = "Unknown argument: ";
static const char cmd_unknown_command[] = "Unknown command: ";
static const char cmd_heap_minimum_ever_free[] = "Minimum Heap (Bytes): ";
static const char cmd_heap_free[] = "Free heap (Bytes): ";
static const char cmd_heap_available_args[] = "Available args: all\r\n";
static const char cmd_goto_target_ra_args[] = "Usage: gotoRA <+14° 34' 21.4\"> <+54° 12' 42.3\">\r\n";

static const char cmd_stack_highwater_uart[] = "Uart stack highwater: ";
static const char cmd_stack_highwater_console[] = "Console stack highwater: ";
static const char cmd_stack_highwater_intervalometer[] = "Intervalometer stack highwater: ";
static const char cmd_stack_highwater_webserver[] = "Webserver stack highwater: ";
static const char cmd_stack_highwater_dnsserver[] = "DNSserver stack highwater: ";

static const char cmd_help_title[] = "Serial terminal usage:\r\n";
static const char cmd_help_help[] = "  help or ?                      Print this usage\r\n";
static const char cmd_help_stack[] = "  stack <0..N task>              Print available stack\r\n";
static const char cmd_help_heap[] = "  heap <all>                     Print free heap\r\n";
static const char cmd_help_reset[] = "  reset                          Reset the controller\r\n";
static const char cmd_goto_target_ra[] = "  gotoRA <current> <target>      Goto target RA\r\n";

// task related
static const char tsk_not_avail[] = "task not available\r\n";
static const char tsk_clear_screen[] = "\033c";
static const char tsk_start_uart[] = "started uartTask\r\n";
static const char tsk_start_console[] = "started ConsoleTask\r\n";
static const char tsk_start_intervalometer[] = "started IntervalometerTask\r\n";
static const char tsk_start_webserver[] = "started WebserverTask\r\n";
static const char tsk_start_dnsserver[] = "started DNSserverTask\r\n";

// general purpose
static const char malloc_failed[] = "Malloc failed!\r\n";
static const char not_supported[] = "not supported on this device\r\n";
static const char str_true[] = "true";
static const char str_false[] = "false";
static const char cr_nl[] = "\r\n";

const char* const string_table[] = {
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

    cmd_stack_highwater_uart,
    cmd_stack_highwater_console,
    cmd_stack_highwater_intervalometer,
    cmd_stack_highwater_webserver,
    cmd_stack_highwater_dnsserver,

    cmd_help_title,
    cmd_help_help,
    cmd_help_stack,
    cmd_help_heap,
    cmd_help_reset,
    cmd_goto_target_ra,

    // task related
    tsk_not_avail,
    tsk_clear_screen,
    tsk_start_uart,
    tsk_start_console,
    tsk_start_intervalometer,
    tsk_start_webserver,
    tsk_start_dnsserver,

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

    CMD_STACK_HIGHWATER_UART,
    CMD_STACK_HIGHWATER_CONSOLE,
    CMD_STACK_HIGHWATER_INTERVALOMETER,
    CMD_STACK_HIGHWATER_WEBSERVER,
    CMD_STACK_HIGHWATER_DNSSERVER,

    CMD_HELP_TITLE,
    CMD_HELP_HELP,
    CMD_HELP_STACK,
    CMD_HELP_HEAP,
    CMD_HELP_RESET,
    CMD_GOTO_TARGET_RA,

    TSK_NOT_AVAIL,
    TSK_CLEAR_SCREEN,
    TSK_START_UART,
    TSK_START_CONSOLE,
    TSK_START_INTERVALOMETER,
    TSK_START_WEBSERVER,
    TSK_START_DNSSERVER,

    MALLOC_FAILED,
    NOT_SUPPORTED,
    STR_TRUE,
    STR_FALSE,
    CR_NL,
};

#endif
