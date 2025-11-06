#include "soc/gpio_struct.h"

#include "axis.h"
#include "uart.h"

#if MICROSTEPPING_MOTOR_DRIVER == USE_MSx_PINS_MICROSTEPPING
#include "drivers/msx_motor_driver.h"
MSxMotorDriver ra_driver(RA_MS1, RA_MS2, AXIS1_DIR);
#elif MICROSTEPPING_MOTOR_DRIVER == USE_TMC_DRIVER_MICROSTEPPING
#include "drivers/tmc_motor_driver.h"
TmcMotorDriver ra_driver(&AXIS_SERIAL_PORT, AXIS1_ADDR, TMC_R_SENSE, AXIS_RX, AXIS_TX);
#else
#error Unknown Motor Driver
#endif

Axis ra_axis(1, &ra_driver, AXIS1_DIR, RA_INVERT_DIR_PIN);

volatile bool ra_axis_step_phase = 0;

void IRAM_ATTR stepTimerRA_ISR()
{
    // ra ISR
    ra_axis_step_phase = !ra_axis_step_phase;
    if (ra_axis_step_phase)
    {
#ifdef BOARD_HAS_PIN_REMAP
        digitalWrite(AXIS1_STEP, HIGH);
#else
        GPIO.out_w1ts = (1 << AXIS1_STEP); // Set pin high
#endif
    }
    else
    {
#ifdef BOARD_HAS_PIN_REMAP
        digitalWrite(AXIS1_STEP, LOW);
#else
        GPIO.out_w1tc = (1 << AXIS1_STEP); // Set pin low
#endif
    }

    int64_t position = ra_axis.getPosition();
    uint16_t uStep = ra_axis.getMicrostep();
    if (ra_axis_step_phase)
    {
        if (ra_axis.direction.absolute ^ ra_axis.direction.tracking)
        {
            position -= MAX_MICROSTEPS / (uStep ? uStep : 1);
        }
        else
        {
            position += MAX_MICROSTEPS / (uStep ? uStep : 1);
        }
        ra_axis.setPosition(position);
    }

    if (ra_axis.counterActive && ra_axis_step_phase)
    { // if counter active
        int64_t temp = ra_axis.getAxisCount();
        if (ra_axis.direction.absolute ^ ra_axis.direction.tracking)
        {
            temp--;
        }
        else
        {
            temp++;
        }
        ra_axis.setAxisCount(temp);
        if (ra_axis.goToTarget && ra_axis.getAxisCount() == ra_axis.getAxisTargetCount())
        {
            print_out("axisCountValue: %lld", ra_axis.getAxisCount());
            print_out("targetCount: %lld", ra_axis.getAxisTargetCount());
            ra_axis.goToTarget = false;
            ra_axis.stopSlew();
        }
    }
}

void IRAM_ATTR slewTimeOutTimer_ISR()
{
    ra_axis.stopSlew();
}

HardwareTimer slewTimeOut(2000, &slewTimeOutTimer_ISR);

// Position class implementation
Position::Position(int ra_hours, int ra_minutes, float ra_seconds, int dec_degrees, int dec_minutes,
                   float dec_seconds)
{
    ra_arcseconds = hmsToArcseconds(ra_hours, ra_minutes, ra_seconds);
    dec_arcseconds = dmsToArcseconds(dec_degrees, dec_minutes, dec_seconds);
}

float Position::raToHours() const
{
    return ra_arcseconds / 3600.0f; // 86400 arcsec = 24 hours
}

float Position::decToDegrees() const
{
    return dec_arcseconds / 3600.0f;
}

int64_t Position::hmsToArcseconds(int hours, int minutes, float seconds)
{
    // RA: 24 hours = 86400 arcseconds of RA (1 hour = 3600 arcsec of RA)
    return (hours * 3600) + (minutes * 60) + static_cast<int>(seconds);
}

int64_t Position::dmsToArcseconds(int degrees, int minutes, float seconds)
{
    // DEC: standard arcseconds (1 degree = 3600 arcsec)
    int64_t total = (abs(degrees) * 3600) + (minutes * 60) + static_cast<int>(seconds);
    return (degrees < 0) ? -total : total;
}

void axisTask(void* parameter)
{
    Axis* axis = (Axis*) parameter;
    for (;;)
    {
        if (axis->trackingRequested())
        {
            axis->startTracking(axis->rate.requested, axis->direction.requested);
        }
        vTaskDelay(1);
    }
}

Axis::Axis(uint8_t axis, MotorDriver* motorDriver, uint8_t dirPinforAxis, bool invertDirPin)
    : stepTimer(TIMER_APB_CLK_FREQ), startRequested(false)
{
    driver = motorDriver;
    axisNumber = axis;
    direction.tracking = c_DIRECTION;
    dirPin = dirPinforAxis;
    invertDirectionPin = invertDirPin;
    rate.tracking = trackingRates.getRate();

    pinMode(dirPin, OUTPUT);

    switch (axisNumber)
    {
        case 1:
            stepTimer.attachInterupt(&stepTimerRA_ISR);
            break;
    }
}

void Axis::begin()
{
    if (xTaskCreatePinnedToCore(axisTask, "axis_task", 4096, this, 1, NULL, 1))
        print_out_nonl("Started axis task\n");
}

void Axis::startTracking(uint64_t rateArg, bool directionArg)
{
    startRequested = false;
    rate.tracking = rateArg;
    direction.tracking = directionArg;
    setDirection(directionArg);
    trackingActive = true;
    stepTimer.stop();
    setMicrostep(TRACKER_MOTOR_MICROSTEPPING);
    stepTimer.start(rate.tracking, true);
}

void Axis::stopTracking()
{
    trackingActive = false;
    stepTimer.stop();
}

void Axis::gotoTarget(uint16_t microstep, uint64_t rateArg, const Position& current,
                      const Position& target)
{
    setMicrostep(microstep);

    /**
     * TODO: implement DEC wrapping aswell
     */
    int64_t deltaArcseconds = target.ra_arcseconds - current.ra_arcseconds;
    int64_t stepsPerSecond = trackingRates.getStepsPerSecondSolar();

    print_out_nonl("deltaArcseconds (RA): %lld\n", deltaArcseconds);

    if (abs(deltaArcseconds) > 86400 / 2)
    {
        if (deltaArcseconds > 0)
        {
            deltaArcseconds = (deltaArcseconds - 86400) % 86400;
        }
        else
        {
            deltaArcseconds = (deltaArcseconds + 86400) % 86400;
        }
    }

    int64_t stepsToMove =
        (deltaArcseconds * stepsPerSecond) / (MAX_MICROSTEPS / (microStep ? microStep : 1));
    bool directionTmp = (stepsToMove < 0) ^ direction.tracking;

    print_out_nonl("stepsToMove: %lld\n", stepsToMove);

    /**
     * TODO: setPosition should take Position object directly
     * Maybe introducing a axis type enum {RA, DEC} to specify which axis to set
     */
    setPosition(current.ra_arcseconds * stepsPerSecond);
    resetAxisCount();
    setAxisTargetCount(stepsToMove);

    if (targetCount != axisCountValue)
    {
        counterActive = true;
        goToTarget = true;
        stepTimer.stop();
        setDirection(directionTmp);
        slewActive = true;
        currentSlewRate = rateArg;
        stepTimer.start(rateArg, true);
    }
}

void Axis::stopGotoTarget()
{
    goToTarget = false;
    counterActive = false;
    stepTimer.stop();
    slewTimeOut.start(1, true);
}

void Axis::resumeGoto(uint64_t rateArg)
{
    // If a goto was previously set (goToTarget true) and the stepTimer is not
    // currently running, restart the timer with the provided rate. Preserve
    // the targetCount and counterActive flags.
    if (goToTarget)
    {
        // Stop any existing timer state then restart with the new rate
        stepTimer.stop();
        setMicrostep(TRACKER_MOTOR_MICROSTEPPING / 2);
        currentSlewRate = rateArg;
        slewActive = true;
        // Do not start generic slew timeout for goto-style movements
        stepTimer.start(rateArg, true);
    }
}

bool Axis::panByDegrees(float degrees, int speed, uint16_t microstep)
{
    if (slewActive || goToTarget || (degrees == 0.0f))
        return false;

    // For a full 360° rotation, the ISR will count to STEPS_PER_TRACKER_FULL_REV_INT /
    // (MAX_MICROSTEPS / microstep) This is because the ISR increments once per step, regardless of
    // microstepping
    int64_t stepsPerFullRotation =
        STEPS_PER_TRACKER_FULL_REV_INT / (MAX_MICROSTEPS / (microstep ? microstep : 1));

    // Calculate target count for the given degrees
    int64_t stepsToMove = (int64_t) ((std::abs(degrees) / 360.0f) * stepsPerFullRotation + 0.5f);

    // Apply sign based on pan direction
    if (degrees < 0)
        stepsToMove = -stepsToMove;

    // Determine direction based on sign of stepsToMove
    bool directionTmp = (stepsToMove < 0) ^ direction.tracking;

    print_out("Pan: %.2f degrees => %lld ISR steps (microstep %d)", degrees, stepsToMove,
              microstep);
    print_out("stepsPerFullRotation: %lld, STEPS_FULL_REV: %lld", stepsPerFullRotation,
              (int64_t) STEPS_PER_TRACKER_FULL_REV_INT);

    // Set up the goto directly without Position wrapping
    setMicrostep(microstep);
    resetAxisCount();
    setAxisTargetCount(stepsToMove);

    if (stepsToMove != 0)
    {
        counterActive = true;
        goToTarget = true;
        stepTimer.stop();
        setDirection(directionTmp);
        slewActive = true;
        currentSlewRate = (2 * rate.tracking) / speed;
        stepTimer.start(currentSlewRate, true);
        print_out("Pan started: counterActive=%d, goToTarget=%d, targetCount=%lld", counterActive,
                  goToTarget, getAxisTargetCount());
    }

    return goToTarget;
}

bool Axis::stopPanByDegrees()
{
    if (slewActive || goToTarget)
    {
        stopGotoTarget();
        return true;
    }
    return false;
}

void Axis::startSlew(uint64_t rate, bool directionArg)
{
    stepTimer.stop();
    setDirection(directionArg);
    slewActive = true;
    setMicrostep(TRACKER_MOTOR_MICROSTEPPING / 2);
    slewTimeOut.start(12000, true);
    currentSlewRate = rate;
    stepTimer.start(rate, true);
}

void Axis::stopSlew()
{
    slewActive = false;
    stepTimer.stop();
    slewTimeOut.stop();
    if (trackingActive)
    {
        requestTracking(rate.tracking, direction.tracking);
    }
}

void Axis::resumeSlewIfNeeded()
{
    // If we are in a goto state but the stepTimer isn't running (e.g., paused by
    // the intervalometer during capture), restart the stepTimer with the stored
    // slew rate.
    if (goToTarget && !slewActive && currentSlewRate != 0)
    {
        slewActive = true;
        // Do not restart the generic slew timeout here; gotoTarget-style moves
        // should run until the ISR reports the target reached.
        stepTimer.start(currentSlewRate, true);
    }
}

void Axis::setAxisTargetCount(int64_t count)
{
    targetCount = count;
}

int64_t Axis::getAxisTargetCount()
{
    return targetCount;
}

void Axis::resetAxisCount()
{
    axisCountValue = 0;
}

void Axis::setAxisCount(int64_t count)
{
    axisCountValue = count;
}

int64_t Axis::getAxisCount()
{
    return axisCountValue;
}

void Axis::setDirection(bool directionArg)
{
    direction.absolute = directionArg;
    driver->setDirection(directionArg ^ invertDirectionPin);
}

void Axis::setMicrostep(uint16_t microstep)
{
    if (microStep != microstep)
    {
        microStep = microstep;
        driver->setMicrosteps(microstep);
    }
}

void Axis::print_status()
{
    driver->print_status();
}
