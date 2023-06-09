/***************************************************
  This is our GFX example for the Adafruit ILI9341 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/


#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "TimerOne.h"

// For the Adafruit shield, these are the default.
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10
#define SERIAL_RX_BUFFER_SIZE 64

const char version[] = "v2.1";

volatile uint8_t _rx_buffer_head = 0;
volatile uint8_t _rx_buffer_tail = 0;

uint8_t pwm = 255;
uint8_t ledPin = 5; // PWM LED Backlight control to digital pin 5
uint8_t textSize = 2;
uint8_t rotation = 1;
uint16_t foregroundColor, backgroundColor;
uint32_t imgDelay = 0;
uint32_t serialDelay = 0;
uint8_t length = 0;
uint8_t lengthFlag = 0;
uint8_t ackFlag = 0;
uint8_t readFlag = 0;
unsigned char btn0Presses = 0;
unsigned char btn0Releases = 0;
unsigned char btn1Presses = 0;
unsigned char btn1Releases = 0;
unsigned char btn2Presses = 0;
unsigned char btn2Releases = 0;
unsigned char btn0Pushed = 0;
unsigned char btn1Pushed = 0;
unsigned char btn2Pushed = 0;

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
// If using the breakout, change pins as desired
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

void setup() {
  Serial.begin(500000);
  Serial.println("Welcome to the Odroid-SHOW"); 
 
  tft.begin();
  initPins();

  // read diagnostics (optional but can help debug problems)
  Serial.println("Diagnostics");
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX);

  tft.setRotation(rotation);
  tft.setTextSize(textSize);
  tft.setCursor(50, 50);
  tft.print("Hello ODROID-SHOW!");
  tft.setCursor(250, 200);
  tft.print(version);
  delay(1000);
  tft.fillScreen(backgroundColor);
  tft.setCursor(0, 0);
  Timer1.initialize(20000);
  Timer1.attachInterrupt(timerCallback);
}

void initPins(){
  pinMode(ledPin, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, INPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  analogWrite(ledPin, pwm);
}

void readBtn(){
  if (!digitalRead(A1) && (btn2Presses == 0)) {
    btn2Presses = 1;
    btn2Releases = 0;
    btn2Pushed = 1;
    digitalWrite(6, LOW);
  }

  if (digitalRead(A1) && (btn2Releases == 0)) {
    btn2Releases = 1;
    btn2Presses = 0;
    btn2Pushed = 0;
    digitalWrite(6, HIGH);
  }

  if (!digitalRead(7) && (btn0Presses == 0)) {
    btn0Presses = 1;
    btn0Releases = 0;
    btn0Pushed = 1;
      if (pwm > 225)
        pwm = 255;
      else
        pwm += 30;
    analogWrite(ledPin, pwm);
    digitalWrite(3, LOW);
  }

  if (digitalRead(7) && (btn0Releases == 0)) {
    btn0Releases = 1;
    btn0Presses = 0;
    btn0Pushed = 0;
    digitalWrite(3, HIGH);
  }

  if (!digitalRead(A0) && (btn1Presses == 0)) {
    btn1Presses = 1;
    btn1Releases = 0;
    btn1Pushed = 1;
    if (pwm < 30)
      pwm = 0;
    else
      pwm -= 30;
    analogWrite(ledPin, pwm);
    digitalWrite(4, LOW);
  }

  if (digitalRead(A0) && (btn1Releases == 0)) {
    btn1Releases = 1;
    btn1Presses = 0;
    btn1Pushed = 0;
    digitalWrite(4, HIGH);
  }
}

void timerCallback(){
  imgDelay++;
  serialDelay++;
  readBtn();

  if (serialDelay > 20) {
    lengthFlag = 0;
    readFlag = 0;
    ackFlag = 0;
  }
  if (lengthFlag) {
    if (getBufferSize() > (length + 2)) {
      readFlag = 1;
      Serial.print(6);
      lengthFlag = 0;
    }
  }
}

uint8_t getBufferSize(void){
  if (_rx_buffer_head >= _rx_buffer_tail)
    return SERIAL_RX_BUFFER_SIZE - _rx_buffer_head + _rx_buffer_tail;
  return _rx_buffer_tail - _rx_buffer_head;
}

void loop(void) {
  for(uint8_t rotation=0; rotation<4; rotation++) {
    tft.setRotation(rotation);
    testFillScreen();
  }
}

unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(ILI9341_BLACK);
  yield();
  tft.fillScreen(ILI9341_RED);
  yield();
  tft.fillScreen(ILI9341_GREEN);
  yield();
  tft.fillScreen(ILI9341_BLUE);
  yield();
  tft.fillScreen(ILI9341_BLACK);
  yield();
  return micros() - start;
}