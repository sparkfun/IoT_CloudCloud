#ifndef STATES_H
#define STATES_H

// Weather definitions
typedef enum {
  WEATHER_ERROR,
  WEATHER_BLUE_SKY,
  WEATHER_NIGHT,
  WEATHER_OVERCAST,
  WEATHER_GOLDEN,
  WEATHER_SNOW,
  WEATHER_LIGHTNING,
  WEATHER_RAIN,
  WEATHER_MIST,
  WEATHER_RAINBOW,
  WEATHER_TWILIGHT,
  WEATHER_DARK,
  WEATHER_UNKNOWN
} WeatherType;

// Parsing state machine states
typedef enum {
  PARSER_START,
  PARSER_WEATHER,
  PARSER_DT,
  PARSER_SUNRISE,
  PARSER_SUNSET
} ParserState;

// shared constants for communication
const byte COMM_RGB       = 1;
const byte COMM_DISCO     = 2;
const byte COMM_BLUE_SKY  = 3;
const byte COMM_NIGHT     = 4;
const byte COMM_OVERCAST  = 5;
const byte COMM_GOLDEN    = 6;
const byte COMM_SNOW      = 7;
const byte COMM_LIGHTNING = 8;
const byte COMM_RAIN      = 9;
const byte COMM_MIST      = 10;
const byte COMM_RAINBOW   = 11;
const byte COMM_TWILIGHT  = 12;
const byte COMM_DARK      = 13;

#endif // STATES_H
