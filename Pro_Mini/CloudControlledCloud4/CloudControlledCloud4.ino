/**
 * LED Control
 * Sarah Al-Mutlaq @ SparkFun Electronics
 * Shawn Hymel @ SparkFun Electronics
 * September 18, 2015
 *
 * Accepts commands from Thing on how to control LEDs.
 * grw 07/03/16 - scaled LED size down to single strand of 60 LED
 *                most LED functions were scaled by dividing by 5
 * grw 11/26/17 - updated to handle multiple weather conditions
 * grw 10/30/17 - added fog effect to mist, made sunset all golden
 * grw 06/29/18 - added twilight and dark mode
 *
 */

// Debug
#define DEBUG    0

#include <Adafruit_NeoPixel.h>
#include <AltSoftSerial.h>
#include <avr/power.h>
#include "States.h"

// LED States
typedef enum {
  LED_RGB,
  LED_DISCO,
  LED_BLUE_SKY,
  LED_NIGHT,
  LED_OVERCAST,
  LED_GOLDEN,
  LED_SNOW,
  LED_LIGHTNING,
  LED_RAIN,
  LED_MIST,
  LED_RAINBOW,
  LED_TWILIGHT,
  LED_DARK
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

// Pin definitions
#define CLOUD_A_PIN 6
#define PIXELS 60

// Other constants
const int TWINKLE_DELAY = 400;    // ms
const int FOG_MAX = 128;    // bright white rgb 128, 128, 128
const int FOG_DELAY = 200; // ms
const int IN_BUF_MAX = 7;         // bytes
const byte SERIAL_SOF = 0xA5;
const byte SERIAL_EOF = 0x5A;
const int TIMEOUT = 100;          // ms
const int DISCO_DELAY = 85;       // ms

// Global variables
//SoftwareSerial softy(4, 5); // RX, TX
AltSoftSerial softy;  // RX = 8, TX = 9, do not use pin 10
unsigned long led_time;

Adafruit_NeoPixel strip_a = Adafruit_NeoPixel(PIXELS, CLOUD_A_PIN, NEO_GRB + NEO_KHZ800);

LEDState led_state;
NightState night_state;
int night_i;
int storm_state;
int storm_delay;
int disco_state;
int disco_delay;
byte in_msg[IN_BUF_MAX];
uint8_t bytes_received;
uint8_t color_r;
uint8_t color_g;
uint8_t color_b;


/****************************************************************
 * Setup
 ***************************************************************/
void setup() {
  
  // Set up debug serial console
#if DEBUG
  Serial.begin(9600);
  Serial.println("LED Control");
#endif

  // Set up software serial
  softy.begin(9600);
  
  // Configure LED pins
  pinMode(CLOUD_A_PIN, OUTPUT);


  // Define initial state
  led_time = millis();
  led_state = LED_DISCO;
  storm_state = 0;
  disco_state = 0;
  disco_delay = DISCO_DELAY;
  night_state = NIGHT_0;
  night_i = 1;
  color_r = 0;
  color_g = 0;
  color_b = 0;

  
  // Set initial random delay
  pinMode(A0, INPUT);
  randomSeed(analogRead(A0));
  storm_delay = random(900, 1500);
  
  // Clear LED strips
  strip_a.begin(); 
  strip_a.show();
  clearCloud();
}

/****************************************************************
 * Loop
 ***************************************************************/
void loop() {
  doLED();
  bytes_received = receiveMessage(in_msg, TIMEOUT);
  if ( bytes_received > 0 ) {
#if DEBUG
    Serial.println(in_msg[0]);
#endif
    switch ( in_msg[0] ) {
      case COMM_RGB:
        led_state = LED_RGB;
        color_r = in_msg[1];
        color_g = in_msg[2];
        color_b = in_msg[3];
        rgb(color_r, color_g, color_b);
        break;
      case COMM_DISCO:
        led_state = LED_DISCO;
        break;
      case COMM_BLUE_SKY:
        led_state = LED_BLUE_SKY;
        break;
      case COMM_NIGHT:
        night_state = NIGHT_0;
        led_state = LED_NIGHT;
        break;
      case COMM_OVERCAST:
        led_state = LED_OVERCAST;
        break;
      case COMM_GOLDEN:
        led_state = LED_GOLDEN;
        break;
      case COMM_SNOW:
        led_state = LED_SNOW;
        break;
      case COMM_LIGHTNING:
        clearCloud();
        led_state = LED_LIGHTNING;
        break;
      case COMM_RAIN:
        clearCloud();
        led_state = LED_RAIN;
        break; 
       case COMM_MIST:
        clearCloud();
        led_state = LED_MIST;
        break;
      case COMM_RAINBOW:
        clearCloud();
        led_state = LED_RAINBOW;
        break;
      case COMM_TWILIGHT:
        clearCloud();
        led_state = LED_TWILIGHT;
        break;  
       case COMM_DARK:
        clearCloud();
        led_state = LED_DARK;
        break;         
      default:
        break;
    }
  }
}

/****************************************************************
 * Receiver
 ***************************************************************/
uint8_t receiveMessage(byte msg[], unsigned long timeout) {
  
  boolean valid;
  uint8_t buf_idx;
  unsigned long start_time;
  byte in_buf[IN_BUF_MAX];
  byte r;
  
  // Wait until we get a valid message or time out
  start_time = millis();
  valid = false;
  while ( !valid ) {
    
    // Wait until we see a Start of Frame (SOF) or time out
    memset(in_buf, 0, IN_BUF_MAX);
    while ( !softy.available() ) {
      if ( (millis() - start_time > timeout) && (timeout > 0) ) {
        return 0;
      }
    }
    r = softy.read();
    if ( r != SERIAL_SOF ) {
      continue;
    }

    // Read buffer
    delay(2);  // Magical delay to let the buffer fill up
    buf_idx = 0;
    while ( softy.available() > 0 ) {
      if ( buf_idx >= IN_BUF_MAX ) {
        buf_idx = IN_BUF_MAX - 1;
      }
      in_buf[buf_idx] = softy.read();
#if DEBUG
      Serial.print("0x");
      Serial.print(in_buf[buf_idx], HEX);
      Serial.print(" ");
#endif
      buf_idx++;
      delay(2);  // Magical delay to let the buffer fill up
    }
#if DEBUG
    Serial.println();
#endif

    if ( in_buf[buf_idx - 1] == SERIAL_EOF ) {
      valid = true;
    }
  }
  
  // Copy our message (don't copy checksum or EOF bytes)
  memcpy(msg, in_buf, buf_idx - 1);
  
  return buf_idx - 1;
}

/****************************************************************
 * LED Functions
 ***************************************************************/

// Update LED strips
void doLED() {
  
  // Display LEDs based on the current weather
  switch ( led_state ) {

    // RGB
    case LED_RGB:
      break;

    // Disco
    case LED_DISCO:
      disco();
      break;
    
    // Blue sky
    case LED_BLUE_SKY:
      blueSky();
      break;
      
    // Night
    case LED_NIGHT:
      nighttime();
      break;
      
    // Overcast
    case LED_OVERCAST:
      overcast();
      break;
      
    // Golden
    case LED_GOLDEN:
      golden();
      break;
      
    // Snow
    case LED_SNOW:
      //clearCloud();
      snow();
      break;
      
    // Lightning
    case LED_LIGHTNING:
      lightningStorm();
      break;
      
   // Rain
    case LED_RAIN:
      rain();
      break;
      
   // Mist
    case LED_MIST:
      // whiteClouds();
      fog(20);
      break;    
      
   // Rainbow
    case LED_RAINBOW:
      rainbow(20);
      break; 
      
    // Twilight
    case LED_TWILIGHT:
      twilight();
      break;  
    // Dark
    case LED_DARK:
     // Do nothing, leave LED's off
     break;
    // Do nothing as default
    default:
      break;
  }
}

// Display static RGB value
void rgb(uint8_t r, uint8_t g, uint8_t b) {
#if DEBUG
  Serial.print("RGB: ");
  Serial.print(r);
  Serial.print(",");
  Serial.print(g);
  Serial.print(",");
  Serial.print(b);
#endif
  for ( int i = 0; i < PIXELS; i++ ) {
    strip_a.setPixelColor(i, r, g, b);
  }
  strip_a.show();
}

/****************************************************************
 * LED Strip - Disco Functions
 ***************************************************************/
void disco() {

  int i;
  int red;
  int green;
  int blue;

#if DEBUG
  Serial.print("Disco: ");
  Serial.println(disco_state);
#endif
  
  if ( (millis() - led_time) >= disco_delay ) {
    switch ( disco_state ) {

      case 0:
        red = random(150);
        green = random(250);
        blue = random(200);
        for (i = 0; i < 50; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 1:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;

      case 2:
        red = random(250);
        green = random(100);
        blue = random(200);
        for (i = 40; i < PIXELS; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;
  
      case 3:
        red = random(250);
        green = random(250);
        blue = random(250);
        for (i = 40; i < PIXELS; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 4:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;

      case 5:
        red = random(250);
        green = random(250);
        blue = random(250);
        for (i = 40; i < PIXELS; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 6:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;
  
      case 7:
        red = random(250);
        green = random(250);
        blue = random(250);
        for (i = 12; i < 22; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 8:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;
  
      case 9:
        red = random(250);
        green = random(250);
        blue = random(250);
        for (i = 0; i < 22; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 10:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;
  
      case 11:
        red = random(200);
        green = random(200);
        blue = random(200);
        for (i = 25; i < 35; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 12:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;
  
      case 13:
        red = random(200);
        green = random(200);
        blue = random(200);
        for (i = 125; i < 175; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 14:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;
  
      case 15:
        red = random(200);
        green = random(200);
        blue = random(200);
        for (i = 200; i < PIXELS; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 16:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;
  
      case 17:
        red = random(200);
        green = random(200);
        blue = random(200);
        for (i = 0; i < 22; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 18:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;

      case 19:
        red = random(200);
        green = random(200);
        blue = random(200);
        for (i = 40; i < PIXELS; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 20:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;

      case 21:
        red = random(200);
        green = random(200);
        blue = random(200);
        for (i = 12; i < 22; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 22:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;

      case 23:
        red = random(200);
        green = random(200);
        blue = random(200);
        for (i = 0; i < 10; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 24:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;

      case 25:
        red = random(200);
        green = random(200);
        blue = random(200);
        for (i = 12; i < 22; i++){
          strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 26:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;

      case 27:
        red = random(200);
        green = random(200);
        blue = random(200);
        for (i = 25; i < 35; i++){
        strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 28:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;
  
      case 29:
        red = random(200);
        green = random(200);
        blue = random(200);
        for (i = 25; i < 35; i++){
        strip_a.setPixelColor(i, red, green, blue);
        }
        strip_a.show();
        break;

      case 30:
        for (i = 0; i < PIXELS; i++){
          strip_a.setPixelColor(i, 0, 0, 0);
        }
        strip_a.show();
        break;

      default:
        disco_state = 0;
        break;
    }
  }

  disco_state = (disco_state + 1) % 31;
  disco_delay = DISCO_DELAY;
  led_time = millis();
}

/****************************************************************
 * LED Strip - Weather Functions
 ***************************************************************/

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip_a.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip_a.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip_a.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

// Clear cloud function:
void clearCloud(){
  for (int i = 0; i < PIXELS; i++){ //for all the LEDs
    strip_a.setPixelColor(i, 0, 0, 0); //turn off in cloud one
  }
  #if DEBUG
    Serial.println("clear cloud");
  #endif
  strip_a.show(); //show what was set in cloud one
}

// Bluesky function
void blueSky() {
  for(int i=0; i<PIXELS; i++) {    //for all of the LEDs 
    strip_a.setPixelColor(i, 0, 170, 175);   //set LEDs a sky blue color in cloud one
  }
  #if DEBUG
    Serial.println("blue sky");
  #endif
  strip_a.show();
//  strip_b.show();
}

// White clouds function
void whiteClouds() {
  for(int i=0; i<PIXELS; i++) {   //for all of the LEDs 
    strip_a.setPixelColor(i, 100, 100, 100);  //set LEDs white-ish in cloud one
  }
  #if DEBUG
    Serial.println("white cloud");
  #endif
  strip_a.show();
}


// Overcast funtion
void overcast() {
  for(int i=0; i<PIXELS; i++) {   //for all of the LEDs 
    strip_a.setPixelColor(i, 70, 64, 75);  //set LEDs grey-ish in cloud one
  }
  #if DEBUG
    Serial.println("overcast");
  #endif
   strip_a.show();
}


//Theater-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
          for (int x=0; x < PIXELS; x++) {
                int q = x % 4;
                for (int i=0; i < PIXELS; i=i+4) {
                  strip_a.setPixelColor(i+q, c);    //turn every tenth pixel on
                }
                strip_a.show();
               
                delay(wait);
                    for (int i=0; i < PIXELS; i=i+4) {
                      strip_a.setPixelColor(i+q, 0);        //turn every tenth pixel off
                    } 
               strip_a.show();     
          }
}

//Rainbow function:
void rainbow(uint8_t wait) {
      uint16_t i, j;
      for(j=0; j<256; j++) {                       //for 256 colors
              for(i=0; i<PIXELS; i++) {                     //for all the LEDs
                strip_a.setPixelColor(i, wheel((i+j) & 255));  //set each LED a different color in cloud one
              }
              strip_a.show();  //show the colors that were set in cloud one
              delay(wait);     //pause for the amount of time put into the function
          }
}

// Mist function to emulate fog
void fog(uint8_t wait) {
  for (int j =0; j < FOG_MAX; j++) {
     for(int i = 0; i < PIXELS; i++) {  
        strip_a.setPixelColor(i, j, j, j);  //set LEDs with mist color
     } // for PIXELS
     strip_a.show();  //show the colors that were set in cloud one
     delay(wait);     //pause for the amount of time put into the function
  }  //for mist 0 to 128 - dark white to bright white
  delay(FOG_DELAY);  // hold for a little bit
  for (int j = 0; j < FOG_MAX; j++) {

    for (int i=0; i<PIXELS; i++) { 
        int mist =  FOG_MAX - j;
        strip_a.setPixelColor(i,  mist, mist, mist);  //set LEDs with mist color
     } // for PIXELS
     strip_a.show();  //show the colors that were set in cloud one
     delay(wait);     //pause for the amount of time put into the function
  }  //for mist 160 to 32 - bright white to dark white
  delay(FOG_DELAY);  // hold for a little bit  
}

//Rain funtion:
void rain(){
  theaterChase(strip_a.Color(  0,   0, 127), 50); // Blue
  
  #if DEBUG
    Serial.println("rain");
  #endif
}

//Snow funtion:
void snow(){
  theaterChase(strip_a.Color(127, 127, 127), 100); 
  
  #if DEBUG
    Serial.println("snow");
  #endif
}


// Night function
void nighttime() {
  #if DEBUG
    Serial.print("Night: ");
    Serial.println(night_state);
  #endif
  switch ( night_state ) {
    
    case NIGHT_0:
      for(int i=0; i<PIXELS; i++) {    //for all of the LEDs 
         strip_a.setPixelColor(i, 0, 0, 64);  //set LEDs dark blue in cloud one
      }
      strip_a.show();
      night_i = 1;
      night_state = NIGHT_1;
      led_time = millis();
      break;
       
    case NIGHT_1:
      if ( (millis() - led_time) >= 50 ) {
        strip_a.setPixelColor((night_i-1), 96, 96, 96);  //set LEDs white in cloud one
        strip_a.show();
        night_state = NIGHT_2;
        led_time = millis();
      }
      break;
     
    case NIGHT_2:
      if ( (millis() - led_time) >= 50 ) {
        strip_a.setPixelColor(night_i, 96, 96, 96);  //set LEDs white in cloud one
        strip_a.show();
        night_state = NIGHT_3;
        led_time = millis();
      }
      break;
       
    case NIGHT_3:
      if ( (millis() - led_time) >= 50 ) {
        strip_a.setPixelColor((night_i+1), 96, 96, 96);  //set LEDs white in cloud one
        strip_a.show();
        night_state = NIGHT_4;
        led_time = millis();
      }
      break;
      
    case NIGHT_4:
      if ( (millis() - led_time) >= TWINKLE_DELAY ) {
        strip_a.setPixelColor((night_i-1), 0, 0, 64);  //turn off
        strip_a.show();
        night_state = NIGHT_5;
        led_time = millis();
      }
      break;
      
    case NIGHT_5:
      if ( (millis() - led_time) >= 50 ) {
        strip_a.setPixelColor(night_i, 0, 0, 64);  //turn off
        strip_a.show();
        night_state = NIGHT_6;
        led_time = millis();
      }
      break;
      
    case NIGHT_6:
      if ( (millis() - led_time) >= 50 ) {
        strip_a.setPixelColor((night_i+1), 0, 0, 64);  //turn off
        strip_a.show();
        night_state = NIGHT_1;
        led_time = millis();
        night_i = random(1, PIXELS);  // blink randomly
      }
      break;
      
      default:
        night_state = NIGHT_0;
        break;
  }
}



// Sun rise/ Sun set function: all golden
void golden(){
  for(int i=0; i<PIXELS; i++)  {
     strip_a.setPixelColor(i, 150, 90, 0);  //set LEDs orange-red in cloud 
  }
  strip_a.show(); //show all the colors that were set in cloud
}

// Sun rise/ Sun set function: reddish purple for twilight
void twilight(){
  for(int i=0; i<PIXELS; i++)  {
     strip_a.setPixelColor(i, 96, 0, 96);  //set LEDs purple-red in cloud 
  }
  strip_a.show(); //show all the colors that were set in cloud
}

// Lightning storm function
void lightningStorm() {

#if DEBUG
  Serial.print("Storm: ");
  Serial.println(storm_state);
#endif
  
  //Various types of lightning functions, where the first two 
  //numbers represent the area that the flash could randomly show 
  //up in, the next number is usually how long the flash is (in milisec)
  //the forth number is sometimes the size of the flash, and the
  //last is the color setting for the flash
  if ( (millis() - led_time) >= storm_delay ) {
    switch ( storm_state ) {
      case 0:
        jumpingFlash_a(10, 16, 50, strip_a.Color(255,255,255));
        break;
      case 1:
        jumpingFlash_a(20, 36, 50, strip_a.Color(255,255,255));
        break;
      case 2:
        scrollingFlash_a(5, 15, 50, 5, strip_a.Color(255,255,255));
        break;
      case 3:
        singleFlash_a(20, 40, 50, 15, strip_a.Color(255,255,255));
        break;
      case 4:
        singleFlash_a(30, 50, 50, 15, strip_a.Color(255,255,255));
        break;
      case 5:
        singleFlash_a(40, 60, 50, 15, strip_a.Color(255,255,255));
        break;
      case 6:
        scrollingFlash_a(40, 50, 50, 15, strip_a.Color(255,255,255));
        break;
      case 7:
        singleFlash_a(10, 20, 50, 5, strip_a.Color(200,200,255));
        break;
      case 8:
        scrollingFlash_a(40, 50, 50, 15, strip_a.Color(255,255,255));
        break;
      case 9:
        jumpingFlash_a(20, 26, 50, strip_a.Color(255,255,255));
        break;
      case 10:
        jumpingFlash_a(30, 36, 50, strip_a.Color(255,255,255));
        break;
      case 11:
        multipleFlashs_a(5, 25, 30, 60, 50, 5, strip_a.Color(255,255,255));
        break;
      case 12:
        flickerFlash_a(30, 60, 10, 25, strip_a.Color(200,200,255));
        break;
      case 13:
        multipleFlashs_a(5, 25, 30, 60, 50, 5, strip_a.Color(255,255,255));
        break;
      case 14:
        scrollingFlash_a(15, 60, 100, 15, strip_a.Color(225,200,255));
        break;
      case 15:
        flickerFlash_a(25, 60, 40, 25, strip_a.Color(255,255,255));
        break;
      case 16:
        flickerFlash_a(35, 55, 40, 10, strip_a.Color(255,255,255));
        break;
      case 17:
        flickerFlash_a(15, 25, 40, 10, strip_a.Color(255,255,255));
        break;
      case 18:
        jumpingFlash_a(40, 26, 50, strip_a.Color(255,255,255));
        break;
      case 19:
        flickerFlash_a(15, 35, 40, 5, strip_a.Color(255,255,255));
        break;
      case 20:
        flickerFlash_a(10, 60, 50, 10, strip_a.Color(200,200,255));
        break;
      case 21:
        scrollingFlash_a(40, 50, 100, 10, strip_a.Color(255,255,255));
        break;
      case 22:
        wholeCloudFlash_a(8, 20, strip_a.Color(255,255,255));
        break;
      case 23:
        multipleFlashs_a(5, 25, 35, 60, 50, 5, strip_a.Color(255,255,255));
        break;
      case 24:
        scrollingFlash_a(4, 13, 50, 3, strip_a.Color(255,255,255));
        break;
      case 25:
        flickerFlash_a(10, 60, 50, 10, strip_a.Color(200,200,255));
        break;
      case 26:
        flickerFlash_a(5, 20, 50, 5, strip_a.Color(200,200,255));
        break;
      case 27:
        singleFlash_a(15, 35, 40, 5, strip_a.Color(255,255,255));
        break;
      case 28:
        singleFlash_a(20, 40, 50, 10, strip_a.Color(255,255,255));
        break;
      case 29:
        scrollingFlash_a(20, 65, 50, 5, strip_a.Color(255,255,255));
        break;
      case 30:
        multipleFlashs_a(20, 25, 40, 60, 50, 5, strip_a.Color(255,255,255));
        break;
      case 31:
        singleFlash_a(20, 40, 50, 5, strip_a.Color(255,255,255));
        break;
      case 32:
        flickerFlash_a(10, 20, 50, 10, strip_a.Color(200,200,255));
        break;
      case 33:
        scrollingFlash_a(40, 60, 50, 15, strip_a.Color(255,255,255));
        break;
      case 34:
        jumpingFlash_a(45, 60, 50, strip_a.Color(255,255,255));
        break;
      case 35:
        jumpingFlash_a(60, 45, 50, strip_a.Color(255,255,255));
        break;
      case 36:
        multipleFlashs_a(20, 25, 40, 60, 50, 5, strip_a.Color(255,255,255));
        break;
      case 37:
        scrollingFlash_a(10, 60, 20, 7, strip_a.Color(225,200,255));
        break;
      case 38:
        singleFlash_a(15, 35, 40, 5, strip_a.Color(255,255,255));
        break;
      case 39:
        singleFlash_a(30, 60, 40, 5, strip_a.Color(255,255,255));
        break;
      case 40:
        jumpingFlash_a(30, 26, 50, strip_a.Color(255,255,255));
        break;
      case 41:
        jumpingFlash_a(10, 16, 50, strip_a.Color(255,255,255));
        break;
      case 42:
        scrollingFlash_a(50, 60, 50, 5, strip_a.Color(255,255,255));
        break;
      case 43:
        flickerFlash_a(50, 60, 50, 10, strip_a.Color(200,200,255));
        break;
      case 44:
        scrollingFlash_a(50, 60, 50, 5, strip_a.Color(255,255,255));
        break;
      case 45:
        multipleFlashs_a(5, 25, 30, 60, 50, 5, strip_a.Color(200,200,255));
        break;
      case 46:
        wholeCloudFlash_a(8, 20, strip_a.Color(255,255,255));
        break;
      default:
        break;
    }
    storm_state = (storm_state + 1) % 47;
    storm_delay = random(900, 1500);
    led_time = millis();
  }
}

/****************************************************************
 * LED Strip - Lightning Functions
 ***************************************************************/

void singleFlash_a(int areaMin, int areaMax, int duration, int Size, uint32_t color) {
  int i;
  int flashStrike = random(areaMin, areaMax);
  for ((i = flashStrike - Size); (i < flashStrike); i++) {
   strip_a.setPixelColor(i, 255,255,255);//color);
 }
 strip_a.show();
 delay(duration);
 for ((i = flashStrike - Size); (i < flashStrike); i++) {
   strip_a.setPixelColor(i, 0, 0, 0);
 }
 strip_a.show();
}

void flickerFlash_a(int areaMin, int areaMax, int duration, int Size, uint32_t color) {
  int i;
  int flashStrike = random(areaMin, areaMax);
  for ((i = flashStrike - Size); (i < flashStrike); i++) {
   strip_a.setPixelColor(i, color);
 }
 strip_a.show();
 delay(50);
 for ((i = flashStrike - Size); (i < flashStrike); i++) {
   strip_a.setPixelColor(i, 0, 0, 0);
 }
 strip_a.show();
 delay(50);
 flashStrike = random(areaMin, areaMax);
  for ((i = flashStrike - Size); (i < flashStrike); i++) {
   strip_a.setPixelColor(i, color);
 }
 strip_a.show();
 delay(duration);
 for ((i = flashStrike - Size); (i < flashStrike); i++) {
   strip_a.setPixelColor(i, 0, 0, 0);
 }
 strip_a.show();
}


void scrollingFlash_a(int areaMin, int areaMax, int duration, int Size, uint32_t color) {
  int i;
  int flashStrike = random(areaMin, areaMax);
  for ((i = flashStrike - Size); (i < flashStrike); i++) {
    strip_a.setPixelColor(i, color);
    strip_a.show();
  }
  for ((i = flashStrike - Size); (i < flashStrike); i++) {
    strip_a.setPixelColor(i, 0, 0, 0);
    strip_a.show();
  }
  for ((i = flashStrike - Size); (i < flashStrike); i++) {
    strip_a.setPixelColor(i, color);
    strip_a.show();
  }
  delay(duration);
  for ((i = flashStrike - Size); (i < flashStrike); i++) {
    strip_a.setPixelColor(i, 0, 0, 0);
    strip_a.show();
  }
} 


void multipleFlashs_a(int areaOneMin, int areaOneMax, int areaTwoMin, int areaTwoMax, int duration, int Size, uint32_t color){
 int i;
 int oneStrike = random(areaOneMin, areaOneMax);
 int twoStrike = random(areaTwoMin, areaTwoMax);
 for ((i = oneStrike - Size); (i < oneStrike); i++) {
   strip_a.setPixelColor(i, color);
   strip_a.show();
 }
 for ((i = twoStrike - Size); (i < twoStrike); i++) {
   strip_a.setPixelColor(i, color);
   strip_a.show();
 }
 delay(duration);
 for ((i = oneStrike - Size); (i < oneStrike); i++) {
   strip_a.setPixelColor(i, 0, 0, 0);
   strip_a.show();
 }
 for ((i = twoStrike - Size); (i < twoStrike); i++) {
   strip_a.setPixelColor(i, 0, 0, 0);
   strip_a.show();
 }
}


void jumpingFlash_a(int areaStart, int areaEnd, int duration, uint32_t color) {
 int i;
 for (i = areaStart; (i < areaStart + 15); i++){
   strip_a.setPixelColor(i, color);
 }
 strip_a.show();
 delay(duration);
 for (i = areaStart; (i < areaStart + 15); i++){
   strip_a.setPixelColor(i, 0, 0, 0);
   strip_a.show();
 }
 for ((i = areaEnd - 15); i < areaEnd; i++){
   strip_a.setPixelColor(i, color);
 }
 strip_a.show();
 delay(duration);
 for ((i = areaEnd - 15); i < areaEnd; i++){
   strip_a.setPixelColor(i, 0, 0, 0);
 }
 strip_a.show();
 delay(duration);
 for ((i = areaEnd - 15); i < areaEnd; i++){
   strip_a.setPixelColor(i, color);
 }
 strip_a.show();
 delay(duration + 50);
 for ((i = areaEnd - 15); i < areaEnd; i++){
   strip_a.setPixelColor(i, 0, 0, 0);
   strip_a.show();
 }
}


void wholeCloudFlash_a(int durationShort, int durationLong, uint32_t color) {
  int i;
  for (i = 12; i < 15; i++){
   strip_a.setPixelColor(i, color);
 }
 strip_a.show();
 delay(durationShort);
 for (i = 12; i < 15; i++){
   strip_a.setPixelColor(i, 0, 0, 0);
 }
    strip_a.show();
    delay(durationLong);
 for (i = 12; i < 15; i++){
   strip_a.setPixelColor(i, color);
   strip_a.show();
 }
 delay(durationShort);
 for (i = 12; i < 15; i++){
   strip_a.setPixelColor(i, 0, 0, 0);
   strip_a.show();
 }
  for (i = 20; i < 23; i++){
   strip_a.setPixelColor(i, color);
   strip_a.show();
 }
 for (i = 20; i < 23; i++){
   strip_a.setPixelColor(i, 0, 0, 0);
   strip_a.show();
 }
 for (i = 20; i < 23; i++){
   strip_a.setPixelColor(i, color);
   strip_a.show();
 }
 delay(durationShort);
 for (i = 20; i < 23; i++){
   strip_a.setPixelColor(i, 0, 0, 0);
   strip_a.show();
 }
 for (i = 23; i < 26; i++){
   strip_a.setPixelColor(i, color);
 }
 strip_a.show();
 delay(durationShort);
 for (i = 23; i < 26; i++){
   strip_a.setPixelColor(i, 0, 0, 0);
 }
 strip_a.show();
 delay(durationShort);
 for (i = 23; i < 26; i++){
   strip_a.setPixelColor(i, color);
 }
 strip_a.show();
 delay(durationShort);
 for (i = 23; i < 26; i++){
   strip_a.setPixelColor(i, 0, 0, 0);
   strip_a.show();
 }
 for (i = 26; i < 30; i++){
   strip_a.setPixelColor(i, color);
 }
 strip_a.show();
 delay(durationShort);
 for (i = 26; i < 30; i++){
   strip_a.setPixelColor(i, 0, 0, 0);
 }
 strip_a.show();
 delay(durationShort);
 for (i = 26; i < 30; i++){
   strip_a.setPixelColor(i, color);
 }
 strip_a.show();
 delay(durationLong);
 for (i = 26; i < 30; i++){
   strip_a.setPixelColor(i, 0, 0, 0);
   strip_a.show();
 }
}



