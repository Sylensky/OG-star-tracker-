/**
 * @file neopixel_manager.h
 * @version 0.1.0
 *
 * @section License
 * Copyright (C) 2026, Sylensky
 */

#ifndef NEOPIXEL_MANAGER_H
#define NEOPIXEL_MANAGER_H

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief NeoPixel Manager Singleton
 *
 * Centralized manager for NeoPixel LED strip control.
 * Handles initialization and provides methods to set individual pixel colors.
 */
class NeoPixelManager
{
  public:
    /**
     * @brief Get the singleton instance
     */
    static NeoPixelManager& getInstance();

    /**
     * @brief Initialize NeoPixel strip based on board configuration
     * @return true if NeoPixels are available, false otherwise
     */
    bool init();

    /**
     * @brief Check if NeoPixels are initialized and available
     */
    bool isAvailable() const;

    /**
     * @brief Set pixel color
     * @param index Pixel index in the strip
     * @param r Red value (0-255)
     * @param g Green value (0-255)
     * @param b Blue value (0-255)
     */
    void setPixelColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Set default color for a pixel (used when turning on)
     * @param index Pixel index in the strip
     * @param r Red value (0-255)
     * @param g Green value (0-255)
     * @param b Blue value (0-255)
     */
    void setDefaultColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Turn on a pixel using its stored default color
     * @param index Pixel index in the strip
     */
    void turnOnPixel(uint8_t index);

    /**
     * @brief Turn off a specific pixel
     * @param index Pixel index in the strip
     */
    void clearPixel(uint8_t index);

    /**
     * @brief Update the NeoPixel strip to show changes
     */
    void show();

    /**
     * @brief Atomically set pixel state and show (thread-safe)
     * @param index Pixel index
     * @param turnOn true to turn on with default color, false to turn off
     */
    void updatePixel(uint8_t index, bool turnOn);

    /**
     * @brief Atomically update all pixels and show (thread-safe)
     * @param states Array of 8 states (HIGH/LOW) for each pixel
     */
    void updateAllPixels(const uint8_t* states, uint8_t count);

    /**
     * @brief Clear all pixels
     */
    void clearAll();

  private:
    NeoPixelManager();
    ~NeoPixelManager();
    NeoPixelManager(const NeoPixelManager&) = delete;
    NeoPixelManager& operator=(const NeoPixelManager&) = delete;

    Adafruit_NeoPixel* _pixels;
    bool _initialized;
    SemaphoreHandle_t _mutex;

    struct PixelColor
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    PixelColor _defaultColors[8];
};

#endif // NEOPIXEL_MANAGER_H
