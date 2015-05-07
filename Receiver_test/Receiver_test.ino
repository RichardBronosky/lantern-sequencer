
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
//#include "printf.h"

#define PIN 6

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(4, PIN, NEO_GRB + NEO_KHZ800);

byte msg[32];
byte pattern = 1;
byte rate = 10;

const byte addr = 8;

unsigned int red;
unsigned int green;
unsigned int blue;

unsigned long timeout;

const uint64_t pipe = 0xE8E8F0F0E1LL;
RF24 radio(9,10);

void setup(void){

  Serial.begin(57600);
//  printf_begin();
  
  radio.begin();
  radio.setAutoAck(0);
//  radio.printDetails();  
  radio.openReadingPipe(1,pipe);
  radio.startListening();

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  timeout = millis() + 10000;

}

void loop(void){
  bool done;
  byte n;
  byte i;
  
  strip.setPixelColor(0, strip.Color(255,0,0));
  strip.setPixelColor(1, strip.Color(0,255,0));
  strip.setPixelColor(2, strip.Color(0,0,255));
  strip.setPixelColor(3, strip.Color(0,255,255));
  strip.show();
  
  while(1){
    done = false;
    switch (pattern){

      case 0:  // display r, g and b from master
      while(!done){
        red = msg[14];
        green = msg[15];
        blue = msg[16];
        strip.setPixelColor(0, strip.Color(red, green, blue));
        strip.setPixelColor(1, strip.Color(red, green, blue));
        strip.setPixelColor(2, strip.Color(red, green, blue));
        strip.setPixelColor(3, strip.Color(red, green, blue));
        strip.show();
        //delay(rate);      
        if(check_radio()){done = process_msg();}   
        if (millis() > timeout){
          timeout = millis()+60000;
          pattern = 2;
          done = true;
        }  
      }
      break;
 
     case 1:  // color rotating around circle
      n = (addr-1) * 16;
      while (!done){
        if (n<16){
          strip.setPixelColor(0, strip.Color(255,0,0));
          strip.setPixelColor(1, strip.Color(255,0,0));
          strip.setPixelColor(2, strip.Color(255,0,0));
          strip.setPixelColor(3, strip.Color(255,0,0));
          strip.show();          
        }
        else
        {
          strip.setPixelColor(0, strip.Color(0,0,255));
          strip.setPixelColor(1, strip.Color(0,0,255));
          strip.setPixelColor(2, strip.Color(0,0,255));
          strip.setPixelColor(3, strip.Color(0,0,255));
          strip.show();
        }
        n++;
        if(check_radio()){done = process_msg();}
        if (millis() > timeout){
          timeout = millis()+60000;
          pattern = 2;
          done = true;
        }  
        delay(rate);
      }
      break;

     case 2:  // rainbow rotating around
      n = (addr-1) * 16;
      while (!done){
          for(i=0; i< strip.numPixels(); i++) {
            strip.setPixelColor(i, Wheel((64 + n) & 255));
          }
          strip.show();
          delay(rate);
        n++;
        if(check_radio()){done = process_msg();}
        if (millis() > timeout){
          timeout = millis()+60000;
          pattern = 2;
          done = true;
        }  
        delay(rate);
      }     
      break;

     default:
      pattern = 2;
    }
  
  }
  
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

bool check_radio(void){
  if (radio.available()){
   timeout = millis() + 10000;
   if (msg[1] == 0){     
     return true;
   }
   else if (msg[1] == addr){
     return true;
   }
   else
   {
     return false; 
   }
  }
  else
  {
    return false; 
  }
}

bool process_msg(void){
  radio.read(msg, 32);
  byte cmnd = msg[2];
  switch (cmnd){
    case 'P':    // set pattern number
      if (msg[3] != pattern){  // new pattern
       pattern = msg[3];
       return true;
      }
      else{
        return false;
      }
    break;
    case 'S':    // set speed of the pattern
      rate = msg[3];
      return false;
    break;
    default:
      return false;
  }
}


// eof



