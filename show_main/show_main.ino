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

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
// If using the breakout, change pins as desired
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

#define LEFT_EDGE 0
#define RIGHT_EDGE (tft.width() - 1)
#define TOP_EDGE 0
#define BOTTOM_EDGE (tft.height() - 1)
#define COLUMN_SIZE (textSize * 6)
#define ROW_SIZE (textSize * 8)
#define SHOW_DEFAULT 1
#define SHOW_IMAGE 5
#define UNSUPPORTED_ESCAPE_SEQUENCE "Unsupported escape sequence received."

const char version[] = "v2.1";

typedef struct cursor
{
  uint32_t x;
  uint32_t y;
} cursor;

cursor cursor_save = {0, 0};
cursor cursor_prev = {0, 0};

uint8_t pwm = 255;
uint8_t ledPin = 5; // PWM LED Backlight control to digital pin 5
uint8_t textSize = 2;
uint8_t rotation = 1;
uint16_t foregroundColor = ILI9341_WHITE;
uint16_t backgroundColor = ILI9341_BLACK;
uint32_t imgDelay = 0;
uint32_t serialDelay = 0;
uint8_t length = 0;
uint8_t lengthFlag = 0;
uint8_t ackFlag = 0;
uint8_t readFlag = 0;
uint8_t current_state = SHOW_DEFAULT;
uint8_t previous_state = SHOW_DEFAULT;
uint32_t imgsize = 0;
uint32_t sizecnt = 0;
uint8_t rgb565hi, rgb565lo;
uint8_t cntenable = 0;
uint8_t c;

unsigned char btn0Presses = 0;
unsigned char btn0Releases = 0;
unsigned char btn1Presses = 0;
unsigned char btn1Releases = 0;
unsigned char btn2Presses = 0;
unsigned char btn2Releases = 0;
unsigned char btn0Pushed = 0;
unsigned char btn1Pushed = 0;
unsigned char btn2Pushed = 0;

void initPins()
{
  pinMode(ledPin, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, INPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  analogWrite(ledPin, pwm);
}

void readBtn()
{
  if (!digitalRead(A1) && (btn2Presses == 0))
  {
    btn2Presses = 1;
    btn2Releases = 0;
    btn2Pushed = 1;
    digitalWrite(6, LOW);
  }

  if (digitalRead(A1) && (btn2Releases == 0))
  {
    btn2Releases = 1;
    btn2Presses = 0;
    btn2Pushed = 0;
    digitalWrite(6, HIGH);
  }

  if (!digitalRead(7) && (btn0Presses == 0))
  {
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

  if (digitalRead(7) && (btn0Releases == 0))
  {
    btn0Releases = 1;
    btn0Presses = 0;
    btn0Pushed = 0;
    digitalWrite(3, HIGH);
  }

  if (!digitalRead(A0) && (btn1Presses == 0))
  {
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

  if (digitalRead(A0) && (btn1Releases == 0))
  {
    btn1Releases = 1;
    btn1Presses = 0;
    btn1Pushed = 0;
    digitalWrite(4, HIGH);
  }
}

void timerCallback()
{
  imgDelay++;
  serialDelay++;
  readBtn();

  if (serialDelay > 20)
  {
    lengthFlag = 0;
    readFlag = 0;
    ackFlag = 0;
  }
  if (lengthFlag)
  {
    if (Serial.read() > (length + 2))
    {
      readFlag = 1;
      Serial.print(6);
      lengthFlag = 0;
    }
  }
}

void switchstate(int newstate)
{
  previous_state = current_state;
  current_state = newstate;
}

void showDiagnostics()
{
  Serial.println("Diagnostics");
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x");
  Serial.println(x, HEX);
}

void setImageDrawingStatus(uint16_t start_x, uint16_t start_y, uint16_t image_width, uint16_t image_height)
{
  imgsize = (image_width - start_x) * (image_height - start_y);
  tft.setAddrWindow(start_x, start_y, image_width - 1, image_height - 1);
  tft.startWrite();
  switchstate(SHOW_IMAGE);
  Serial.println("Cat the raw image data over serial now.");
}

void drawPixel(int16_t point_x, int16_t point_y, uint16_t colour)
{
  Serial.print("Plotting x, y: ");
  Serial.print(point_x);
  Serial.print("/");
  Serial.println(point_y);
  tft.drawPixel(point_x, point_y, colour);
}

void setCursorColumnRow(int16_t col = 1, int16_t row = 1)
{
  // Ensure column and row are within the screen area
  if (col >= 1 && col <= MAX_COLUMNS && row >= 1 and row <= MAX_ROWS)
  {
    cursor_prev.x = tft.getCursorX();
    cursor_prev.y = tft.getCursorY();

    tft.setCursor((col - 1) * COLUMN_SIZE, (row - 1) * ROW_SIZE);
}
  else
{
    Serial.print(INVALID_POSITION_A);
    Serial.print(col);
    Serial.print(INVALID_POSITION_B);
    Serial.print(row);
    Serial.println(INVALID_POSITION_C);
  }
}

void setCursorXY(int16_t x = 0, int16_t y = 0)
{
  // Ensure x, y coordinates are within the screen dimensions
  if (x >= 0 && x <= RIGHT_EDGE && y >= 0 and y <= BOTTOM_EDGE)
  {
    cursor_prev.x = tft.getCursorX();
    cursor_prev.y = tft.getCursorY();
    tft.setCursor(x, y);
  }
  else
  {
    Serial.print(INVALID_POSITION_A);
    Serial.print(x);
    Serial.print(INVALID_POSITION_B);
    Serial.print(y);
    Serial.println(INVALID_POSITION_C);
  }
}

void resetCursorXY()
{
  // Goes back to previous cursor position, this is different to the saved cursor position
  setCursorXY(cursor_prev.x, cursor_prev.y);
}

void reportCursorPosition()
{
  String string_x = String(tft.getCursorX());
  String string_y = String(tft.getCursorY());
  // Save "program storage space" by using multiple `Serial.print()` statements instead of
  // concatenating the string and using a single `Serial.println()` which is more costly.
  Serial.print("Cursor Position: (");
  Serial.print(string_x);
  Serial.print(", ");
  Serial.print(string_y);
  Serial.println(")");
}

uint16_t getNamedColour(int option)
{
  switch (option)
  {
  case 0:
    return ILI9341_BLACK;
  case 1:
    return ILI9341_RED;
  case 2:
    return ILI9341_GREEN;
  case 3:
    return ILI9341_YELLOW;
  case 4:
    return ILI9341_BLUE;
  case 5:
    return ILI9341_MAGENTA;
  case 6:
    return ILI9341_CYAN;
  case 7:
    return ILI9341_WHITE;
  case 8:
    return ILI9341_NAVY;
  case 9:
    return ILI9341_DARKGREEN;
  case 10:
    return ILI9341_DARKCYAN;
  case 11:
    return ILI9341_MAROON;
  case 12:
    return ILI9341_PURPLE;
  case 13:
    return ILI9341_OLIVE;
  case 14:
    return ILI9341_LIGHTGREY;
  case 15:
    return ILI9341_DARKGREY;
  case 16:
    return ILI9341_ORANGE;
  case 17:
    return ILI9341_GREENYELLOW;
  case 18:
    return ILI9341_PINK;
  }
}

uint16_t getRGBColours(uint16_t R, uint16_t G, uint16_t B)
{
  uint16_t RGB = ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | ((B & 0xF8) >> 3);
  return RGB;
}

void setColours(uint16_t foreground = ILI9341_WHITE, uint16_t background = ILI9341_BLACK)
{
  tft.setTextColor(foreground, background);
}

void resetDefaults()
{
  tft.setTextSize(2);
  tft.setRotation(1);
  setColours(ILI9341_WHITE, ILI9341_BLACK);
  tft.fillScreen(ILI9341_BLACK);
  setCursorXY();
}

void resetScreen() {
  tft.fillScreen(backgroundColour);
  setCursorXY();
}

void carriageReturn()
{
  setCursorXY(0, tft.getCursorY());
}

void lineFeed()
{
  setCursorXY(tft.getCursorX(), (tft.getCursorY() + ROW_SIZE));
}

void verticalTab()
{
  setCursorXY(tft.getCursorX(), (tft.getCursorY() + (4 * ROW_SIZE)));
}

void horizontalTab()
{
  setCursorXY((tft.getCursorX() + (4 * COLUMN_SIZE)), tft.getCursorY());
}

void backSpace()
{
  setCursorXY((tft.getCursorX() - COLUMN_SIZE), tft.getCursorY());
  tft.print(" ");
  setCursorXY((tft.getCursorX() - COLUMN_SIZE), tft.getCursorY());
}

void saveCursorPosition()
{
  cursor_save.x = tft.getCursorX();
  cursor_save.y = tft.getCursorY();
}

void restoreSavedCursorPosition()
{
  setCursorXY(cursor_save.x, cursor_save.y);
}

void cursorUp(int rows = 1)
{
  setCursorXY(tft.getCursorX(), (tft.getCursorY() - (rows * ROW_SIZE)));
}

void cursorDown(int rows = 1)
{
  setCursorXY(tft.getCursorX(), (tft.getCursorY() + (rows * ROW_SIZE)));
}

void cursorLeft(int columns = 1)
{
  setCursorXY((tft.getCursorX() - (columns * COLUMN_SIZE)), tft.getCursorY());
}

void cursorRight(int columns = 1)
{
  setCursorXY((tft.getCursorX() + (columns * COLUMN_SIZE)), tft.getCursorY());
}

void setup()
{
  Serial.begin(500000);
  Serial.println("Welcome to the Odroid-SHOW");

  tft.begin();
  initPins();
  // show diagnostics over serial
  showDiagnostics();

  tft.setRotation(rotation);
  tft.setTextSize(textSize);
  setCursorXY(50, 50);
  tft.print("Hello ODROID-SHOW!");
  setCursorXY(250, 200);
  tft.print(version);
  delay(1000);
  tft.fillScreen(backgroundColor);
  setCursorXY(0, 0);
  Timer1.initialize(20000);
  Timer1.attachInterrupt(timerCallback);
}

void loop(void)
{
  if (current_state == SHOW_IMAGE)
  {
    if (Serial.available() > 1)
    {
      rgb565lo = Serial.read();
      rgb565hi = Serial.read();
      tft.spiWrite(rgb565hi);
      tft.spiWrite(rgb565lo);
      cntenable = 1;
      sizecnt++;
      imgDelay = 0;
    }
    else if (cntenable == 1)
    {
      if ((sizecnt == imgsize) || (imgDelay > 100))
      {
        cntenable = 0;
        sizecnt = 0;
        tft.endWrite();
        tft.setAddrWindow(0, 0, tft.width(), tft.height());
        switchstate(SHOW_DEFAULT);
      }
    }
  }
  else
  {
    if (Serial.available() > 0)
    {
      c = Serial.read();
      tft.print((char)c);
    }
  }
}