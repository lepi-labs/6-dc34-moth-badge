#pragma once

#include <Arduino.h>

#define MAX_BRIGHTNESS 100


typedef struct {
  uint8_t pin_num;
  uint8_t value;
  bool invert = false;
} pwm_pin;


void setup_pwm(pwm_pin **pwm_pins, size_t count);
void handle_pwm(pwm_pin **pwm_pins, size_t count);
