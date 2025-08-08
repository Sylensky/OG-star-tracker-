#ifndef _MSX_MOTOR_DRIVER_H_
#define _MSX_MOTOR_DRIVER_H_ 1

#include "motor_driver.h"

class MSxMotorDriver : public MotorDriver
{
  public:
    MSxMotorDriver(uint8_t ms1Pin, uint8_t ms2Pin, uint8_t dirPin);
    ~MSxMotorDriver()
    {
    }
    void setMicrosteps(uint16_t microsteps);
    void setDirection(bool direction);
    void print_status();

  private:
    uint8_t _ms1Pin;
    uint8_t _ms2Pin;
    uint8_t _dirPin;
};

#endif /* _MSX_MOTOR_DRIVER_H_ */
