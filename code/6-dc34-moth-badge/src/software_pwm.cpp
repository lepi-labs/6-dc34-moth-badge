#include "software_pwm.hpp"


void setup_pwm(pwm_pin **pwm_pins, size_t count) {
  for (size_t i = 0; i < count; i++) {
    pwm_pins[i]->value = pwm_pins[i]->invert ? 0 : MAX_BRIGHTNESS;
    pinMode(pwm_pins[i]->pin_num, OUTPUT);
    digitalWrite(pwm_pins[i]->pin_num, pwm_pins[i]->invert);
  }
}


void handle_pwm(pwm_pin **pwm_pins, size_t count) {
  // number of microseconds since last run of this function
  static uint64_t last_tick_us = 0;
  // number of microseconds since last run of the PWM routine
  static uint8_t last_run_us = 0;
  // number of microseconds that must pass between PWM routine runs
  const uint8_t pwm_interval = 50;
  // number of routines in a cycle. there will be roughly
  // pwmInterval * pwmMax us between cycles.
  const uint8_t pwm_max = MAX_BRIGHTNESS;

  last_tick_us = micros();
  static uint8_t pwm_tick = 0;
  if (micros() - last_run_us >= pwm_interval) {
    pwm_tick++;
    if (pwm_tick < pwm_max) {
      for (size_t i = 0; i < count; i++) {
        if (pwm_tick >= pwm_pins[i]->value) {
          digitalWrite(pwm_pins[i]->pin_num, pwm_pins[i]->invert);
        }
      }
    } else {
      for (size_t i = 0; i < count; i++) {
        if (pwm_pins[i]->value > 0) {
          digitalWrite(pwm_pins[i]->pin_num, !pwm_pins[i]->invert);
        }
      }
      pwm_tick = 0;
    }
    last_run_us = last_tick_us;
  }
}