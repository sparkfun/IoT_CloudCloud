#ifndef STATES_H
#define STATES_H

// LED States
typedef enum {
  LED_RGB,
  LED_DISCO,
  LED_BLUE_SKY,
  LED_NIGHT,
  LED_OVERCAST,
  LED_GOLDEN,
  LED_SNOW,
  LED_LIGHTNING
} LEDState;

// Twinkle state machine
typedef enum {
  NIGHT_0,
  NIGHT_1,
  NIGHT_2,
  NIGHT_3,
  NIGHT_4,
  NIGHT_5,
  NIGHT_6
} NightState;

#endif // STATES_H
