/**
 * @file uart.cpp
 * @version 0.1.0
 *
 * @section License
 * Copyright (C) 2025, Sylensky
 */
#include <Arduino.h>
#include <pgmspace.h>
#include <string.h>

#include <common_strings.h>
#include <uart.h>

QueueHandle_t uartq;
SemaphoreHandle_t uart_tx_mutex;
SemaphoreHandle_t uart_rx_mutex;
HardwareSerial* _uart;

char rec_uart_buffer[MAX_UART_LINE_LEN];
char tra_uart_buffer[MAX_UART_LINE_LEN];

void print_out(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintf(tra_uart_buffer, MAX_UART_LINE_LEN, format, args);
    va_end(args);

    strcat(tra_uart_buffer, "\r\n");

    xQueueSend(uartq, tra_uart_buffer, portMAX_DELAY);
}

void print_out_nonl(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintf(tra_uart_buffer, MAX_UART_LINE_LEN, format, args);
    va_end(args);

    xQueueSend(uartq, tra_uart_buffer, portMAX_DELAY);
}

void print_out_tbl(uint8_t index)
{
    char tra_uart_buffer[MAX_UART_LINE_LEN];
    const char* src = (const char*) pgm_read_ptr(&string_table[index]);

    // Copy string from PROGMEM to RAM
    int i = 0;
    char c;
    while ((c = pgm_read_byte(src + i)) != '\0' && i < MAX_UART_LINE_LEN - 1)
    {
        tra_uart_buffer[i] = c;
        i++;
    }
    tra_uart_buffer[i] = '\0';

    xQueueSend(uartq, tra_uart_buffer, portMAX_DELAY);
}

void setup_uart(HardwareSerial* serial, long baudrate)
{
    _uart = serial;
    _uart->begin(baudrate);
    uartq = xQueueCreate(128, MAX_UART_LINE_LEN);
    uart_tx_mutex = xSemaphoreCreateMutex();
    uart_rx_mutex = xSemaphoreCreateMutex();
    xSemaphoreGive(uart_tx_mutex);
    xSemaphoreGive(uart_rx_mutex);
}

void uart_task()
{
    if (uxQueueMessagesWaiting(uartq) > 0)
    {
        if (xSemaphoreTake(uart_rx_mutex, 10) == pdTRUE)
        {
            if (xQueueReceive(uartq, &rec_uart_buffer, portMAX_DELAY) == pdPASS)
                _uart->print(rec_uart_buffer);

            xSemaphoreGive(uart_rx_mutex);
        }
    }
}
