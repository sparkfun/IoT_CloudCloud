/**
 * CloudCloud
 * Shawn Hymel @ SparkFun Electronics
 * September 17, 2015
 *
 * Use Blynk app to set up some buttons and LEDs. See the pin
 * definitions below.
 */

// Debug
#define DEBUG    1

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "States.h"

// Debug to Blynk Serial
#if DEBUG
#define BLYNK_PRINT Serial
#endif

// Pin definitions
#define LED_PIN           5
#define WEATHER_BTN_PIN   V0
#define WEATHER_LED_PIN   V1
#define RGB_BTN_PIN       V2
#define RGB_LED_PIN       V3
#define DISCO_BTN_PIN     V4
#define DISCO_LED_PIN     V5
#define ZERGBA_PIN        V8
#define LCD_PIN           V9

// WiFi parameters
const char WIFI_SSID[] = "sparkfun-guest";
const char WIFI_PSK[] = "sparkfun6333";
const char BLYNK_AUTH[] = "b8960ce24e324ce9ade3490034bf6215";

// Location data (Niwot, CO)
const float LATITUDE = 40.090554;
const float LONGITUDE = -105.184861;

// Remote site information
const char http_site[] = "api.openweathermap.org";
const int http_port = 80;

// Other constants
const int TIMEOUT = 300;    // ms
const int BUFFER_SIZE = 22; // bytes
const int ICON_SIZE = 4;    // bytes
const int DT_SIZE = 12;     // bytes

// Global variables
WidgetLED weather_led(WEATHER_LED_PIN);
WidgetLED rgb_led(RGB_LED_PIN);
WidgetLED disco_led(DISCO_LED_PIN);
WidgetLCD lcd(LCD_PIN);
CloudState cloud_state;
WiFiClient client;
WeatherType current_weather;
unsigned long time;
bool force_update;
uint8_t color_r;
uint8_t color_g;
uint8_t color_b;

/****************************************************************
 * Setup
 ***************************************************************/
void setup() {
  
  uint8_t led = 0;

  // Set up status LED
  pinMode(LED_PIN, OUTPUT);
  
  // Set up debug serial console
#if DEBUG
  Serial.begin(9600);
  Serial.println("Blynk test");
#endif

  // Start Blynk and wait for connection
  Blynk.begin(BLYNK_AUTH, WIFI_SSID, WIFI_PSK);
  while ( Blynk.connect() == false ) {
    led ^= 0x01;
    digitalWrite(LED_PIN, led);
    delay(200);
  }
  digitalWrite(LED_PIN, HIGH);
  
  // Define initial state
  cloud_state = CLOUD_WEATHER;
  setLEDs();
  time = millis();
  force_update == false;
  color_r = 0;
  color_g = 0;
  color_b = 0;
  lcd.clear();
}

/****************************************************************
 * Loop
 ***************************************************************/
void loop() {
  Blynk.run();
  
  // Perform LED functions
  switch ( cloud_state ) {
    
    // Reflect weather data in clouds
    case CLOUD_WEATHER:
      doWeather();
      break;
      
    // Pull in RGB data from Blynk
    case CLOUD_RGB:
      // Do RGB
      break;
      
    // Disco party!
    case CLOUD_DISCO:
      // Do Disco
      break;
      
    // Unknown state
    default:
      break;
  }
}

/****************************************************************
 * Blynk Callbacks
 ***************************************************************/

// Callback when weather button is pushed
BLYNK_WRITE(WEATHER_BTN_PIN) {
  if ( (param.asInt() == 1) && (cloud_state != CLOUD_WEATHER) ) {
#if DEBUG
    Serial.println("Switching to WEATHER mode");
#endif
    cloud_state = CLOUD_WEATHER;
    setLEDs();
    force_update = true;
  }
}

// Callback when RGB button is pushed
BLYNK_WRITE(RGB_BTN_PIN) {
  if ( (param.asInt() == 1) && (cloud_state != CLOUD_RGB) ) {
#if DEBUG
    Serial.println("Switching to RGB mode");
#endif
    cloud_state = CLOUD_RGB;
    setLEDs();
  }
}

// Callback when Disco button is pushed
BLYNK_WRITE(DISCO_BTN_PIN) {
  if ( (param.asInt() == 1) && (cloud_state != CLOUD_DISCO) ) {
#if DEBUG
    Serial.println("Switching to DISCO mode");
#endif
    cloud_state = CLOUD_DISCO;
    setLEDs();
  }
}

// Callback when the ZeRGBa changes
BLYNK_WRITE(ZERGBA_PIN) {
  color_r = param[0].asInt();
  color_g = param[1].asInt();
  color_b = param[2].asInt();
#if DEBUG
  Serial.print("Color changed (R,G,B): (");
  Serial.print(color_r);
  Serial.print(",");
  Serial.print(color_g);
  Serial.print(",");
  Serial.print(color_b);
  Serial.println(")");
#endif
}

// Set app LEDs
void setLEDs() {
  
  // Turn them all off
  weather_led.off();
  rgb_led.off();
  disco_led.off();
  lcd.clear();
  
  // Only turn on the one we want
  switch ( cloud_state ) {
    case CLOUD_WEATHER:
      weather_led.on();
      lcd.print(0, 0, "WEATHER");
      break;
    case CLOUD_RGB:
      rgb_led.on();
      lcd.print(0, 0, "RGB");
      break;
    case CLOUD_DISCO:
      disco_led.on();
      lcd.print(0, 0, "DISCO");
      break;
    default:
      break;
  }
}

/****************************************************************
 * Weather Functions
 ***************************************************************/

// Every 30 seconds, pull weather data and update LEDs
void doWeather() {
  
  // Only update every 30 seconds
  if ( ((millis() - time) >= 30000) || force_update ) {
    force_update = false;
    time = millis();
    lcd.clear();
    lcd.print(0, 0, "WEATHER");
    
    // Get weather data
    current_weather = getWeather();
  
    // Display weather on clouds
#if DEBUG
    Serial.println();
    Serial.print("Weather: ");
#endif
    switch ( current_weather ) {
    
      // Cannot get weather data
      case WEATHER_ERROR:
        lcd.print(0, 1, "ERROR");
#if DEBUG
        Serial.println("ERROR");
#endif
        break;
      
      // Clear skies (day)
      case WEATHER_BLUE_SKY:
        lcd.print(0, 1, "BLUE SKY");
#if DEBUG
        Serial.println("Blue skies");
#endif
        break;
      
      // Clear skies (night)
      case WEATHER_NIGHT:
        lcd.print(0, 1, "NIGHT");
#if DEBUG
        Serial.println("Starry night");
#endif
        break;
      
      // Lots of clouds
      case WEATHER_OVERCAST:
        lcd.print(0, 1, "OVERCAST");
#if DEBUG
        Serial.println("Overcast");
#endif
        break;
      
      // Dawn or dusk
      case WEATHER_GOLDEN:
        lcd.print(0, 1, "GOLDEN");
#if DEBUG
        Serial.println("Golden hour");
#endif
        break;
      
      // It's snowing!
      case WEATHER_SNOW:
        lcd.print(0, 1, "SNOW");
#if DEBUG
        Serial.println("Snow");
#endif
        break;
      
      // Storm
      case WEATHER_LIGHTNING:
        lcd.print(0, 1, "STORM");
#if DEBUG
        Serial.println("Stormy");
#endif
        break;
      
      // Unknown weather type
      case WEATHER_UNKNOWN:
      default:
        lcd.print(0, 1, "UNKNOWN");
#if DEBUG
        Serial.println("UNKNOWN");
#endif
        break;
    }
  }
}

// Perform an HTTP GET request to openweathermap.org
WeatherType getWeather() {
  
  unsigned long time = millis();
  unsigned long dt = 0;
  unsigned long sunrise = 0;
  unsigned long sunset = 0;
  char buf[BUFFER_SIZE];
  char icon[ICON_SIZE];
  char dt_str[DT_SIZE];
  char c;
  char* dummy;
  bool golden = false;
  ParserState parser_state = PARSER_START;
  WeatherType condition = WEATHER_UNKNOWN;
  
  // Flush read buffer
  if ( client.available() ) {
    client.read();
  }
  
  // Set string buffers to null
  memset(icon, 0, ICON_SIZE);
  memset(dt_str, 0, DT_SIZE);
  
  // Attempt to make a connection to the remote server
  if ( !client.connect(http_site, http_port) ) {
    return WEATHER_ERROR;
  }
  
  // Construct HTTP GET request
  String cmd = "GET /data/2.5/weather?lat=";
  cmd += String(LATITUDE);
  cmd += "&lon=";
  cmd += String(LONGITUDE);
  cmd += " HTTP/1.1\nHost: ";
  cmd += http_site;
  cmd += "\nConnection: close\n\n";
  
  // Send out GET request to site
  client.print(cmd);
  
  // Parse data
  while ( (millis() - time) < TIMEOUT ) {
    
    // Get web page data and push to window buffer
    if ( client.available() ) {
      c = client.read();
#if DEBUG
      Serial.print(c);
#endif
      pushOnBuffer(c, buf, BUFFER_SIZE);
      time = millis();
    }
    
    // Depending on the state of the parser, look for string
    switch ( parser_state ) {
      case PARSER_START:
        if ( memcmp(buf, "\"weather\":", 10) == 0 ) {
          parser_state = PARSER_WEATHER;
        }
        break;
        
      case PARSER_WEATHER:
        if ( memcmp(buf, "\"icon\":\"", 8) == 0 ) {
          parser_state = PARSER_DT;
          memcpy(icon, buf + 8, 3);
        }
        break;
        
      case PARSER_DT:
        if ( memcmp(buf, "\"dt\":", 5) == 0 ) {
          parser_state = PARSER_SUNRISE;
          memcpy(dt_str, buf + 5, 10); // This assumes 10 digits!
          dt = strtol(dt_str, &dummy, 10);
        }
        break;
        
      case PARSER_SUNRISE:
        if ( memcmp(buf, "\"sunrise\":", 10) == 0 ) {
          parser_state = PARSER_SUNSET;
          memcpy(dt_str, buf + 10, 10); // Assumes 10 digits!
          sunrise = strtol(dt_str, &dummy, 10);
        }
        break;
        
        case PARSER_SUNSET:
        if ( memcmp(buf, "\"sunset\":", 9) == 0 ) {
          parser_state = PARSER_SUNSET;
          memcpy(dt_str, buf + 9, 10); // Assumes 10 digits!
          sunset = strtol(dt_str, &dummy, 10);
        }
        break;
        
      default:
        break;
    }
  }
  
  // Print our newly found icon and times
#if DEBUG
  Serial.print("Icon: ");
  Serial.println(icon);
  Serial.print("Time: ");
  Serial.println(dt, DEC);
  Serial.print("Sunrise: ");
  Serial.println(sunrise, DEC);
  Serial.print("Sunset: ");
  Serial.println(sunset, DEC);
#endif
  
  // Make sure we have a legitimate icon and times
  if ( (strcmp(icon, "") == 0) || (dt == 0) || 
        (sunrise == 0) || (sunset == 0) ) {
    return WEATHER_ERROR;
  }
  
  // See if we are in the golden hour
  if ( ((dt - sunrise) >= 0) && ((dt - sunrise) <= 1800) ) {
    golden = true;
  }
  if ( ((sunset - dt) >= 0) && ((sunset - dt) <= 1800) ) {
    golden = true;
  }
  
  // Lookup what to display based on icon
  // http://openweathermap.org/weather-conditions
  if ( (strcmp(icon, "01d") == 0) || 
        (strcmp(icon, "02d") == 0) ) {
    if ( golden ) {
      return WEATHER_GOLDEN;
    } else {
      return WEATHER_BLUE_SKY;
    }
  } else if ( (strcmp(icon, "01n") == 0) || 
              (strcmp(icon, "02n") == 0) ) {
    if ( golden ) {
      return WEATHER_GOLDEN;
    } else {
      return WEATHER_NIGHT;
    }
  } else if ( (strcmp(icon, "03d") == 0) || 
              (strcmp(icon, "03n") == 0) ||
              (strcmp(icon, "04d") == 0) ||
              (strcmp(icon, "04n") == 0) ||
              (strcmp(icon, "09d") == 0) ||
              (strcmp(icon, "09n") == 0) ||
              (strcmp(icon, "10d") == 0) ||
              (strcmp(icon, "10n") == 0) ) {
    return WEATHER_OVERCAST;
  } else if ( (strcmp(icon, "11d") == 0) ||
              (strcmp(icon, "11n") == 0) ) {
    return WEATHER_LIGHTNING;
  } else if ( (strcmp(icon, "13d") == 0) ||
              (strcmp(icon, "13n") == 0) ) {
    return WEATHER_SNOW;
  } else if ( (strcmp(icon, "50d") == 0) ||
              (strcmp(icon, "50n") == 0) ) {
    return WEATHER_OVERCAST;
  } else {
    return WEATHER_UNKNOWN;
  }
}

// Shift buffer to the left by 1 and append new char at end
void pushOnBuffer(char c, char *buf, uint8_t len) {
  for ( uint8_t i = 0; i < len - 1; i++ ) {
    buf[i] = buf[i + 1];
  }
  buf[len - 1] = c;
}
