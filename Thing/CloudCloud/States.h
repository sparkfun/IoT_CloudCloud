#ifndef STATES_H
#define STATES_H

// Cloud states
typedef enum {
  CLOUD_WEATHER,
  CLOUD_RGB,
  CLOUD_DISCO
} CloudState;

// Weather definitions
typedef enum {
  WEATHER_ERROR,
  WEATHER_BLUE_SKY,
  WEATHER_NIGHT,
  WEATHER_OVERCAST,
  WEATHER_GOLDEN,
  WEATHER_SNOW,
  WEATHER_LIGHTNING,
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

#endif // STATES_H
