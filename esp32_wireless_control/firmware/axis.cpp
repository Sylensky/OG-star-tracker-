#include "soc/gpio_struct.h"

#include <cmath>

#include "axis.h"
#include "functions/board_version/board_config.h"
#include "uart.h"

#if MICROSTEPPING_MOTOR_DRIVER == USE_MSx_PINS_MICROSTEPPING
#include "drivers/msx_motor_driver.h"
#elif MICROSTEPPING_MOTOR_DRIVER == USE_TMC_DRIVER_MICROSTEPPING
#include "drivers/tmc_motor_driver.h"
#else
#error Unknown Motor Driver
#endif

static MotorDriver* ra_driver = nullptr;
Axis ra_axis;
HardwareTimer slewTimeOut;
static uint8_t axis1StepPin = 0;

volatile bool ra_axis_step_phase = 0;

void IRAM_ATTR stepTimerRA_ISR()
{
    // ra ISR
    ra_axis_step_phase = !ra_axis_step_phase;
    if (ra_axis_step_phase)
    {
#ifdef BOARD_HAS_PIN_REMAP
        digitalWrite(axis1StepPin, HIGH);
#else
        GPIO.out_w1ts = (1 << axis1StepPin); // Set pin high
#endif
    }
    else
    {
#ifdef BOARD_HAS_PIN_REMAP
        digitalWrite(axis1StepPin, LOW);
#else
        GPIO.out_w1tc = (1 << axis1StepPin); // Set pin low
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

void initAxis()
{
    const BoardConfig& cfg = boardCfg();
    axis1StepPin = cfg.getAxis1Step();

    pinMode(axis1StepPin, OUTPUT);
    digitalWrite(axis1StepPin, LOW);
    pinMode(cfg.getEn12(), OUTPUT);
    digitalWrite(cfg.getEn12(), LOW);

#if MICROSTEPPING_MOTOR_DRIVER == USE_MSx_PINS_MICROSTEPPING
    ra_driver = new MSxMotorDriver(cfg.getRaMs1(), cfg.getRaMs2(), cfg.getAxis1Dir());
#elif MICROSTEPPING_MOTOR_DRIVER == USE_TMC_DRIVER_MICROSTEPPING
    ra_driver = new TmcMotorDriver(&Serial2, cfg.getAxis1Addr(), cfg.getTmcRSense(),
                                   cfg.getAxisRx(), cfg.getAxisTx());
#endif

    ra_axis.init(1, ra_driver, cfg.getAxis1Dir(), RA_INVERT_DIR_PIN);
    slewTimeOut.init(2000, &slewTimeOutTimer_ISR);
}

// Position class implementation
Position::Position(int degrees, int minutes, float seconds)
{
    arcseconds = toArcseconds(degrees, minutes, seconds);
}

float Position::toDegrees() const
{
    return arcseconds / 3600.0f;
}

int64_t Position::toArcseconds(int degrees, int minutes, float seconds)
{
    return (degrees * 3600) + (minutes * 60) + static_cast<int>(seconds);
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
#if SLEW_RAMP_ENABLE
        if (axis->rampActive)
        {
            axis->updateSlewRamp();
        }
#endif
        vTaskDelay(1);
    }
}

Axis::Axis()
    : axisCountValue(0), targetCount(0), goToTarget(false), slewActive(false),
      trackingActive(false), direction(), counterActive(false), rate(), position(0), stepTimer(),
      microStep(0), stepPin(0), dirPin(0), axisNumber(0), invertDirectionPin(false),
      driver(nullptr), startRequested(false)
#if SLEW_RAMP_ENABLE
      ,
      rampActive(false), rampStartRate(0), rampTargetRate(0), rampCurrentRate(0), rampStep(0),
      rampTotalSteps(0), rampingDown(false), rampLastUpdateMs(0)
#endif
{
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

void Axis::init(uint8_t axis, MotorDriver* motorDriver, uint8_t dirPinforAxis, bool invertDirPin)
{
    driver = motorDriver;
    axisNumber = axis;
    direction.tracking = c_DIRECTION;
    dirPin = dirPinforAxis;
    invertDirectionPin = invertDirPin;
    rate.tracking = trackingRates.getRate();

    pinMode(dirPin, OUTPUT);

    stepTimer.init(TIMER_APB_CLK_FREQ);
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
                      const Position& target, bool hemisphereDirection)
{
    setMicrostep(microstep);
    // delta in RA seconds-of-time (NOT angular arcseconds) TODO: investigate
    int64_t deltaRASeconds = target.arcseconds - current.arcseconds;

    print_out_nonl("deltaRASeconds: %lld\n", deltaRASeconds);

    int64_t absDelta = (deltaRASeconds < 0) ? -deltaRASeconds : deltaRASeconds;
    if (absDelta > RA_SECONDS_PER_FULL_REV / 2)
    {
        if (deltaRASeconds > 0)
        {
            deltaRASeconds = (deltaRASeconds - RA_SECONDS_PER_FULL_REV) % RA_SECONDS_PER_FULL_REV;
        }
        else
        {
            deltaRASeconds = (deltaRASeconds + RA_SECONDS_PER_FULL_REV) % RA_SECONDS_PER_FULL_REV;
        }
    }

    int64_t stepsPerRASecondAtMax = STEPS_PER_TRACKER_FULL_REV_INT / RA_SECONDS_PER_FULL_REV;
    int64_t stepsToMoveAtCurrentMicrostep =
        (deltaRASeconds * stepsPerRASecondAtMax * microstep) / MAX_MICROSTEPS;
    int64_t positionDeltaAtMax = deltaRASeconds * stepsPerRASecondAtMax;

    // Calculate motor direction based on hemisphere and movement direction
    // North hemisphere: direction=0 is LEFT (increasing RA), direction=1 is RIGHT (decreasing RA)
    // South hemisphere: direction=1 is LEFT (increasing RA), direction=0 is RIGHT (decreasing RA)
    bool motorDirection = (positionDeltaAtMax < 0) == hemisphereDirection;

    // For goto, set direction.absolute to make counter count in the correct direction
    // The ISR counter uses (direction.absolute XOR direction.tracking)
    // - XOR = 0: counter increments (for positive target)
    // - XOR = 1: counter decrements (for negative target)
    // For positive stepsToMove: want counter to increment, so direction.absolute =
    // direction.tracking For negative stepsToMove: want counter to decrement, so direction.absolute
    // != direction.tracking
    bool positionTrackingDirection =
        (positionDeltaAtMax >= 0) ? direction.tracking : !direction.tracking;

    print_out_nonl("stepsToMove: %lld (at microstep %d), positionDelta: %lld (at MAX)\n",
                   stepsToMoveAtCurrentMicrostep, microstep, positionDeltaAtMax);
    print_out_nonl(
        "hemisphereDirection: %d, motorDirection: %d, positionTrackingDir: %d (should move %s)\n",
        hemisphereDirection, motorDirection, positionTrackingDirection,
        (positionDeltaAtMax >= 0) ? "LEFT/EAST" : "RIGHT/WEST");

    // Set current position (normalized to MAX_MICROSTEPS) and prepare counter for relative movement
    setPosition(current.arcseconds * stepsPerRASecondAtMax);
    resetAxisCount();
    // Use signed target - counter will count up for positive, down for negative
    // Counter tracks actual motor steps at current microstep setting
    setAxisTargetCount(stepsToMoveAtCurrentMicrostep);

    if (targetCount != axisCountValue)
    {
        counterActive = true;
        goToTarget = true;
        stepTimer.stop();

        // Set direction.absolute equal to direction.tracking to make counter increment
        // Set physical motor direction
        // This is a workaround to move the motor in the correct direction while
        // keeping the counter logic consistent
        direction.absolute = positionTrackingDirection;
        driver->setDirection(motorDirection ^ invertDirectionPin);

        slewActive = true;
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
        stepTimer.start((2 * rate.tracking) / speed, true);
        print_out("Pan started: counterActive=%d, goToTarget=%d, targetCount=%lld", counterActive,
                  goToTarget, getAxisTargetCount());
    }

    return goToTarget;
}

bool Axis::stopPanByDegrees()
{
    if (slewActive || goToTarget)
    {
        counterActive = false;
        goToTarget = false;
        stopSlew();
        return true;
    }
    return false;
}

void Axis::startSlew(uint64_t targetRate, bool directionArg)
{
    stepTimer.stop();
    setDirection(directionArg);
    slewActive = true;
    setMicrostep(TRACKER_MOTOR_MICROSTEPPING / 2);
    slewTimeOut.start(12000, true);
#if SLEW_RAMP_ENABLE
    rampStartRate = targetRate * SLEW_RAMP_START_DIVISOR;
    rampTargetRate = targetRate;
    rampCurrentRate = rampStartRate;
    rampStep = 0;
    rampTotalSteps = SLEW_RAMP_STEPS;
    rampingDown = false;
    rampLastUpdateMs = millis();
    rampActive = true;
    stepTimer.start(rampStartRate, true);
#else
    stepTimer.start(targetRate, true);
#endif
}

void Axis::stopSlew()
{
#if SLEW_RAMP_ENABLE
    rampActive = false;
#endif
    slewActive = false;
    stepTimer.stop();
    slewTimeOut.stop();
    if (trackingActive)
    {
        requestTracking(rate.tracking, direction.tracking);
    }
}

#if SLEW_RAMP_ENABLE
void Axis::initiateSlewStop()
{
    if (!slewActive)
        return;
    if (rampActive && rampingDown)
        return; // Already ramping down

    // Start ramp-down from current actual speed
    // If ramp-up already finished, current speed == original target rate
    uint64_t currentSpeed = rampActive ? rampCurrentRate : rampTargetRate;
    rampStartRate = currentSpeed;
    rampTargetRate = currentSpeed * SLEW_RAMP_START_DIVISOR;
    rampCurrentRate = currentSpeed;
    rampStep = 0;
    rampTotalSteps = SLEW_RAMP_STOP_STEPS;
    rampingDown = true;
    rampLastUpdateMs = millis();
    rampActive = true;
}

void Axis::updateSlewRamp()
{
    if (!rampActive || !slewActive)
        return;

    uint32_t now = millis();
    if ((now - rampLastUpdateMs) < (uint32_t) SLEW_RAMP_INTERVAL_MS)
        return;
    rampLastUpdateMs = now;

    rampStep++;

    if (!rampingDown)
    {
        // Ramp-up: decrease alarm value (increase speed) toward rampTargetRate
        if (rampStep >= rampTotalSteps)
        {
            rampCurrentRate = rampTargetRate;
            stepTimer.setAlarm(rampTargetRate);
            rampActive = false;
        }
        else
        {
            float ratio = logf((float) rampTargetRate / (float) rampStartRate);
            float t = (float) rampStep / (float) rampTotalSteps;
            uint64_t newRate = (uint64_t) ((float) rampStartRate * expf(ratio * t) + 0.5f);
            // Clamp to target (avoid overshooting due to float rounding)
            if (newRate < rampTargetRate)
                newRate = rampTargetRate;
            rampCurrentRate = newRate;
            stepTimer.setAlarm(newRate);
        }
    }
    else
    {
        // Ramp-down: increase alarm value (decrease speed) then stop
        if (rampStep >= rampTotalSteps)
        {
            stopSlew();
        }
        else
        {
            float ratio = logf((float) rampTargetRate / (float) rampStartRate);
            float t = (float) rampStep / (float) rampTotalSteps;
            uint64_t newRate = (uint64_t) ((float) rampStartRate * expf(ratio * t) + 0.5f);
            // Clamp to maximum (avoid overshooting)
            if (newRate > rampTargetRate)
                newRate = rampTargetRate;
            rampCurrentRate = newRate;
            stepTimer.setAlarm(newRate);
        }
    }
}
#endif

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
