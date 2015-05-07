#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
//#include "printf.h"

#define MSG_ADDR      msg[0]
#define MSG_UNUSED01  msg[1]
#define MSG_CMD       msg[2]
#define MSG_MOD1      msg[3]
#define MSG_MOD2      msg[4]
#define MSG_MOD3      msg[5]
#define MSG_MOD4      msg[6]
#define MSG_UNUSED02  msg[7]
#define MSG_UNUSED03  msg[8]
#define MSG_UNUSED04  msg[9]
#define MSG_UNUSED05  msg[10]
#define MSG_UNUSED06  msg[11]
#define MSG_UNUSED07  msg[12]
#define MSG_UNUSED08  msg[13]
#define MSG_LED1R     msg[14]
#define MSG_LED1G     msg[15]
#define MSG_LED1B     msg[16]
#define MSG_UNUSED12  msg[17]
#define MSG_UNUSED13  msg[18]
#define MSG_LED2R     msg[19]
#define MSG_LED2G     msg[20]
#define MSG_LED2B     msg[21]
#define MSG_UNUSED17  msg[22]
#define MSG_UNUSED18  msg[23]
#define MSG_LED3R     msg[24]
#define MSG_LED3G     msg[25]
#define MSG_LED3B     msg[26]
#define MSG_UNUSED22  msg[27]
#define MSG_UNUSED23  msg[28]
#define MSG_LED4R     msg[29]
#define MSG_LED4G     msg[30]
#define MSG_LED4B     msg[31]



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
          red = MSG_LED1R;
          green = MSG_LED1G;
          blue = MSG_LED1B;
          strip.setPixelColor(0, strip.Color(red, green, blue));
          strip.setPixelColor(1, strip.Color(red, green, blue));
          strip.setPixelColor(2, strip.Color(red, green, blue));
          strip.setPixelColor(3, strip.Color(red, green, blue));
          strip.show();
          //delay(rate);
          done = process_msg();
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
          done = process_msg();
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
          done = process_msg();
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
    if (MSG_ADDR == 0 || MSG_ADDR == addr){
      return true;
    }
    // for safety reset msg target to a value that matches no lantern
    MSG_ADDR = 255;
    return false;
  }
}

bool process_msg(void){
  if(check_radio() != true){
    return false;
  }
  radio.read(msg, 32);
  switch (MSG_CMD){
    case 'P':    // set pattern number
      if (MSG_MOD1 != pattern){  // new pattern
        pattern = MSG_MOD1;
        return true;
      }
      else{
        return false;
      }
      break;
    case 'S':    // set speed of the pattern
      rate = MSG_MOD1;
      return false;
      break;
    default:
      return false;
  }
}


// eof



