/**
 * @file uart.h
 * @version 0.1.0
 *
 * @section License
 * Copyright (C) 2025, Sylensky
 */

#ifndef UART_H
#define UART_H
#include <Arduino.h>

#define MAX_UART_LINE_LEN 64

void print_out(const char* format, ...);
void setup_uart(HardwareSerial* serial, long baudrate);
void uart_task();

#endif
