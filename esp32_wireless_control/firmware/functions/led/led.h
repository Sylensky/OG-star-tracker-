/**
 * @file led.h
 * @version 0.1.0
 *
 * @section License
 * Copyright (C) 2026, Sylensky
 */

#ifndef LED_H
#define LED_H

#include <Arduino.h>

// Forward declarations
class BoardConfig;
class NeoPixelManager;

/**
 * @brief Base LED control class
 *
 * Abstract base class for all LED types.
 * Provides common functionality for LED state management and hardware control.
 */
class LEDBase
{
  public:
    LEDBase();
    virtual ~LEDBase() = default;

    virtual void init(uint8_t pin);

    /**
     * @brief Initialize LED with PWM support
     * @param pin GPIO pin number
     * @param freq PWM frequency in Hz
     * @param resolution PWM resolution (bits)
     * @return true if PWM attached successfully, false otherwise
     */
    virtual bool initPWM(uint8_t pin, uint32_t freq, uint8_t resolution);

    virtual void toggle();
    virtual void on();
    virtual void off();

    /**
     * @brief Set the LED to a specific state
     * @param state HIGH or LOW
     */
    virtual void set(uint8_t state);

    /**
     * @brief Set LED brightness (only works if PWM is enabled)
     * @param brightness Brightness value (0-255 for 8-bit resolution)
     */
    virtual void setBrightness(uint8_t brightness);

    /**
     * @brief Set LED color (NeoPixel only, ignored on regular LEDs)
     * @param r Red value (0-255)
     * @param g Green value (0-255)
     * @param b Blue value (0-255)
     */
    virtual void setColor(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Get the current LED state
     * @return Current state (HIGH or LOW)
     */
    uint8_t getState() const;

    /**
     * @brief Get the current brightness value
     * @return Brightness value (0-255)
     */
    uint8_t getBrightness() const;

    /**
     * @brief Check if LED is initialized
     */
    bool isInitialized() const;

    /**
     * @brief Check if PWM is enabled for this LED
     */
    bool isPWMEnabled() const;

  protected:
    friend class LED;

    uint8_t _pin;
    uint8_t _state;
    uint8_t _brightness;
    bool _initialized;
    bool _pwmEnabled;
    uint32_t _pwmFreq;
    uint8_t _pwmResolution;
    bool _isNeoPixel;
    uint8_t _neoPixelIndex;

    void setNeoPixelMode(uint8_t index, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);
};

/**
 * @brief Status LED class
 *
 * Used for general system status indication.
 */
class StatusLED : public LEDBase
{
  public:
    StatusLED() = default;
    ~StatusLED() override = default;
};

/**
 * @brief Trigger LED class
 *
 * Used for camera trigger control in intervalometer modes.
 */
class TriggerLED : public LEDBase
{
  public:
    TriggerLED() = default;
    ~TriggerLED() override = default;
};

/**
 * @brief Camera LED class
 *
 * Used for camera-related visual indication (v2.2 NeoPixel only).
 */
class CameraLED : public LEDBase
{
  public:
    CameraLED() = default;
    ~CameraLED() override = default;
};

/**
 * @brief Power LED class
 *
 * Used for power status indication (v2.2 NeoPixel only).
 */
class PowerLED : public LEDBase
{
  public:
    PowerLED() = default;
    ~PowerLED() override = default;
};

/**
 * @brief LED Manager Singleton
 *
 * Centralized manager for all LED instances in the system.
 * Usage: LED::getInstance().status.toggle()
 */
class LED
{
  public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the LED manager instance
     */
    static LED& getInstance();

    StatusLED status;
    TriggerLED trigger;
    CameraLED camera; // NeoPixel on v3, unused on v2.x
    PowerLED power;   // NeoPixel on v3, unused on v2.x

    /**
     * @brief Initialize all LEDs based on board configuration
     */
    void initAll();

    /**
     * @brief Update all NeoPixel LEDs to reflect their current states
     * Call this periodically from main loop to apply state changes
     */
    void updateAll();

  private:
    LED();                               // Private constructor
    LED(const LED&) = delete;            // Prevent copying
    LED& operator=(const LED&) = delete; // Prevent assignment
};

#endif // LED_H
