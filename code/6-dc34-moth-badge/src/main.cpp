#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <EEPROM.h>
#include <FastLED.h>

#include "software_pwm.hpp"

#define EYE_R_R PIN_PB0
#define EYE_R_G PIN_PA6
#define EYE_R_B PIN_PB1
#define EYE_L_R PIN_PA7
#define EYE_L_G PIN_PB3
#define EYE_L_B PIN_PB2
#define MODE_SW PIN_PA4

// void timer_init() {
//   // this is the value the timer resets at (the top of the sawtooth wave).
//   // the interrupt will fire when this is reached.
//   const uint16_t timer_top = 0x0050;

//   // set prescaler
//   TCB0_CTRLA |= TCB_CLKSEL_CLKDIV1_gc;
//   // Write TOP value to compare/capture register
//   TCB0_CCMP = timer_top;
//   // enable counter
//   TCB0_CTRLA |= TCB_ENABLE_bm;
//   // enable the interrupt
//   TCB0_INTCTRL |= TCB_CAPT_bm;
// }

// ISR(TCB0_INT_vect) {
//   static bool status = false;
//   digitalWrite(EYE_R_R, status);
//   digitalWrite(EYE_L_R, status);
//   status = !status;
// }

const uint8_t MODE_ADDR = 0;
const uint16_t UPDATE_WAIT_MS = 3000;
uint8_t current_mode = 0;

pwm_pin eye_l_r { EYE_L_R, 0, true };
pwm_pin eye_l_g { EYE_L_G, 0, true };
pwm_pin eye_l_b { EYE_L_B, 0, true };
pwm_pin eye_r_r { EYE_R_R, 0, true };
pwm_pin eye_r_g { EYE_R_G, 0, true };
pwm_pin eye_r_b { EYE_R_B, 0, true };
pwm_pin *pwm_pins[] = {
  &eye_l_r,
  &eye_l_g,
  &eye_l_b,
  &eye_r_r,
  &eye_r_g,
  &eye_r_b
};
constexpr size_t pwm_pin_count = sizeof(pwm_pins) / sizeof(pwm_pin*);

#define EYE_L_RGB &eye_l_r, &eye_l_g, &eye_l_b
#define EYE_R_RGB &eye_r_r, &eye_r_g, &eye_r_b
void set_rgb_led(pwm_pin *pin_r, pwm_pin *pin_g, pwm_pin *pin_b, uint8_t r, uint8_t g, uint8_t b) {
  pin_r->value = (uint8_t)((uint16_t)r * MAX_BRIGHTNESS / 255);
  pin_g->value = (uint8_t)((uint16_t)g * MAX_BRIGHTNESS / 255);
  pin_b->value = (uint8_t)((uint16_t)b * MAX_BRIGHTNESS / 255);
}

void mode_red() {
  set_rgb_led(EYE_L_RGB, 100, 0, 0);
  set_rgb_led(EYE_R_RGB, 100, 0, 0);
}

void mode_green() {
  set_rgb_led(EYE_L_RGB, 0, 100, 0);
  set_rgb_led(EYE_R_RGB, 0, 100, 0);
}

void mode_blue() {
  set_rgb_led(EYE_L_RGB, 0, 0, 100);
  set_rgb_led(EYE_R_RGB, 0, 0, 100);
}

void mode_rainbow() {
  static CHSV hsv_l(0, 255, 100);
  static CHSV hsv_r(40, 255, 100);
  CRGB rgb_l(hsv_l);
  CRGB rgb_r(hsv_r);
  set_rgb_led(EYE_L_RGB, rgb_l.r, rgb_l.g, rgb_l.b);
  set_rgb_led(EYE_R_RGB, rgb_r.r, rgb_r.g, rgb_r.b);
  hsv_l.h++;
  hsv_r.h++;
}

void (*MODES[])() = {
  mode_red,
  mode_green,
  mode_blue,
  mode_rainbow
};
constexpr int NUM_MODES = sizeof(MODES) / sizeof(void (*)());

void setup() {
  setup_pwm(pwm_pins, pwm_pin_count);
  pinMode(MODE_SW, INPUT_PULLUP);

  EEPROM.get(MODE_ADDR, current_mode);
  if (current_mode >= NUM_MODES) {
    current_mode = 0;
  }
}

void loop() {
  handle_pwm(pwm_pins, pwm_pin_count);
  
  const uint16_t anim_delay = 1000 / 60;
  static uint64_t last_run = 0;
  if (millis() - last_run >= anim_delay) {
    MODES[current_mode]();
    last_run = millis();
  }

  static bool was_pressed = false;
  bool currently_pressed = digitalRead(MODE_SW) == LOW;
  static bool update_pending = false;
  static unsigned long last_update = 0;
  if (!was_pressed && currently_pressed) {
    was_pressed = true;
  } else if (was_pressed && !currently_pressed) {
    was_pressed = false;
    current_mode++;
    current_mode %= NUM_MODES;
    update_pending = true;
    last_update = millis();
  } else if (update_pending && millis() - last_update >= UPDATE_WAIT_MS) {
    update_pending = false;
    EEPROM.put(MODE_ADDR, current_mode);
  }
}
