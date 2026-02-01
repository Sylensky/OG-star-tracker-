#pragma once

#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "error.h"

/**
 * @brief Base class for all intervalometer modes
 *
 * Each mode runs in its own FreeRTOS task with its own execution loop.
 * The task is dynamically created when capture starts and deleted when complete.
 */
class IntervalometerMode
{
  public:
    enum class State : uint8_t
    {
        Inactive,
        PreDelay,
        Capture,
        Dither,
        Pan,
        Delay,
        Rewind,
        Complete
    };

    struct Settings // 28 bytes
    {
        float panAngle = 0.0f;  // degrees (for pan modes) (4b)
        float pixelSize = 1.0f; // micrometre (um) - for dither calculation (4b)

        uint16_t exposures = 1;    // Number of exposures (2b)
        uint16_t delayTime = 1;    // seconds between exposures (2b)
        uint16_t preDelay = 1;     // seconds before first exposure (2b)
        uint16_t exposureTime = 1; // seconds per exposure (2b)
        uint16_t frames = 1;       // Number of frames (for movie mode) (2b)
        uint16_t focalLength = 1;  // mm - for dither calculation (2b)

        uint8_t mode = 0;            // Mode enum value (1b)
        uint8_t ditherFrequency = 1; // Dither every N exposures (1b)

        bool panDirection = true;    // true = forward, false = reverse (1b)
        bool continuousPan = false;  // Continuous pan during entire sequence (1b)
        bool dither = false;         // Enable dithering (1b)
        bool enableTracking = false; // Enable tracking during capture (1b)
    };

    // Current packed layout should be 28 bytes (padded to 4-byte boundary).
    static_assert(sizeof(Settings) == 28,
                  "IntervalometerMode::Settings size changed; update code/comments");

    /**
     * @brief Construct a new Intervalometer Mode object
     * @param triggerPin GPIO pin for camera trigger
     * @param settings Capture settings
     */
    IntervalometerMode(uint8_t triggerPin, const Settings& settings);

    /**
     * @brief Virtual destructor to ensure proper cleanup
     */
    virtual ~IntervalometerMode();

    /**
     * @brief Start the capture sequence by creating the FreeRTOS task
     * @return true if task creation successful
     */
    bool startCapture();

    /**
     * @brief Request abort of the capture sequence
     */
    void abortCapture();

    /**
     * @brief Check if capture is currently active
     */
    bool isActive() const
    {
        return active;
    }

    /**
     * @brief Get current state
     */
    State getState() const
    {
        return currentState;
    }

    /**
     * @brief Get settings
     */
    const Settings& getSettings() const
    {
        return settings;
    }

    /**
     * @brief Get error message
     */
    ErrorMessage getErrorMessage() const
    {
        return errorMessage;
    }

    /**
     * @brief Get current exposure count
     */
    uint16_t getCurrentExposure() const
    {
        return currentExposure;
    }

    /**
     * @brief Get total exposures taken
     */
    uint16_t getExposuresTaken() const
    {
        return exposuresTaken;
    }

    /**
     * @brief Get start tick count
     */
    TickType_t getStartCaptureTickCount() const
    {
        return startCaptureTickCount;
    }

    /**
     * @brief Get estimated duration in ticks
     */
    TickType_t getCaptureDurationTickCount() const
    {
        return captureDurationTickCount;
    }

  protected:
    /**
     * @brief Pure virtual function - main execution loop for the mode
     * Each derived class implements its own capture logic
     */
    virtual void executeLoop() = 0;

    /**
     * @brief Get the name of this mode for logging
     */
    virtual const char* getModeName() const = 0;

    /**
     * @brief Calculate total capture duration in seconds
     */
    virtual uint32_t calculateTotalDuration() const;

    /**
     * @brief Perform pre-delay before starting captures
     */
    void performPreDelay();

    /**
     * @brief Trigger camera shutter ON
     */
    void triggerOn();

    /**
     * @brief Trigger camera shutter OFF
     */
    void triggerOff();

    /**
     * @brief Wait for specified milliseconds while checking abort flag
     * @param ms Milliseconds to wait
     * @return true if wait completed, false if aborted
     */
    bool waitWithAbortCheck(uint32_t ms);

    /**
     * @brief Perform dithering movement
     * @return true if dither completed successfully
     */
    bool performDither();

    /**
     * @brief Calculate steps needed for 10 pixels
     */
    uint16_t getStepsPerTenPixels() const;

    /**
     * @brief Calculate arcseconds per pixel
     */
    float getArcsecPerPixel() const;

    /**
     * @brief Generate biased random direction for dithering
     */
    uint8_t biasedRandomDirection(uint8_t previousDirection);

    /**
     * @brief Static task function wrapper for FreeRTOS
     */
    static void taskWrapper(void* pvParameters);

    /**
     * @brief Cleanup resources when task completes
     */
    void cleanup();

    // Protected member variables
    uint8_t triggerPin;
    Settings settings;
    State currentState;
    ErrorMessage errorMessage;

    bool active;
    bool abortRequested;
    uint16_t exposuresTaken;
    uint16_t currentExposure;
    uint8_t previousDitherDirection;

    TickType_t startCaptureTickCount;
    TickType_t captureDurationTickCount;

    TaskHandle_t taskHandle;
};
