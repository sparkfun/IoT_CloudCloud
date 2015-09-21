/**
 * CloudDemo
 * Shawn Hymel @ SparkFun Electronics
 * September 18, 2015
 *
 * Demo controller for cloud lighting.
 */

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "States.h"

// Pin definitions
#define LED_PIN           5
#define CLOUD_A_PIN       12
#define CLOUD_B_PIN       13
#define DEMO_BTN_PIN      V0
#define LCD_PIN           V9

// WiFi parameters
const char WIFI_SSID[] = "sparkfun-guest";
const char WIFI_PSK[] = "sparkfun6333";
const char BLYNK_AUTH[] = "b8960ce24e324ce9ade3490034bf6215";

// Other constants
const byte SERIAL_SOF = 0xA5;
const byte SERIAL_EOF = 0x5A;
const int OUT_BUF_MAX = 7;         // bytes
const byte COMM_RGB = 1;
const byte COMM_DISCO = 2;
const byte COMM_BLUE_SKY = 3;
const byte COMM_NIGHT = 4;
const byte COMM_OVERCAST = 5;
const byte COMM_GOLDEN = 6;
const byte COMM_SNOW = 7;
const byte COMM_LIGHTNING = 8;

// Global variables
WidgetLCD lcd(LCD_PIN);
WeatherType current_weather;
unsigned long led_time;
byte out_msg[OUT_BUF_MAX];

/****************************************************************
 * Setup
 ***************************************************************/
void setup() {
  
  uint8_t led = 0;

  // Set up status LED
  pinMode(LED_PIN, OUTPUT);

  // Set up serial
  Serial.begin(9600);

  // Start Blynk and wait for connection
  Blynk.begin(BLYNK_AUTH, WIFI_SSID, WIFI_PSK);
  while ( Blynk.connect() == false ) {
    led ^= 0x01;
    digitalWrite(LED_PIN, led);
    delay(200);
  }
  digitalWrite(LED_PIN, HIGH);
  
  // Define initial state
  current_weather = WEATHER_OVERCAST;
  lcd.clear();
  led_time = millis();
}

/****************************************************************
 * Loop
 ***************************************************************/
void loop() {
  Blynk.run();
}

/****************************************************************
 * Blynk Callbacks
 ***************************************************************/

// Callback when demo button is pushed
BLYNK_WRITE(DEMO_BTN_PIN) {
  memset(out_msg, 0, 3);
  if ( (param.asInt() == 1) ) {
    lcd.clear();
    switch ( current_weather ) {
      case WEATHER_BLUE_SKY:
        current_weather = WEATHER_NIGHT;
        lcd.print(0, 0, "NIGHT");
        out_msg[0] = COMM_NIGHT;
        break;
        
      case WEATHER_NIGHT:
        current_weather = WEATHER_OVERCAST;
        lcd.print(0, 0, "OVERCAST");
        out_msg[0] = COMM_OVERCAST;
        break;
        
      case WEATHER_OVERCAST:
        current_weather = WEATHER_GOLDEN;
        lcd.print(0, 0, "GOLDEN");
        out_msg[0] = COMM_GOLDEN;
        break;
        
      case WEATHER_GOLDEN:
        current_weather = WEATHER_SNOW;
        lcd.print(0, 0, "SNOW");
        out_msg[0] = COMM_SNOW;
        break;
        
      case WEATHER_SNOW:
        current_weather = WEATHER_LIGHTNING;
        lcd.print(0, 0, "STORM");
        out_msg[0] = COMM_LIGHTNING;
        break;
        
      case WEATHER_LIGHTNING:
      default:
        current_weather = WEATHER_BLUE_SKY;
        lcd.print(0, 0, "BLUE SKY");
        out_msg[0] = COMM_BLUE_SKY;
        break;
    }
    transmitMessage(out_msg, 3);
  }
}

/****************************************************************
 * Transmitter
 ***************************************************************/
void transmitMessage(byte msg[], uint8_t len) {
  
  int i;
  byte cs;
  byte *out_buf;
  uint8_t buf_size;
  
  // If message is greater than max size, only xmit max bytes
  if ( len > OUT_BUF_MAX ) {
    len = OUT_BUF_MAX;
  }
  
  // Full buffer is message + SOF, EOF bytes
  buf_size = len + 2;
  
  // Create the output buffer with BEGIN, SOF, CS, and EOF
  out_buf = (byte*)malloc(buf_size * sizeof(byte));
  out_buf[0] = SERIAL_SOF;
  memcpy(out_buf + 1, msg, len);
  out_buf[buf_size - 1] = SERIAL_EOF;
  
  // Transmit buffer
  for ( i = 0; i < buf_size; i++ ) {
    Serial.write(out_buf[i]);
  }
  
  // Free some memory
  free(out_buf);
}
  
