/**
 * Based on Sparkfun CloudCloud
 * by Shawn Hymel @ SparkFun Electronics
 * 
 * Copyright (c) 2015-2022 by Gaston Willams
 *
 * September 17, 2015
 * Initial code adapted from Sparkfun
 * 
 * July 2, 2016
 * Updated to handle multiple condition codes from OpenWeather API.
 * 
 * June 23, 2022
 * Removed Blynk and blynk demo modes
 * 
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#include "States.h"
#include "Secrets.h"

// Constant for conditions
#define MAX_CONDITIONS    3

// Pin definitions
#define LED_PIN           5

// NTP Constants
#define NTP_PACKET_SIZE 48

//Sleep times - 10:00 pm to 5:00 am
#define BEDTIME  79200
#define WAKETIME 18000



// Other constants
const byte SERIAL_SOF = 0xA5;
const byte SERIAL_EOF = 0x5A;
const int OUT_BUF_MAX = 7;         // bytes


// Location data (Durham, NC)
// const float LATITUDE = 35.926792;
// const float LONGITUDE = -78.824102;
// Remote site information
const char http_site[] = "api.openweathermap.org";
const int http_port = 80;

// Other constants
const int TIMEOUT = 300;    // ms
const int BUFFER_SIZE = 22; // bytes
const int ICON_SIZE = 4;    // bytes
const int DT_SIZE = 12;     // bytes

// Global variables
WiFiClient client; // http client for API requests
WiFiUDP Udp;       // udp client for NTP requests
WeatherType current_weather[MAX_CONDITIONS];
unsigned long time_update = 0;  // minute counter  
unsigned long time_interval = 0;  // 10 second interval counter
bool force_update;
byte out_msg[OUT_BUF_MAX];
int condition_count = 0;
int condition_index = 0;
boolean daybreak   = false;

/*
 * Sleep mode indicates that the local time is between bedtime and wake time.  
 * We don't need to query the weather during this time since the cloud is off.
 */
boolean sleep_mode = false;

/****************************************************************
 * Setup
 ***************************************************************/
void setup() {
  // Enable status LED
  pinMode(LED_PIN, OUTPUT);
  //Turn off, wifi turns on when it connects
  digitalWrite(LED_PIN, LOW);
    
  // Set up serial communication
  Serial.begin(9600);

  //Wifi ssid and password are defined in Secrets.h file
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  delay(500); //wait a bit to connect
  
  while (WiFi.status() != WL_CONNECTED) {
    //show disco mode (error) until wifi connected
    sendDisco();
    delay(500); //wait a bit and try again
  }
  
  // Set up time
  setSyncProvider(getNtpTime);
  setSyncInterval(1800);
  
  // Define initial state
  time_update = millis();
  time_interval = time_update;
  force_update == true;
 
  //set weather dark in case we are asleep
  condition_count = 1;
  current_weather[0] = WEATHER_DARK;
  
  //Turn cloud orange for 3 seconds
  sendRGB(133, 58, 0);
  delay(3000);

  //Show each possible condition on the cloud
  testWeather();

  //then go dark until API update
  sendRGB(0, 0, 0);
} // setup

/****************************************************************
 * Loop
 ***************************************************************/
void loop() {
  //After removing Blynk, just do weather all the time
  doWeather();
  delay(1000);
} // loop

/****************************************************************
 * Send Modes to Pro Mini
 ***************************************************************/

// Send RGB code with red, green, blue values
void sendRGB(uint8_t color_r, uint8_t color_g, uint8_t color_b) {
  out_msg[0] = COMM_RGB;
  out_msg[1] = color_r;
  out_msg[2] = color_g;
  out_msg[3] = color_b;
  transmitMessage(out_msg, 4);
} // sendRGB

// Send Disco code
void sendDisco() {
  out_msg[0] = COMM_DISCO;
  transmitMessage(out_msg, 4);
} // sendDisco

/****************************************************************
 * Weather Functions
 ***************************************************************/
 
// for multiple conditions, send the next condition
void sendNextWeather() {      
    if (condition_count > 1) {
        condition_index++;
        condition_index %= condition_count;
        sendWeather(current_weather[condition_index]);
    } // if
} // sendNextWeather

// send the weather condition to the cloud
void sendWeather(int condition) {
      switch ( condition ) {      
        // Shouldn't happen, but just in case
        case WEATHER_ERROR:
          out_msg[0] = COMM_DISCO;
          return;
          break;
        
        // Clear skies (day)
        case WEATHER_BLUE_SKY:
          out_msg[0] = COMM_BLUE_SKY;
          break;
        
        // Clear skies (night)
        case WEATHER_NIGHT:
          out_msg[0] = COMM_NIGHT;
          break;
        
        // Lots of clouds
        case WEATHER_OVERCAST:
          out_msg[0] = COMM_OVERCAST;
          break;
        
        // Dawn or dusk
        case WEATHER_GOLDEN:
          out_msg[0] = COMM_GOLDEN;
          break;
        
        // It's snowing!
        case WEATHER_SNOW:
          out_msg[0] = COMM_SNOW;
          break;
        
        // Storm
        case WEATHER_LIGHTNING:
          out_msg[0] = COMM_LIGHTNING;
          break;
          
        // Rain
        case WEATHER_RAIN:
          out_msg[0] = COMM_RAIN;
          break;  
          
        //Fog, Mist or Haze
        case WEATHER_MIST:
          out_msg[0] = COMM_MIST;
          break;    
          
        // Twilight
        case WEATHER_TWILIGHT:
          out_msg[0] = COMM_TWILIGHT;
          break;  
          
        // Dark (Sleep time)
        case WEATHER_DARK:
          out_msg[0] = COMM_DARK;
          break;      
              
        // Unknown weather type        
        case WEATHER_UNKNOWN:
        default:
          out_msg[0] = COMM_RAINBOW;
          break;
      } // switch
      transmitMessage(out_msg, 4);
} // sendWeather

// send all weather conditions to the cloud one by one
void testWeather() {
  //step through each weather condition in enumeration
  for (int condition = WEATHER_ERROR; condition <= WEATHER_UNKNOWN; condition++) {
     out_msg[0] = condition;
     transmitMessage(out_msg, 4);
     //show the state long enough to view it
     delay(5000);
  } //for condition
}// testWeather

// Every 60 seconds, pull weather data and update LEDs
// Updated logic to show error on lcd rather than cloud
void doWeather() {  
  boolean result = false;
  // Only update every 60 seconds
  if ( force_update || ((millis() - time_update) >= 60000)) {
    force_update = false;
    time_update = millis();
    time_interval = time_update;

    //if we are asleep, check the time and update the sleep_mode
    if (sleep_mode) {
      // get the current time via NTP
      unsigned long currentTime = now();
      int tz_offset = isDST() ? 4 : 5;      
      long local_time = getLocalTime(currentTime, tz_offset);
      
      //check the time to see if we should wake up next time
      sleep_mode = isSleepTime(local_time);
    } // if sleep_mode
    
    // Get weather data, when not asleep
    if (!sleep_mode) {
     result = getWeather();
    } // if !sleep_mode
    
    // if we get new weather codes show them, 
    // otherwise continue scrolling old codes
    if (result) {         
      //send the first weather condition
      condition_index = 0;
      sendWeather(current_weather[condition_index]);     
    } else {
      // show next condition from previous request
      sendNextWeather();
    }  // if-else (result)
    // Update the cloud
  } else if ( ((millis() - time_interval) >= 10000) ) {
    time_interval = millis(); // get new 10 second interval
    // if we have more than one condition change every 10 seconds
    sendNextWeather();
  } // end if - else if(millis)
} // doWeather

// Time functions
/*
 * Get the local time value after midnight from epoc value
 * from OpenWeather API.
 *
 * epoc - UTC time in seconds since Jan 1, 1970
 * tz_offset - timezone offset, in hours behind UTC
 *            + west of UTC / - east of UTC
 *
 *        5 for US Eastern Standard Time 
 *        4 for US Eastern Daylight Time
 *
 * returns long - seconds after midnight
 */
long getLocalTime(unsigned long epoc, int tz_offset) {
  long lt = (epoc - tz_offset *3600) % 86400;
  return lt;
} // getLocalTime

/*
 * Determine if we are in US Daylight Savings time.
 *
 * There is a TimeZone library function for this and more, but
 * it seemed like over-kill.  
 */
boolean isDST() {
  boolean dst = false;
  int m = month();
  int previousSunday = day() - weekday() + 1;
  
  if (m < 3 || m > 11) {
    //January, february, and december are standard time.
    dst = false;
  } else if (m > 3 && m < 11) {
    //April through October are in daylight savings time.
    dst = true;
  } else if (m == 3) {
    // In March, DST begins after the second sunday, so in DST
    // if our previous sunday was on or after the 8th.
    dst = previousSunday >= 8;
  } else if (m == 11) {
    // In November we must be before the first sunday to be dst.
    // That means the previous sunday must be before the 1st.
    dst = previousSunday <= 0;
  }
  return dst;   
}

// Perform an HTTP GET request to openweathermap.org
boolean getWeather() {
  unsigned long currentTime = 0;
  unsigned long time_read = millis();
  unsigned long dt = 0;
  unsigned long sunrise = 0;
  unsigned long sunset = 0;
  char buf[BUFFER_SIZE];
  char icon[MAX_CONDITIONS][ICON_SIZE];
  char dt_str[DT_SIZE];
  char c;
  char* dummy;  // used in strtol function
  boolean golden   = false;
  boolean twilight = false;
  boolean darkness = false;
  int tz_offset = 5;  // default for EST

  // save old count in case of error
  int old_count = condition_count;  
  
  ParserState parser_state = PARSER_START;
  WeatherType condition = WEATHER_UNKNOWN;

    
  // Flush read buffer
  if ( client.available() ) {
    client.read();
  }

  // Set string buffers to null
  for (int i = 0; i < MAX_CONDITIONS; i++) {
    memset(icon[i], 0, ICON_SIZE);
  } // for
  
  //clear variables
  memset(dt_str, 0, DT_SIZE);
  condition_count = 0;
  
  // Attempt to make a connection to the remote server
  if ( !client.connect(http_site, http_port) ) {
    condition_count = old_count;
    return false;
  }
  
  // Construct HTTP GET request
  //String cmd = "GET /data/2.5/weather?lat=";
  String cmd = "GET /data/2.5/weather?id=";
  // use city id for durham
  cmd += "4464368";

  //API ID string defined in Secrets.h file
  //You can get your own API id at https://home.openweathermap.org/users/sign_up 
  cmd += API_ID_STR;  
  
  cmd += " HTTP/1.1\nHost: ";
  cmd += http_site;
  cmd += "\nConnection: close\n\n";
  
  // Send out GET request to site
  client.print(cmd);
  
  // Parse data
  while ( (millis() - time_read) < TIMEOUT ) {
    
    // Get web page data and push to window buffer
    if ( client.available() ) {
      c = client.read();
      pushOnBuffer(c, buf, BUFFER_SIZE);
      time_read = millis();
    }
    
    // Depending on the state of the parser, look for string
    switch ( parser_state ) {
      case PARSER_START:
        if ( memcmp(buf, "\"weather\":", 10) == 0 ) {
          parser_state = PARSER_WEATHER;
        }
        break;
        
      case PARSER_WEATHER:
        // read up to 3 icon conditions
        if ( memcmp(buf, "\"icon\":\"", 8) == 0 ) {
          memcpy(icon[condition_count], buf + 8, 3);
          condition_count++;
        } else if ( memcmp(buf, "}]", 2) == 0 ) {
          parser_state = PARSER_DT;
        } else if (condition_count >= MAX_CONDITIONS) {
          parser_state = PARSER_DT;
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
    } // switch (parser_state)
  } // while (time_read)
  
  // Make sure we have a legitimate icon and times
  if ( (condition_count == 0) || (strcmp(icon[0], "") == 0) ||
        (dt == 0) || (sunrise == 0) || (sunset == 0) ) {
    condition_count = old_count;
    return false;
  }
  
  //grw - set up time values
  currentTime = now();
  tz_offset = isDST() ? 4 : 5; 
  daybreak = false; 
  
  //grw - correct api values for local time in seconds after midnight
  sunrise = getLocalTime(sunrise, tz_offset);
  sunset  = getLocalTime(sunset, tz_offset);

  
  //grw - Since dt can be unrealiable, get the clock time via NTP.
  //      Use it when clock time is more recent (almost always true)
  if (currentTime > dt) {
    dt = getLocalTime(currentTime, tz_offset);
  } else {
    // if currentTime is smaller, that means we didn't sync with NTP
    // so use the API observation time
    dt = getLocalTime(dt, tz_offset);
  } // if current time
  
  // See if we are in the golden time
  // 30 min before sunset or 30 after sunrise
  if ( ((dt - sunrise) >= 0) && ((dt - sunrise) <= 1800) ) {
    golden = true;
    daybreak = true;
  }
  if ( ((sunset - dt) >= 0) && ((sunset - dt) <= 1800) ) {
    golden = true;
    daybreak = false;
  }
  
  // See if we are in twilight
  // 30 min before sunrise or 30 after sunset
  if ( ((sunrise - dt) > 0) && ((sunrise - dt) <= 1800) ) {
    twilight = true;
    daybreak = true;
  }
  if ( ((dt - sunset) > 0) && ((dt - sunset) <= 1800) ) {
    twilight = true;
    daybreak = false;
  }
  
  // See if we should go dark because of night mode
  if (isSleepTime(dt)) {
    // Turn off lights with one single condition reported (Dark)
    condition_count = 1;
    current_weather[0] = WEATHER_DARK;
    // Set sleep mode flag and return
    sleep_mode = true;
    return true;
  }  // if isSleepTime
  
  // Lookup what to display based on icon
  // http://openweathermap.org/weather-conditions
  for (int i = 0; i < condition_count; i++) {
    if ( (strcmp(icon[i], "01d") == 0) || 
          (strcmp(icon[i], "02d") == 0) ) {
      // adjust for golden time or twilight      
      if ( golden ) {
        current_weather[i] = WEATHER_GOLDEN;
      } else if ( twilight ) {
        current_weather[i] = WEATHER_TWILIGHT;
      } else {
        current_weather[i] = WEATHER_BLUE_SKY;
      }
    } else if ( (strcmp(icon[i], "01n") == 0) || 
                (strcmp(icon[i], "02n") == 0) ) {
      // adjust for golden time or twilight             
      if ( golden ) {
        current_weather[i] = WEATHER_GOLDEN;
      } else if ( twilight ) {
        current_weather[i] = WEATHER_TWILIGHT;
      } else {
        current_weather[i] = WEATHER_NIGHT;
      } // if-else golden etc.
    } else if ( (strcmp(icon[i], "03d") == 0) || 
                (strcmp(icon[i], "03n") == 0) ||
                (strcmp(icon[i], "04d") == 0) ||
                (strcmp(icon[i], "04n") == 0) ) {
      // adjust for golden time or twilight if cloudy             
      if ( golden ) {
        current_weather[i] = WEATHER_GOLDEN;
      } else if ( twilight ) {
        current_weather[i] = WEATHER_TWILIGHT;
      } else {
        current_weather[i] = WEATHER_OVERCAST;
      } // if - else golden etc.     
    } else if ( (strcmp(icon[i], "09d") == 0) ||
                (strcmp(icon[i], "09n") == 0) ||
                (strcmp(icon[i], "10d") == 0) ||
                (strcmp(icon[i], "10n") == 0) ) {
      current_weather[i] = WEATHER_RAIN;
    } else if ( (strcmp(icon[i], "11d") == 0) ||
                (strcmp(icon[i], "11n") == 0) ) {
      current_weather[i] = WEATHER_LIGHTNING;
    } else if ( (strcmp(icon[i], "13d") == 0) ||
                (strcmp(icon[i], "13n") == 0) ) {
      current_weather[i] = WEATHER_SNOW;
    } else if ( (strcmp(icon[i], "50d") == 0) ||
                (strcmp(icon[i], "50n") == 0) ) {
      current_weather[i] = WEATHER_MIST;
    } else {
      current_weather[i] = WEATHER_UNKNOWN;
    } // if - else current_weather
  } // for
  return true;
} // getWeather

// See if we should go dark because of night time
boolean isSleepTime(unsigned long time_val) {  
//  return (night_mode && 
//          ((time_val > BEDTIME) || (time_val < WAKETIME)));
  return ((time_val > BEDTIME) || (time_val < WAKETIME));          
}

// Shift buffer to the left by 1 and append new char at end
void pushOnBuffer(char c, char *buf, uint8_t len) {
  for ( uint8_t i = 0; i < len - 1; i++ ) {
    buf[i] = buf[i + 1];
  }
  buf[len - 1] = c;
} // pushOnBuffer

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
} // transmitMessage


/****************************************************************
 * NTP code
 ***************************************************************/
/*
 * Get the UTC epoc time from an Ntp server
 */
time_t getNtpTime(){
  static const char ntpServerName[] = "us.pool.ntp.org";
  static const int  timeZone   = 0;    // UTC
  unsigned int      localPort = 8888;  // local port to listen for UDP packets
  
  IPAddress ntpServerIP; // NTP server's ip address
  byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
  time_t result = 0;
  
  // start UDP 
  Udp.begin(localPort);

  while (Udp.parsePacket() > 0) ; // discard any previously received packets

  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);

  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      unsigned long secsSince1900;
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer

      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];

      result = secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    } // if 
  }
  // signal we are done
  Udp.stop();

  return result; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
  
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
