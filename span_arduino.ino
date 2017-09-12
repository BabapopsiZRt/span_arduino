/*
Arduino firmware for Techfest '17

TODO:
-serialParse() error handling
-reset fctn?
-LED start animation
-rgbw support

written by David Baka
*/

#include "HSB_Color.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_DRV2605.h"

#define NUM_OF_SERIAL_ARGS 6
//#define motorTest 0        //to continuously test motors in loop
#define debug 1         // set to 1 for debug messages over serial
#define MUX_ADDR 0x70
#define PIN 6         //NeoPixewl pin
#define MAX_BRIGHTNESS 50
#define FAR_COLOR 255     //if we map around to 360deg we get back to red again... last argument sets color of farthest objects
#define LED 13

//NeoPixel parameters
/* Parameter 1 = number of pixels in strip
   Parameter 2 = pin number (most are valid)
   Parameter 3 = pixel type flags, add together as needed:
     NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
     NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
     NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
     NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)*/
Adafruit_NeoPixel strip = Adafruit_NeoPixel(6, PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_DRV2605 drv;

const byte MaxNumChars = 32;
char ReceivedChars[MaxNumChars];
int temp = 0;
char * strtokIndx;
byte CurrentIndex = 0;
const char EndMarker = '\n';
char ReceivedChar;

int getHapticType(int cmd) {
    switch (cmd) {
    case 0:
      return 16;
    case 1:
      return 47;
    case 2:
      return 10;
    case 3:
      return 2;
    case 4:
      return 6;
    case 5:
      return 7;
    case 6:
      return 9;
    default:
      return 9;
    }
}

//selects output on i2c MUX
void muxSelect(int i)
{
  if (i > 7) return;

  Wire.beginTransmission(MUX_ADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

//writes command[] to vibr motors
void vibeWrite(int _i, int _command)
{
  muxSelect(_i);
  sendHaptic(_command);
}

void sendHaptic(int _cmd)
{
  drv.setWaveform(0, _cmd);   // play effect 
  drv.setWaveform(1, 0);              // end waveform
  drv.go();
}

void setLED(int _i, int _command)
{
  int rgb[3];
  int val = map(_command, 0, 9, 0, 120);
  hsi2rgb(val, 255, MAX_BRIGHTNESS, rgb);
  strip.setPixelColor(_i, rgb[0], rgb[1], rgb[2], 0);
  strip.show();

  Serial.print("LED hue:");
  Serial.println(val);
}

void testHaptic(int sensorId) {
    muxSelect((int)sensorId);
    Serial.print("testing S");
    Serial.println(sensorId);
    sendHaptic(1);
}

int knightRiderValues[6] = {0, 0, 0, 0, 0, 0};
int KNIGHT_RIDER_DELAY = 20;

void knightRider_showLights() {
  for(int i = 0; i<6; i++) {
    strip.setPixelColor(i, knightRiderValues[i], 0, 0, 0);
    strip.show();
  }
}

void knightRider_fadeOut() {
  for(int i = 0; i < 6; i++) {
    if(knightRiderValues[i] <= (int) (255/6)) {
      knightRiderValues[i] = 0;
    } else {
      knightRiderValues[i] /= 2;
    }
  }
}

void knightRider() {
  int lightStrength[6] = {0, 0, 0, 0, 0, 0};
  Serial.println("Hello Michael!");
  for(int round = 0 ; round < 6; round++) {
  testHaptic(round);
    for(int i = 0; i<6; i++) {
      knightRider_fadeOut();
      knightRiderValues[i] = 255;
      knightRider_showLights();
      delay(KNIGHT_RIDER_DELAY);
    }
    for(int i = 5; i>=0; i--) {
      knightRider_fadeOut();
      knightRiderValues[i] = 255;
      knightRider_showLights();
      delay(KNIGHT_RIDER_DELAY);
    }
  }
  // final fade out
  for(int i = 0; i<6; i++) {
    knightRider_fadeOut();
    knightRider_showLights();
    delay(KNIGHT_RIDER_DELAY);
  }
  Serial.println("Super Pursuit Mode ACTIVATED");
}

void parseData() {
  // this is used by strtok() as an index
  strtokIndx = strtok(ReceivedChars, ";");      // get the first part - the string
  for (int i = 0; i < 6; i++) {
    temp = atoi(strtokIndx);     // convert this part to an integer
  vibeWrite(i, temp);
  setLED(i, temp);
    if (debug) {
    Serial.print("To S");
    Serial.print(i);
    Serial.print(" set value: ");
    Serial.println(temp);
    }
  
    strtokIndx = strtok(NULL, ";");      // get the first part - the string
  }
}

void ReadFromSerial() {
  while (Serial.available() > 0)
  {
    ReceivedChar = Serial.read();

    if (ReceivedChar != EndMarker)
    {
      ReceivedChars[CurrentIndex] = ReceivedChar;
      CurrentIndex++;

      if (CurrentIndex >= MaxNumChars)
      {
        CurrentIndex = MaxNumChars - 1;
      }
    }
    else
    {
      ReceivedChars[CurrentIndex] = '\0'; // terminate the string
      CurrentIndex = 0;
      parseData();
    }
  }
}

void setup() {
  Serial.begin(115200);
  //Serial.setTimeout(200);
  
  Wire.begin();
  
  strip.begin();
  strip.setBrightness(MAX_BRIGHTNESS);
  strip.show();
  
  drv.begin();

  //cycle through all DRVs duh...
  for (int i = 0; i < 6; i++)
  {
    muxSelect(i);
    drv.setMode(DRV2605_MODE_INTTRIG);
  }

  // cycleThrough();      //useful when it's on somebody's head, not useful when testing the serial comms
  knightRider();
  
  if(debug) Serial.println("setup complete");

  //bm_LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
}

void loop() 
{
  ReadFromSerial();
}

