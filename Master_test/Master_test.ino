#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
//#include "printf.h"
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define LED_PIN 6
#define ADDR_PIN 7
#define NUMPIXELS 4

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

byte pattern;
byte rate;
char cmnd;

byte msg[32];
RF24 radio(9,10);
int x;
int n;
int numPixels = 4;

const uint64_t pipe = 0xE8E8F0F0E1LL;
unsigned int counter = 0;
long lasttime;
long nexttime;

void setup(void){
//  Serial.begin(57600);
//  printf_begin(); 
  radio.begin();
  
  radio.setAutoAck(0);
  radio.setPALevel(RF24_PA_HIGH);
//  radio.printDetails();  
  radio.openWritingPipe(pipe);

  nexttime = millis()+1000;
  
  strip.begin();
  strip.setPixelColor(0, strip.Color(255,0,0));
  strip.setPixelColor(0, strip.Color(0,255,0));
  strip.setPixelColor(0, strip.Color(0,0,255));
  strip.setPixelColor(0, strip.Color(0,255,255));
  
  strip.show(); // Initialize all pixels to 'off'

  nexttime = millis();  // + 30000;
  pattern = 0;
}

void loop(void){
  
  if (millis() > nexttime){
     nexttime = millis() + 3000;
     n++;
     if (n > 9){
       pattern++;
       if (pattern > 1) pattern = 0;
       n=0;
     }
     msg[2] = 'P';
     msg[3] = pattern;
     
     msg[14] = 255;
     msg[15] = 0;
     msg[16] = 0;
     radio.write(msg,32);
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      msg[i*3] = c >> 16;
      msg[i*3+1] = c >> 8;
      msg[i*3+2] = c;
      radio.write(msg,32);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;
  uint32_t c;
  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      c = Wheel((i+j) & 255);
      strip.setPixelColor(i, c);
      msg[i*3] = c >> 16;
      msg[i*3+1] = c >> 8;
      msg[i*3+2] = c;      
    }
    radio.write(msg,32);
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  uint32_t c;
  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      c = Wheel(((i * 256 / strip.numPixels()) + j) & 255);
//      Wheel(((i * 256 / strip.numPixels()) + j) & 255);
      strip.setPixelColor(i, c);
      msg[i*3] = c >> 16;
      msg[i*3+1] = c >> 8;
      msg[i*3+2] = c;
    }
    radio.write(msg,32);
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i++) {
          if (i+q == x){
            strip.setPixelColor(i+q, 0);    //turn every third pixel off
            msg[i*3] = 0;
            msg[i*3+1] = 0;
            msg[i*3+2] = 0;            
          }
          else
          {
            strip.setPixelColor(i+q, c);    //turn every other pixel on
            msg[i*3] = c >> 16;
            msg[i*3+1] = c >> 8;
            msg[i*3+2] = c;
          }
      }
      radio.write(msg,32);
      strip.show();     
      delay(wait);
     
    }
    x++;
    if (x >= strip.numPixels()) x=0;
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  uint32_t c;
  x=0;
  for (int j=0; j < 256; j+=5) {     // cycle all 256 colors in the wheel

    for (int q=0; q < 3; q++) {

        for (int i=0; i < strip.numPixels(); i++) {
          if (i+q == x){
            strip.setPixelColor(i+q, 0);    //turn every third pixel off
            msg[i*3] = 0;
            msg[i*3+1] = 0;
            msg[i*3+2] = 0;            
          }
          else
          {
            c = Wheel((i+j) % 255);
            strip.setPixelColor(i+q, c);    //turn every other pixel on
            msg[i*3] = c >> 16;
            msg[i*3+1] = c >> 8;
            msg[i*3+2] = c;
          }
        }
        radio.write(msg,32);
        strip.show();
        delay(wait);
    }
    
    x++;
    if (x >= strip.numPixels()) x=0;

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

