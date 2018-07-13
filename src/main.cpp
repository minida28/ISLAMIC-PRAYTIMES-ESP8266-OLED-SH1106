/**
   The MIT License (MIT)

   Copyright (c) 2016 by Daniel Eichhorn
   Copyright (c) 2016 by Fabrice Weinberg

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.

*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "sholat.h"
#include "sholathelper.h"
#include "timehelper.h"
#include <Ticker.h>
#include <StreamString.h>
#include <time.h>
#include <sys/time.h>  // struct timeval
#include <coredecls.h> // settimeofday_cb()
#include "EspGoodies.h"

// Include the correct display library
// For a connection via I2C using Wire include
// #include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "SH1106.h" // alis for `#include "SH1106Wire.h"`
// For a connection via I2C using brzo_i2c (must be installed) include
// #include <brzo_i2c.h> // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Brzo.h"
// #include "SH1106Brzo.h"
// For a connection via SPI include
// #include <SPI.h> // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Spi.h"
// #include "SH1106SPi.h"

// Include the UI lib
#include "OLEDDisplayUi.h"

// Include custom images
#include "images.h"

#define ESP_PIN_0 0   //D3
#define ESP_PIN_1 1   //Tx
#define ESP_PIN_2 2   //D4 -> Led on ESP8266
#define ESP_PIN_3 3   //D9(Rx)
#define ESP_PIN_4 4   //D2
#define ESP_PIN_5 5   //D1
#define ESP_PIN_9 9   //S2
#define ESP_PIN_10 10 //S3
#define ESP_PIN_12 12 //D6
#define ESP_PIN_13 13 //D7
#define ESP_PIN_14 14 //D5
#define ESP_PIN_15 15 //D8
#define ESP_PIN_16 16 //D0 -> Led on NodeMcu

#define IO_EXPANDER_PIN_0 0
#define IO_EXPANDER_PIN_1 1
#define IO_EXPANDER_PIN_2 2
#define IO_EXPANDER_PIN_3 3
#define IO_EXPANDER_PIN_4 4
#define IO_EXPANDER_PIN_5 5
#define IO_EXPANDER_PIN_6 6
#define IO_EXPANDER_PIN_7 7

#define SDA ESP_PIN_0               //D3
#define SCL ESP_PIN_4               //D2
#define BUZZER ESP_PIN_14           //D5
#define MCU_INTERRUPT_PIN ESP_PIN_5 //D1

#ifdef USE_IO_EXPANDER

#define A IO_EXPANDER_PIN_7
#define B IO_EXPANDER_PIN_6
#define C IO_EXPANDER_PIN_5
#define D IO_EXPANDER_PIN_4

#define RTC_SQW_PIN IO_EXPANDER_PIN_0
#define ENCODER_SW_PIN IO_EXPANDER_PIN_1
#define ENCODER_CLK_PIN IO_EXPANDER_PIN_2
#define ENCODER_DT_PIN IO_EXPANDER_PIN_3

#endif

// Use the corresponding display class:

// Initialize the OLED display using SPI
// D5 -> CLK
// D7 -> MOSI (DOUT)
// D0 -> RES
// D2 -> DC
// D8 -> CS
// SSD1306Spi        display(D0, D2, D8);
// or
// SH1106Spi         display(D0, D2);

// Initialize the OLED display using brzo_i2c
// D3 -> SDA
// D5 -> SCL
// SSD1306Brzo display(0x3c, D3, D5);
// or
// SH1106Brzo display(0x3c, D3, D5);

// Initialize the OLED display using Wire library
// SSD1306  display(0x3c, D3, D5);
SH1106 display(0x3c, SDA, SCL);

OLEDDisplayUi ui(&display);

int screenW = 128;
int screenH = 64;
int clockCenterX = screenW / 2;
int clockCenterY = ((screenH - 16) / 2) + 16; // top yellow part is 16 px height
int clockRadius = 23;

const char ssid[] = "your_wifi_ssid";     //  your network SSID (name)
const char pass[] = "your_wifi_password"; // your network password

// utility function for digital clock display: prints leading 0
char *twoDigits(int digits)
{
  static char buf[3];
  snprintf(buf, sizeof(buf), "%02d", digits);
  return buf;
}

void clockOverlay(OLEDDisplay *display, OLEDDisplayUiState *state)
{
}

void analogClockFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
  //  ui.disableIndicator();

  // Draw the clock face
  //  display->drawCircle(clockCenterX + x, clockCenterY + y, clockRadius);
  display->drawCircle(clockCenterX + x, clockCenterY + y, 2);
  //
  //hour ticks
  for (int z = 0; z < 360; z = z + 30)
  {
    //Begin at 0° and stop at 360°
    float angle = z;
    angle = (angle / 57.29577951); //Convert degrees to radians
    int x2 = (clockCenterX + (sin(angle) * clockRadius));
    int y2 = (clockCenterY - (cos(angle) * clockRadius));
    int x3 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 8))));
    int y3 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 8))));
    display->drawLine(x2 + x, y2 + y, x3 + x, y3 + y);
  }

  time_t rawtime;
  time(&rawtime);

  struct tm *tm;
  tm = localtime(&rawtime);

  uint8_t hour;
  uint8_t minute;
  uint8_t second;

  hour = tm->tm_hour;
  minute = tm->tm_min;
  second = tm->tm_sec;

  // display second hand
  float angle = second * 6;
  angle = (angle / 57.29577951); //Convert degrees to radians
  int x3 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 5))));
  int y3 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 5))));
  display->drawLine(clockCenterX + x, clockCenterY + y, x3 + x, y3 + y);
  //
  // display minute hand
  angle = minute * 6;
  angle = (angle / 57.29577951); //Convert degrees to radians
  x3 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 4))));
  y3 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 4))));
  display->drawLine(clockCenterX + x, clockCenterY + y, x3 + x, y3 + y);
  //
  // display hour hand
  angle = hour * 30 + int((minute / 12) * 6);
  angle = (angle / 57.29577951); //Convert degrees to radians
  x3 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 2))));
  y3 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 2))));
  display->drawLine(clockCenterX + x, clockCenterY + y, x3 + x, y3 + y);
}

void digitalClockFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(clockCenterX + x, 24, getTimeStr());
}

void timeLeft(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
  char buf[20];
  if (HOUR != 0)
  {
    snprintf(buf, sizeof(buf), "%d h %d m", HOUR, MINUTE);
  }
  else
  {
    snprintf(buf, sizeof(buf), "%d m %d s", MINUTE, SECOND);
  }
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(64 + x, 14 + y, buf);
  snprintf(buf, sizeof(buf), "to %s", sholatNameStr(NEXTTIMEID));
  display->drawString(64 + x, 37 + y, buf);
}

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState *state)
{
  time_t rawtime;
  time(&rawtime);

  struct tm *tm;
  tm = localtime(&rawtime);

  uint8_t hour;
  uint8_t minute;

  hour = tm->tm_hour;
  minute = tm->tm_min;

  char buf[6];
  snprintf(buf, sizeof(buf), "%s:%s", itoa(hour, buf, 10), twoDigits(minute));
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(128, 0, buf);
}

// This array keeps function pointers to all frames
// frames are the single views that slide in
//FrameCallback frames[] = { analogClockFrame, digitalClockFrame };
// FrameCallback frames[] = {timeLeft, digitalClockFrame, analogClockFrame};
FrameCallback frames[] = {timeLeft, digitalClockFrame};
// FrameCallback frames[] = {timeLeft};

// how many frames are there?
int frameCount = 2;

// Overlays are statically drawn on top of a frame eg. a clock
//OverlayCallback overlays[] = { clockOverlay };
OverlayCallback overlays[] = {msOverlay};
int overlaysCount = 1;

// double times[sizeof(TimeName) / sizeof(char*)];

//void p(char *fmt, ... ) {
void p(const char *fmt, ...)
{
  char tmp[128]; // resulting string limited to 128 chars
  va_list args;
  va_start(args, fmt);
  vsnprintf(tmp, 128, fmt, args);
  va_end(args);
  Serial.print(tmp);
}

Ticker ticker;

timeval cbtime; // time set in callback
bool cbtime_set = false;
bool updateSholat = false;

void time_is_set(void)
{
  gettimeofday(&cbtime, NULL);
  cbtime_set = true;
  updateSholat = true;
  Serial.println("------------------ settimeofday() was called ------------------");
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());

  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(60);

  // Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(TOP);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();

  settimeofday_cb(time_is_set);

  // configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  configTime(0, 0, "id.pool.ntp.org", "asia.pool.ntp.org", "pool.ntp.org");
  // setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 3);
  // tzset();
  configTZ(TZ_Asia_Jakarta);
  // configTZ(TZ_Asia_Kathmandu);

  delay(1000);

  // timeSetup();

  //  unsigned long secsSinceStart = millis();
  //  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  //  const unsigned long seventyYears = 2208988800UL;
  //  // subtract seventy years:
  //  unsigned long epoch = secsSinceStart - seventyYears * SECS_PER_HOUR;
  //  setTime(epoch);

  // flip the pin every 0.3s
  // ticker.attach(5, check_time_status);

  // process_sholat();
  // process_sholat_2nd_stage();

  Serial.println(F("Setup completed\r\n"));
}

time_t prevDisplay = 0; // when the digital clock was displayed

// for testing purpose:
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

#define PTM(w)              \
  Serial.print(":" #w "="); \
  Serial.print(tm->tm_##w);

void printTm(const char *what, const tm *tm)
{
  Serial.print(what);
  PTM(isdst);
  PTM(yday);
  PTM(wday);
  PTM(year);
  PTM(mon);
  PTM(mday);
  PTM(hour);
  PTM(min);
  PTM(sec);
}

timeval tv;
timespec tp;
time_t now;
uint32_t now_ms, now_us;

void loop()
{
  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0)
  {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.

    static uint32_t timer100ms = 0;
    if (millis() >= timer100ms + 100)
    {
      timer100ms = millis();

      gettimeofday(&tv, nullptr);
      clock_gettime(0, &tp);
      now = time(nullptr);
      now_ms = millis();
      now_us = micros();

      // localtime / gmtime every second change
      static time_t lastv = 0;
      if (lastv != tv.tv_sec)
      {
        lastv = tv.tv_sec;

        tick1000ms = true;

        Serial.println();
        printTm("localtime", localtime(&now));
        Serial.println();
        printTm("gmtime   ", gmtime(&now));
        Serial.println();

        // time from boot
        Serial.print("clock:");
        Serial.print((uint32_t)tp.tv_sec);
        Serial.print("/");
        Serial.print((uint32_t)tp.tv_nsec);
        Serial.print("ns");

        // time from boot
        Serial.print(" millis:");
        Serial.print(now_ms);
        Serial.print(" micros:");
        Serial.print(now_us);

        // EPOCH+tz+dst
        Serial.print(" gtod:");
        Serial.print((uint32_t)tv.tv_sec);
        Serial.print("/");
        Serial.print((uint32_t)tv.tv_usec);
        Serial.print("us");

        // EPOCH+tz+dst
        Serial.print(" time:");
        Serial.println((uint32_t)now);

        // human readable
        // Printed format: Wed Oct 05 2011 16:48:00 GMT+0200 (CEST)        
        char buf[60];
        strftime(buf, sizeof(buf), "%a %b %d %Y %X GMT%z (%Z)", localtime(&now));
        Serial.print("LOCAL date/time: ");
        Serial.println(buf);
        strftime(buf, sizeof(buf), "%a %b %d %Y %X GMT", gmtime(&now));
        Serial.print("GMT   date/time: ");
        Serial.println(buf);
        Serial.println();
      }
    }

    if (updateSholat)
    {
      updateSholat = false;
      process_sholat();
      // process_sholat_2nd_stage();
    }

    //update the display every 1000ms
    if (tick1000ms)
    {
      // if (timeStatus() != timeNotSet)
      // {
      //   ProcessSholatEverySecond();
      // }
      ProcessSholatEverySecond();
    }

    // timeLoop();

    tick1000ms = false;

    delay(remainingTimeBudget);
  }
}
