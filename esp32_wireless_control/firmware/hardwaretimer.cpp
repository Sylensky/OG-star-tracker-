#include "esp32-hal-timer.h"

#include "hardwaretimer.h"


HardwareTimer::HardwareTimer(uint64_t frequency) {
  timer_pointer = timerBegin(frequency);
}
HardwareTimer::HardwareTimer(uint64_t frequency, void (*functionToCall)()) {
  this->ISR_Function = functionToCall;
  timer_pointer = timerBegin(frequency);
  timerAttachInterrupt(timer_pointer, ISR_Function);
}
void HardwareTimer::attachInterupt(void (*functionToCall)()){
  this->ISR_Function = functionToCall;
  timerAttachInterrupt(timer_pointer, ISR_Function);
}

void HardwareTimer::start(uint64_t alarmValue, bool autoReload) {
  timerAlarm(timer_pointer, alarmValue, true, 0);
}

void HardwareTimer::stop() {
  timerStop(timer_pointer);
}

void HardwareTimer::setCountValue(uint64_t countValue) {
  timerWrite(timer_pointer, countValue);
}
