#ifndef AXIS_H
#define AXIS_H

#include "configs/config.h"
#include "configs/consts.h"
#include "drivers/motor_driver.h"
#include "hardwaretimer.h"

#include "tracking_rates.h"

extern HardwareTimer slewTimeOut;

class Position
{
  public:
    int64_t ra_arcseconds;  // Right Ascension in arcseconds (0-86399 for 0-24h)
    int64_t dec_arcseconds; // Declination in arcseconds (-324000 to +324000 for -90° to +90°)

    // Default constructor
    Position() : ra_arcseconds(0), dec_arcseconds(0)
    {
    }

    // Constructor for RA and DEC in HMS/DMS format
    Position(int ra_hours, int ra_minutes, float ra_seconds, int dec_degrees, int dec_minutes,
             float dec_seconds);

    // Constructor for RA and DEC directly in arcseconds
    Position(int64_t ra_arc, int64_t dec_arc) : ra_arcseconds(ra_arc), dec_arcseconds(dec_arc)
    {
    }

    // Convert RA to hours (0-24)
    float raToHours() const;

    // Convert DEC to degrees (-90 to +90)
    float decToDegrees() const;

    // Static helper to convert HMS to arcseconds
    static int64_t hmsToArcseconds(int hours, int minutes, float seconds);

    // Static helper to convert DMS to arcseconds
    static int64_t dmsToArcseconds(int degrees, int minutes, float seconds);
};

class Direction
{
  public:
    bool tracking;
    bool requested;
    volatile bool absolute;
};

class Rate
{
  public:
    uint64_t tracking;
    uint64_t requested;
};

class Axis
{
  public:
    Axis(uint8_t axisNumber, MotorDriver* driver, uint8_t dirPinforAxis, bool invertDirPin);

    void setAxisTargetCount(int64_t count);
    int64_t getAxisTargetCount();
    void resetAxisCount();
    void setAxisCount(int64_t count);
    int64_t getAxisCount();

    void startTracking(uint64_t rate, bool directionArg);
    void stopTracking();
    void startSlew(uint64_t rate, bool directionArg);
    void stopSlew();
    void resumeSlewIfNeeded();

    void gotoTarget(uint16_t microstep, uint64_t rate, const Position& current,
                    const Position& target);
    void stopGotoTarget();
    void resumeGoto(uint64_t rateArg);

    bool panByDegrees(float degrees, int speed,
                      uint16_t microstep = TRACKER_MOTOR_MICROSTEPPING / 2);
    bool stopPanByDegrees();

    volatile int64_t axisCountValue;
    volatile int64_t targetCount;
    volatile bool goToTarget;
    bool slewActive;
    bool trackingActive;

    Direction direction;

    volatile bool counterActive;

    Rate rate;
    uint64_t currentSlewRate = 0;

    uint16_t getMicrostep()
    {
        return microStep;
    }

    volatile int64_t position;
    void resetPosition()
    {
        setPosition(0);
    }
    void setPosition(int64_t pos)
    {
        position = pos;
    }
    int64_t getPosition()
    {
        return position;
    }

    void requestTracking(uint64_t requestedRate, bool requestedDirection)
    {
        rate.requested = requestedRate;
        direction.requested = requestedDirection;
        startRequested = true;
    }

    bool trackingRequested()
    {
        return startRequested;
    }

    void begin();

    void print_status();

  private:
    void setDirection(bool directionArg);
    void setMicrostep(uint16_t microstep);

    HardwareTimer stepTimer;
    uint16_t microStep;
    uint8_t stepPin;
    uint8_t dirPin;
    uint8_t axisNumber;
    bool invertDirectionPin;
    MotorDriver* driver;
    volatile bool startRequested;
};

extern Axis ra_axis;

#endif
