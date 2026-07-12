#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <EEPROM.h>
#include <FastLED.h>

volatile uint8_t eye_l_r = 0;
volatile uint8_t eye_l_g = 0;
volatile uint8_t eye_l_b = 0;
volatile uint8_t eye_r_r = 0;
volatile uint8_t eye_r_g = 0;
volatile uint8_t eye_r_b = 0;

// ISR for doing PWM
ISR(TCB0_INT_vect) {
  TCB0_INTFLAGS = TCB_CAPT_bm;

  static uint8_t cycle = 0;
  if (eye_l_r > cycle) PORTA_OUT &= ~PIN7_bm; else PORTA_OUT |= PIN7_bm;
  if (eye_l_g > cycle) PORTB_OUT &= ~PIN3_bm; else PORTB_OUT |= PIN3_bm;
  if (eye_l_b > cycle) PORTB_OUT &= ~PIN2_bm; else PORTB_OUT |= PIN2_bm;
  if (eye_r_r > cycle) PORTB_OUT &= ~PIN0_bm; else PORTB_OUT |= PIN0_bm;
  if (eye_r_g > cycle) PORTA_OUT &= ~PIN6_bm; else PORTA_OUT |= PIN6_bm;
  if (eye_r_b > cycle) PORTB_OUT &= ~PIN1_bm; else PORTB_OUT |= PIN1_bm;
  cycle++;
}

void mode_red() {
  eye_l_r = 128;
  eye_l_g = 0;
  eye_l_b = 0;
  eye_r_r = 128;
  eye_r_g = 0;
  eye_r_b = 0;
}

void mode_green() {
  eye_l_r = 0;
  eye_l_g = 128;
  eye_l_b = 0;
  eye_r_r = 0;
  eye_r_g = 128;
  eye_r_b = 0;
}

void mode_blue() {
  eye_l_r = 0;
  eye_l_g = 0;
  eye_l_b = 128;
  eye_r_r = 0;
  eye_r_g = 0;
  eye_r_b = 128;
}

void mode_rainbow() {
  static CHSV eye_l(0, 255, 100);
  static CHSV eye_r(30, 255, 100);

  CRGB eye_l_rgb(eye_l);
  CRGB eye_r_rgb(eye_r);
  eye_l_r = eye_l_rgb.r;
  eye_l_g = eye_l_rgb.g;
  eye_l_b = eye_l_rgb.b;
  eye_r_r = eye_r_rgb.r;
  eye_r_g = eye_r_rgb.g;
  eye_r_b = eye_r_rgb.b;

  eye_l.h++;
  eye_r.h++;
}

void (*MODES[])() = {
  mode_red,
  mode_green,
  mode_blue,
  mode_rainbow
};
constexpr int NUM_MODES = sizeof(MODES) / sizeof(void (*)());

uint8_t current_mode = 0;
const uint8_t MODE_ADDR = 0;
const uint16_t UPDATE_WAIT_MS = 3000;
bool mode_update_pending = false;
uint64_t mode_last_update_ms = 0;

// ISR for handling mode button
ISR(PORTA_PORT_vect) {
  PORTA_INTFLAGS = PIN4_bm;

  static uint64_t last_press_ms = 0;
  const uint64_t now_ms = millis();
  const uint64_t debounce_ms = 100;
  if (now_ms - last_press_ms < debounce_ms) {
    last_press_ms = now_ms;
    return;
  } else {
    last_press_ms = now_ms;
    current_mode = (current_mode + 1) % NUM_MODES;
    mode_update_pending = true;
    mode_last_update_ms = now_ms;
  }
}

void setup() {
  // configure LED pins
  PORTA_DIR |= PIN6_bm | PIN7_bm;
  PORTA_OUT |= PIN6_bm | PIN7_bm;
  PORTB_DIR |= PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
  PORTB_OUT |= PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
  // configure mode switch pin. since it is being used in pullup mode,
  // a rising edge will represent the button being released, and that's
  // what the ISR will be attached to.
  PORTA_DIR &= ~PIN4_bm;
  PORTA_PIN4CTRL |= PORT_PULLUPEN_bm;
  PORTA_PIN4CTRL |= PORT_ISC_RISING_gc;

  // setup timer for driving LEDs
  TCB0_CCMP = 0x008F; // ISR will run when counter reaches this value
  TCB0_CTRLA |= TCB_CLKSEL_DIV2_gc; // counter will increment at CPU / 2
  TCB0_CTRLA |= TCB_ENABLE_bm; // enable the counter
  TCB0_INTCTRL |= TCB_CAPT_bm; // enable the interrupt

  EEPROM.get(MODE_ADDR, current_mode);
  if (current_mode >= NUM_MODES) {
    current_mode = 0;
  }
}

void loop() {
  const uint16_t anim_delay = 1000 / 60;
  static uint64_t last_run = 0;
  if (millis() - last_run >= anim_delay) {
    MODES[current_mode]();
    last_run = millis();
  }

  if (mode_update_pending && millis() - mode_last_update_ms > UPDATE_WAIT_MS) {
    mode_update_pending = false;
    EEPROM.put(MODE_ADDR, current_mode);
  }
}
