#include "esp32-hal-gpio.h"
#include "intervalometer.h"

Intervalometer intervalometer(INTERV_PIN);

void intervalometer_ISA() {
  intervalometer.nextState = true;
}



Intervalometer::Intervalometer(uint8_t triggerPinArg)
  : intervalometerTimer(2000, intervalometer_ISA) {  //2kHz resolution of 0.5 ms
  currentState = INACTIVE;
  nextState = false;
  intervalometerActive = false;
  readPresetsFromEEPROM();
  timerStarted = false;
  triggerPin = triggerPinArg;
}

void Intervalometer::run() {
  switch (currentState) {
    case INACTIVE:
      intervalometerActive = false;
      exposures_taken = 0;
      frames_taken = 0;
      if (!currentSettings.post_tracking_on) {
        ra_axis.stopTracking();
      }
      break;
    case PRE_DELAY:
      if (!timerStarted) {
        if ((currentSettings.mode == DAY_TIME_LAPSE || currentSettings.mode == DAY_TIME_LAPSE_PAN) && ra_axis.trackingActive) {
          ra_axis.stopTracking();
        } else if ((currentSettings.mode == LONG_EXPOSURE_MOVIE || currentSettings.mode == LONG_EXPOSURE_STILL) && !ra_axis.trackingActive) {
          sendError("TRACKING NOT ACTIVE!! Please start");
          currentState = INACTIVE;
          break;
        }
        ra_axis.counterActive = currentSettings.mode == LONG_EXPOSURE_MOVIE ? true : false;
        intervalometerTimer.start(2000 * currentSettings.pre_delay_time, false);
        timerStarted = false;
        intervalometerActive = true;
      }
      if (nextState) {
        nextState = false;
        timerStarted = false;
        currentState = CAPTURE;
      }
      break;
    case CAPTURE:  //add capture for day time

      if (!timerStarted) {
        ra_axis.counterActive = currentSettings.mode == LONG_EXPOSURE_MOVIE ? true : false;
        digitalWrite(triggerPin, HIGH);
        intervalometerTimer.start(2000 * currentSettings.exposure_time, false);
        timerStarted = true;
      }
      if (nextState) {
        digitalWrite(triggerPin, LOW);
        nextState = false;
        timerStarted = false;
        exposures_taken++;
        currentState = currentSettings.dither ? DITHER : currentSettings.mode == DAY_TIME_LAPSE_PAN ? PAN
                                                                                                    : DELAY;
      }
      break;
    case DITHER:
      if (exposures_taken % currentSettings.dither_frequency == 0) {
        runDither(); //program dither.
        currentState = DELAY;
      }
      break;
    case PAN:
      //work out steps and axis timer preset count
      break;
    case DELAY:
      if (exposures_taken >= currentSettings.exposures) {
        currentState = currentSettings.mode != LONG_EXPOSURE_MOVIE ? INACTIVE : frames_taken < currentSettings.frames ? REWIND
                                                                                                                      : INACTIVE;
      } else {
        if (!timerStarted) {
          intervalometerTimer.start(2000 * currentSettings.delay_time, false);
          timerStarted = true;
        }
        if (nextState) {
          nextState = false;
          timerStarted = false;
          currentState = CAPTURE;
        }
      }

      break;
    case REWIND:
      if (!ra_axis.goToTarget) {
        exposures_taken = 0;
        frames_taken++;
        ra_axis.setAxisTargetCount(0);
        ra_axis.goToTarget = true;
        ra_axis.startSlew(ra_axis.tracking_rate/5, !ra_axis.trackingDirection); //rewind at 10 x tracking rate.
        exposures_taken = 0;
        frames_taken++;
      }
      if (!ra_axis.slewActive) {
        currentState = CAPTURE;
      }
      break;
  }
}

void Intervalometer::saveSettingsToPreset(uint8_t preset) {
  presets[preset] = currentSettings;
  savePresetsToEEPPROM();
}

void Intervalometer::readSettingsFromPreset(uint8_t preset) {
  currentSettings = presets[preset];
}

void Intervalometer::savePresetsToEEPPROM() {
  writeObjectToEEPROM(PRESETS_EEPROM_START_LOCATION, presets);
}

void Intervalometer::readPresetsFromEEPROM() {
  readObjectFromEEPROM(PRESETS_EEPROM_START_LOCATION, presets);
}

uint16_t Intervalometer::getStepsPerTenPixels() {
  //add 0.5 to round up float to nearest int while casting
  return (int)(((getArcsec_per_pixel() * 10.0) / ARCSEC_PER_STEP) + 0.5);
}

float Intervalometer::getArcsec_per_pixel() {
  //div pixel size by 100 since we multiplied it by 100 in html page
  return (((float)currentSettings.pixel_size / 100.0) / currentSettings.focal_length) * 206.265;
}

void Intervalometer::runDither() {
  //pass
}

template<class T> int Intervalometer::writeObjectToEEPROM(int address, const T& object) {
  const byte* p = (const byte*)(const void*)&object;
  unsigned int i;
  for (i = 0; i < sizeof(object); i++)
    EEPROM.write(address++, *p++);
  return i;
}

template<class T> int Intervalometer::readObjectFromEEPROM(int address, T& object) {
  byte* p = (byte*)(void*)&object;
  unsigned int i;
  for (i = 0; i < sizeof(object); i++)
    *p++ = EEPROM.read(address++);
  return i;
}
