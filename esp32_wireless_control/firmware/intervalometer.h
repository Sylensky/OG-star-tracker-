#ifndef INTERVALOMETER_H
#define INTERVALOMETER_H

#include "axis.h"
#include "config.h"
#include "error.h"
#include "hardwaretimer.h"
#include <EEPROM.h>

#ifdef STEPPER_0_9
#define ARCSEC_PER_STEP 2.0
#else
#define ARCSEC_PER_STEP 4.0
#endif

class Intervalometer
{
  public:
    enum Mode
    {
        LONG_EXPOSURE_STILL,
        LONG_EXPOSURE_MOVIE,
        DAY_TIME_LAPSE,
        DAY_TIME_LAPSE_PAN,
        MAX_MODES
    };

  private:
    HardwareTimer intervalometerTimer;
    bool timerStarted;
    bool axisMoving;
    int total;
    void savePresetsToEEPPROM();

    uint8_t triggerPin;
    uint16_t exposures_taken;
    uint16_t frames_taken;
    uint16_t getStepsPerTenPixels();
    float getArcsec_per_pixel();
    template <class T> int writeObjectToEEPROM(int address, const T& object);
    template <class T> int readObjectFromEEPROM(int address, T& object);
    uint8_t biasedRandomDirection(uint8_t previous_direction);
    uint8_t previousDitherDirection;

    struct Settings
    {                            // 28 bytes
        Mode mode;               // 4b
        uint16_t exposures;      // 2b
        uint16_t delayTime;      // seconds, max limt 18 h, 12 mins //2b
        uint16_t preDelay;       // seconds //2b
        uint16_t exposureTime;   // seconds, max limt 18 h, 12 mins //2b
        float panAngle;          // degrees //4b
        bool panDirection;       // 1b
        bool dither;             // 1b
        uint8_t ditherFrequency; // 1b
        bool enableTracking;     // 1b
        uint16_t frames;         // 2b
        float pixelSize;         // micrometre (um) //4b
        uint16_t focalLength;    // mm //2b
    };

  public:
    enum State
    {
        INACTIVE,
        PRE_DELAY,
        CAPTURE,
        DITHER,
        PAN,
        DELAY,
        REWIND
    };
    State currentState;
    Settings currentSettings;
    Settings presets[10];
    ErrorMessage currentErrorMessage;
    void readPresetsFromEEPROM();
    void abortCapture();
    void startCapture();
    bool nextState;
    bool intervalometerActive;
    Intervalometer(uint8_t triggerPinArg);
    void saveSettingsToPreset(uint8_t preset);
    void readSettingsFromPreset(uint8_t preset);
    void run();
};

extern Intervalometer intervalometer;

#endif