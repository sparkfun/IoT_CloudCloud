#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define PINA 6 //Cloud one Data input pin hooked up to pin 6

#define BUTTON_PIN   3    // Digital IO pin connected to the button.  This will be
                          // driven with a pull-up resistor so the switch should
                          // pull the pin to ground momentarily.  On a high -> low
                          // transition the button press logic will execute.



// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip_a = Adafruit_NeoPixel(300, PINA, NEO_GRB + NEO_KHZ800);


bool oldState = HIGH;
int showType = 0;
int L = 0;
int reset = 0;

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.
int i = 0;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), change, RISING);
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code


  strip_a.begin(); 
  strip_a.show(); // Initialize all pixels in cloud to 'off'
}

void loop() {
  reset = 0;
  startShow(showType);
}


void change(){
  delayMicroseconds(1500000);
  showType = showType + 1;
      if (showType > 9){
        showType=0;
      }
  reset = 1;
}


void startShow(int L) {
  switch(L){
    case 0: clearCloud();  // Function turns all the LEDs in the clouds off
            break;
    case 1: blueSky();     // Function that sets the clouds blue
            break;
    case 2: nighttime(400); //Function that sets the clouds to a dark blue color
            break;
    case 3: clearCloud();
            theaterChase(strip_a.Color(127, 127, 127), 100); 
            // Function that makes a white theatre crawl type lighting simulation
            break;
    case 4: rainbow(20);  // Function that slowly scrolls rainbows across the clouds
            break;
    case 5: sunSet();    // Function that sets the clouds red/orange/yellow
            break;
    case 6: clearCloud();
            theaterChase(strip_a.Color(  0,   0, 127), 50); // Blue
            break;
    case 7: clearCloud();
            lightningStorm();  // Function that makes what looks like a lightning storm
            break;
    case 8: whiteClouds(); // Function that sets the clouds white
            break;
    case 9: clearCloud();
            disco(85, 100); // Function that makes the clouds look like a colorful disco
            break;
  }
}







void colorWheel(int Red, int Green, int Blue){
  if(reset != 1){
        for (i=0; i<300; i++){
          strip_a.setPixelColor(i, Red, Green, Blue);  ;
        }
        strip_a.show();  //show the colors that were set in cloud one
  }
}



//Rainbow function:
void rainbow(uint8_t wait) {
  while(reset != 1){  
      uint16_t i, j;
      for(j=0; j<256; j++) {                       //for 256 colors
          if(reset != 1){
              for(i=0; i<300; i++) {                     //for all the LEDs
                strip_a.setPixelColor(i, Wheel((i+j) & 255));  //set each LED a different color in cloud one
              }
              strip_a.show();  //show the colors that were set in cloud one
              delay(wait);     //pause for the amount of time put into the function
          }
      }
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  while(reset != 1){  
    for (int j=0; j<10; j++) {  //do 10 cycles of chasing
      if(reset != 1){
          for (int q=0; q < 20; q++) {
              if(reset != 1){
                for (int i=0; i < 300; i=i+20) {
                  strip_a.setPixelColor(i+q, c);    //turn every tenth pixel on
                }
                strip_a.show();
               
                delay(wait);
                  if(reset != 1){
                    for (int i=0; i < 300; i=i+20) {
                      strip_a.setPixelColor(i+q, 0);        //turn every tenth pixel off
                    } 
                  }
              }
          }
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
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

//Bluesky function:
void blueSky(){
    if(reset != 1){
      for(i=0; i<300; i++) {    //for all of the LEDs 
                strip_a.setPixelColor(i, 0, 170, 175);   //set LEDs a sky blue color in cloud one
      }
      strip_a.show();
    }
}

//White clouds function
void whiteClouds(){
    if(reset != 1){
      for(i=0; i<300; i++) {   //for all of the LEDs 
                  strip_a.setPixelColor(i, 100, 100, 100);  //set LEDs white-ish in cloud one
      }
      strip_a.show();
    }
}


//Overcast function:
void overcast(){
    if(reset != 1){
      for(i=0; i<300; i++) {   //for all of the LEDs 
                strip_a.setPixelColor(i, 70, 64, 75);  //set LEDs grey-ish in cloud one
       }
       strip_a.show();
    }
}


//Storm cloud function:
void nighttime(int twinkle){
    if(reset != 1){
      for(i=0; i<300; i++) {    //for all of the LEDs 
                  strip_a.setPixelColor(i, 0, 0, 102);  //set LEDs dark blue in cloud one
       }
       strip_a.show();
    }
    while(reset != 1){
      for(i=1; i<299; i=i+15){ //for every 15th LED
        if(reset != 1){
            strip_a.setPixelColor((i-1), 255, 255, 255);  //set LEDs white in cloud one
            strip_a.show();
            delay(50);
            strip_a.setPixelColor(i, 255, 255, 255);  //set LEDs white in cloud one
            strip_a.show();
            delay(50);
            strip_a.setPixelColor((i+1), 255, 255, 255);  //set LEDs white in cloud one
            strip_a.show();
            
            delay(twinkle);
            strip_a.setPixelColor((i-1), 0, 0, 102);  //set LEDs white in cloud one
            strip_a.show();
            delay(50);
            strip_a.setPixelColor(i, 0, 0, 102);  //set LEDs white in cloud one
            strip_a.show();
            delay(50);
            strip_a.setPixelColor((i+1), 0, 0, 102);  //set LEDs white in cloud one
            strip_a.show();
        }
      }
  }
}



//Sunset function:
void sunSet(){
    if(reset != 1){
        for(i=0; i<100; i++) {   //for the first 100 LEDs
                    strip_a.setPixelColor(i, 150, 20, 0);   //set LEDs red-ish in cloud one
                  }
                  for(i=101; i<150; i++) {  //for LEDs 101 to 150
                    strip_a.setPixelColor(i, 150, 90, 0);  //set LEDs orange-red in cloud one
                  }
                  for(i=151; i<250; i++) {  //for LEDs 151 to 250
                    strip_a.setPixelColor(i, 150, 50, 0);  //set LEDs red-orage in cloud one
                  }
                  for(i=251; i<260; i++) {  //for LEDs 251 to 260
                    strip_a.setPixelColor(i, 150, 50, 0);  //set LEDs red-orage in cloud one
                  }
                  for(i=261; i<300; i++) {  //for LEDs 261 to the last LED
                     strip_a.setPixelColor(i, Wheel(((i * 256 / 300)) & 255));  //set LEDs to the first part of the color wheel (red-yellow) in cloud one
          }
          strip_a.show(); //show all the colors that were set in cloud one
    }
}

//Clear cloud function:
void clearCloud(){
    if(reset != 1){
      for (i = 0; i < 300; i++){ //for all the LEDs
          strip_a.setPixelColor(i, 0, 0, 0); //turn off in cloud one   
          }
    
          strip_a.show(); //show what was set in cloud one
    }
}

//lightning storm function:
void lightningStorm() {
  while(reset != 1){
    //Various types of lightning functions, where the first two 
    //numbers represent the area that the flash could randomly show 
    //up in, the next number is usually how long the flash is (in milisec)
    //the forth number is sometimes the size of the flash, and the
    //last is the color setting for the flash:
      if(reset != 1){jumpingFlash_a(50, 80, 50, strip_a.Color(255,255,255));}
      if(reset != 1){scrollingFlash_a(20, 65, 50, 5, strip_a.Color(255,255,255));}
      if(reset != 1){singleFlash_a(100, 200, 50, 15, strip_a.Color(255,255,255));}
      if(reset != 1){singleFlash_a(50, 100, 50, 5, strip_a.Color(200,200,255));}
      if(reset != 1){scrollingFlash_a(200, 250, 50, 15, strip_a.Color(255,255,255));}
      if(reset != 1){jumpingFlash_a(100, 130, 50, strip_a.Color(255,255,255));}
      if(reset != 1){multipleFlashs_a(20, 125, 150, 300, 50, 5, strip_a.Color(255,255,255));}
      if(reset != 1){scrollingFlash_a(10, 60, 100, 15, strip_a.Color(225,200,255));}
      if(reset != 1){flickerFlash_a(75, 175, 40, 25, strip_a.Color(255,255,255));}
      if(reset != 1){jumpingFlash_a(200, 130, 50, strip_a.Color(255,255,255));}
      if(reset != 1){flickerFlash_a(50, 300, 50, 25, strip_a.Color(200,200,255));}
      if(reset != 1){scrollingFlash_a(200, 250, 100, 10, strip_a.Color(255,255,255));}
      if(reset != 1){multipleFlashs_a(20, 125, 175, 300, 50, 5, strip_a.Color(255,255,255));}
      if(reset != 1){scrollingFlash_a(20, 65, 50, 3, strip_a.Color(255,255,255));}
      if(reset != 1){singleFlash_a(75, 175, 40, 3, strip_a.Color(255,255,255));}
      if(reset != 1){singleFlash_a(100, 200, 50, 30, strip_a.Color(255,255,255));}
      if(reset != 1){scrollingFlash_a(200, 500, 50, 15, strip_a.Color(255,255,255));}
      if(reset != 1){jumpingFlash_a(250, 300, 50, strip_a.Color(255,255,255));}
      if(reset != 1){multipleFlashs_a(20, 125, 200, 300, 50, 5, strip_a.Color(255,255,255));}
      if(reset != 1){singleFlash_a(75, 175, 40, 3, strip_a.Color(255,255,255));}
      if(reset != 1){jumpingFlash_a(150, 180, 50, strip_a.Color(255,255,255));}
      if(reset != 1){jumpingFlash_a(50, 80, 50, strip_a.Color(255,255,255));}
      if(reset != 1){flickerFlash_a(0, 100, 50, 50, strip_a.Color(200,200,255));}
      if(reset != 1){multipleFlashs_a(20, 125, 150, 300, 50, 5, strip_a.Color(200,200,255));}
      if(reset != 1){wholeCloudFlash_a(40, 100, strip_a.Color(255,255,255));}
  }
}
  

void singleFlash_a(int areaMin, int areaMax, int duration, int Size, uint32_t color) {
  int i;
  int flashStrike = random(areaMin, areaMax);

        for ((i = flashStrike - Size); (i < flashStrike); i++) {
         strip_a.setPixelColor(i, color);
       }
       strip_a.show();
       delay(duration);
       for ((i = flashStrike - Size); (i < flashStrike); i++) {
         strip_a.setPixelColor(i, 0, 0, 0);
       }
       strip_a.show();
       int delayTime = random(900, 1500);
       delay(delayTime);
 
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
         int delayTime = random(900, 1500);
         delay(delayTime);
  
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
          int delayTime = random(800, 1200);
          delay(delayTime);
 

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
         int delayTime = (600, 1200);
         delay(delayTime);
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
           int delayTime = random(100, 600);
           delay (delayTime);
 
}


void wholeCloudFlash_a(int durationShort, int durationLong, uint32_t color) {
  int i;
          for (i = 60; i < 75; i++){
           strip_a.setPixelColor(i, color);
         }
         strip_a.show();
         delay(durationShort);
         for (i = 60; i < 75; i++){
           strip_a.setPixelColor(i, 0, 0, 0);
         }
            strip_a.show();
            delay(durationLong);
         for (i = 60; i < 75; i++){
           strip_a.setPixelColor(i, color);
           strip_a.show();
         }
         delay(durationShort);
         for (i = 60; i < 75; i++){
           strip_a.setPixelColor(i, 0, 0, 0);
           strip_a.show();
         }
          for (i = 100; i < 115; i++){
           strip_a.setPixelColor(i, color);
           strip_a.show();
         }
         for (i = 100; i < 115; i++){
           strip_a.setPixelColor(i, 0, 0, 0);
           strip_a.show();
         }
         for (i = 100; i < 115; i++){
           strip_a.setPixelColor(i, color);
           strip_a.show();
         }
         delay(durationShort);
         for (i = 100; i < 115; i++){
           strip_a.setPixelColor(i, 0, 0, 0);
           strip_a.show();
         }
         for (i = 115; i < 130; i++){
           strip_a.setPixelColor(i, color);
         }
         strip_a.show();
         delay(durationShort);
         for (i = 115; i < 130; i++){
           strip_a.setPixelColor(i, 0, 0, 0);
         }
         strip_a.show();
         delay(durationShort);
         for (i = 115; i < 130; i++){
           strip_a.setPixelColor(i, color);
         }
         strip_a.show();
         delay(durationShort);
         for (i = 115; i < 130; i++){
           strip_a.setPixelColor(i, 0, 0, 0);
           strip_a.show();
         }
         for (i = 130; i < 150; i++){
           strip_a.setPixelColor(i, color);
         }
         strip_a.show();
         delay(durationShort);
         for (i = 130; i < 150; i++){
           strip_a.setPixelColor(i, 0, 0, 0);
         }
         strip_a.show();
         delay(durationShort);
         for (i = 130; i < 150; i++){
           strip_a.setPixelColor(i, color);
         }
         strip_a.show();
         delay(durationLong);
         for (i = 130; i < 150; i++){
           strip_a.setPixelColor(i, 0, 0, 0);
           strip_a.show();
         }
         int delayTime = random(50, 500);
         delay (delayTime);
 
}

void disco(int Speed, int duration){
  int i, j;
  int red;
  int green;
  int blue;
  while(reset != 1){
        for (j = 0; j < duration; j++){
          if(reset != 1){       
                  red = random(150);
                  green = random(250);
                  blue = random(200);
                for (i = 0; i < 50; i++){
                  strip_a.setPixelColor(i, red, green, blue);
                }
                strip_a.show();
                delay(Speed);
                  for (i = 0; i < 300; i++){
                  strip_a.setPixelColor(i, 0, 0, 0);
                  }
                  strip_a.show();
                 delay(Speed);
          }
           

          if(reset != 1){   
                   red = random(250);
                   green = random(250);
                   blue = random(250);
                  for (i = 200; i < 300; i++){
                    strip_a.setPixelColor(i, red, green, blue);
                  }
                  strip_a.show();
                  delay(Speed);
                    for (i = 0; i < 300; i++){
                    strip_a.setPixelColor(i, 0, 0, 0);
                    }
                    strip_a.show();
                    delay(Speed);
          }
                    
          if(reset != 1){           
                  red = random(250);
                  green = random(250);
                  blue = random(250);
                  for (i = 60; i < 110; i++){
                    strip_a.setPixelColor(i, red, green, blue);
                  }
                  strip_a.show();
                  delay(Speed);
                    for (i = 0; i < 300; i++){
                    strip_a.setPixelColor(i, 0, 0, 0);
                    }
                    strip_a.show();
                    delay(Speed);
          }
           
                    
          if(reset != 1){           
                  red = random(200);
                  green = random(200);
                  blue = random(200);
                  for (i = 125; i < 175; i++){
                    strip_a.setPixelColor(i, red, green, blue);
                  }
                  strip_a.show();
                  delay(Speed);
                    for (i = 0; i < 300; i++){
                    strip_a.setPixelColor(i, 0, 0, 0);
                    }
                    strip_a.show();
                    delay(Speed);
          }
          if(reset != 1){           
                  red = random(200);
                  green = random(200);
                  blue = random(200);
                  for (i = 200; i < 300; i++){
                    strip_a.setPixelColor(i, red, green, blue);
                  }
                  strip_a.show();
                  delay(Speed);
                    for (i = 0; i < 300; i++){
                    strip_a.setPixelColor(i, 0, 0, 0);
                    }
                    strip_a.show();
                    delay(Speed);
          }
          if(reset != 1){          
                  red = random(200);
                  green = random(200);
                  blue = random(200);
                  for (i = 0; i < 110; i++){
                    strip_a.setPixelColor(i, red, green, blue);
                  }
                  strip_a.show();
                  delay(Speed);
                    for (i = 0; i < 300; i++){
                    strip_a.setPixelColor(i, 0, 0, 0);
                    }
                    strip_a.show();
                    delay(Speed);
          }
                    
          if(reset != 1){       
                    red = random(200);
                  green = random(200);
                  blue = random(200);
                  for (i = 60; i < 110; i++){
                    strip_a.setPixelColor(i, red, green, blue);
                  }
                  strip_a.show();
                  delay(Speed);
                    for (i = 0; i < 300; i++){
                    strip_a.setPixelColor(i, 0, 0, 0);
                    }
                    strip_a.show();
                    delay(Speed);
          }
          if(reset != 1){           
                   red = random(200);
                   green = random(200);
                   blue = random(200);
                  for (i = 0; i < 50; i++){
                    strip_a.setPixelColor(i, red, green, blue);
                  }
                  strip_a.show();
                  delay(Speed);
                    for (i = 0; i < 300; i++){
                    strip_a.setPixelColor(i, 0, 0, 0);
                    }
                    strip_a.show();
                    delay(Speed);
          }
                    
          if(reset != 1){           
                  red = random(200);
                  green = random(200);
                  blue = random(200);
                  for (i = 125; i < 175; i++){
                    strip_a.setPixelColor(i, red, green, blue);
                  }
                  strip_a.show();
                  delay(Speed);
                    for (i = 0; i < 300; i++){
                    strip_a.setPixelColor(i, 0, 0, 0);
                    }
                    strip_a.show();
                    delay(Speed);
          }
            
            
        }
  }
}

