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

#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* clock=*/SCL, /* data=*/SDA, /* reset=*/U8X8_PIN_NONE);

void u8g2_prepare(void)
{
  // u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFont(u8g2_font_courB10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void u8g2_box_frame(uint8_t a)
{
  u8g2.drawStr(0, 0, "drawBox");
  u8g2.drawBox(5, 10, 20, 10);
  u8g2.drawBox(10 + a, 15, 30, 7);
  u8g2.drawStr(0, 30, "drawFrame");
  u8g2.drawFrame(5, 10 + 30, 20, 10);
  u8g2.drawFrame(10 + a, 15 + 30, 30, 7);
}

void u8g2_disc_circle(uint8_t a)
{
  u8g2.drawStr(0, 0, "drawDisc");
  u8g2.drawDisc(10, 18, 9);
  u8g2.drawDisc(24 + a, 16, 7);
  u8g2.drawStr(0, 30, "drawCircle");
  u8g2.drawCircle(10, 18 + 30, 9);
  u8g2.drawCircle(24 + a, 16 + 30, 7);
}

void u8g2_r_frame(uint8_t a)
{
  u8g2.drawStr(0, 0, "drawRFrame/Box");
  u8g2.drawRFrame(5, 10, 40, 30, a + 1);
  u8g2.drawRBox(50, 10, 25, 40, a + 1);
}

// void u8g2_string(uint8_t a)
// {
//   u8g2.setFontDirection(0);
//   u8g2.drawStr(30 + a, 31, " 0");
//   u8g2.setFontDirection(1);
//   u8g2.drawStr(30, 31 + a, " 90");
//   u8g2.setFontDirection(2);
//   u8g2.drawStr(30 - a, 31, " 180");
//   u8g2.setFontDirection(3);
//   u8g2.drawStr(30, 31 - a, " 270");
// }

void u8g2_string(uint16_t a)
{
  u8g2.setFontDirection(0);
  char buf[36];
  if (HOUR != 0)
  {
    snprintf(buf, sizeof(buf), "%d jam %d menit menuju %s", HOUR, MINUTE, sholatNameStr(NEXTTIMEID));
  }
  else
  {
    snprintf(buf, sizeof(buf), "%d jam %d menit menuju %s", MINUTE, SECOND, sholatNameStr(NEXTTIMEID));
  }
  u8g2.drawStr(128 - a, 0, buf);
}

void u8g2_line(uint8_t a)
{
  u8g2.drawStr(0, 0, "drawLine");
  u8g2.drawLine(7 + a, 10, 40, 55);
  u8g2.drawLine(7 + a * 2, 10, 60, 55);
  u8g2.drawLine(7 + a * 3, 10, 80, 55);
  u8g2.drawLine(7 + a * 4, 10, 100, 55);
}

void u8g2_triangle(uint8_t a)
{
  uint16_t offset = a;
  u8g2.drawStr(0, 0, "drawTriangle");
  u8g2.drawTriangle(14, 7, 45, 30, 10, 40);
  u8g2.drawTriangle(14 + offset, 7 - offset, 45 + offset, 30 - offset, 57 + offset, 10 - offset);
  u8g2.drawTriangle(57 + offset * 2, 10, 45 + offset * 2, 30, 86 + offset * 2, 53);
  u8g2.drawTriangle(10 + offset, 40 + offset, 45 + offset, 30 + offset, 86 + offset, 53 + offset);
}

void u8g2_ascii_1()
{
  char s[2] = " ";
  uint8_t x, y;
  u8g2.drawStr(0, 0, "ASCII page 1");
  for (y = 0; y < 6; y++)
  {
    for (x = 0; x < 16; x++)
    {
      s[0] = y * 16 + x + 32;
      u8g2.drawStr(x * 7, y * 10 + 10, s);
    }
  }
}

void u8g2_ascii_2()
{
  char s[2] = " ";
  uint8_t x, y;
  u8g2.drawStr(0, 0, "ASCII page 2");
  for (y = 0; y < 6; y++)
  {
    for (x = 0; x < 16; x++)
    {
      s[0] = y * 16 + x + 160;
      u8g2.drawStr(x * 7, y * 10 + 10, s);
    }
  }
}

void u8g2_extra_page(uint8_t a)
{
  u8g2.drawStr(0, 0, "Unicode");
  u8g2.setFont(u8g2_font_unifont_t_symbols);
  u8g2.setFontPosTop();
  u8g2.drawUTF8(0, 24, "☀ ☁");
  switch (a)
  {
  case 0:
  case 1:
  case 2:
  case 3:
    u8g2.drawUTF8(a * 3, 36, "☂");
    break;
  case 4:
  case 5:
  case 6:
  case 7:
    u8g2.drawUTF8(a * 3, 36, "☔");
    break;
  }
}

#define cross_width 24
#define cross_height 24
static const unsigned char cross_bits[] U8X8_PROGMEM = {
    0x00,
    0x18,
    0x00,
    0x00,
    0x24,
    0x00,
    0x00,
    0x24,
    0x00,
    0x00,
    0x42,
    0x00,
    0x00,
    0x42,
    0x00,
    0x00,
    0x42,
    0x00,
    0x00,
    0x81,
    0x00,
    0x00,
    0x81,
    0x00,
    0xC0,
    0x00,
    0x03,
    0x38,
    0x3C,
    0x1C,
    0x06,
    0x42,
    0x60,
    0x01,
    0x42,
    0x80,
    0x01,
    0x42,
    0x80,
    0x06,
    0x42,
    0x60,
    0x38,
    0x3C,
    0x1C,
    0xC0,
    0x00,
    0x03,
    0x00,
    0x81,
    0x00,
    0x00,
    0x81,
    0x00,
    0x00,
    0x42,
    0x00,
    0x00,
    0x42,
    0x00,
    0x00,
    0x42,
    0x00,
    0x00,
    0x24,
    0x00,
    0x00,
    0x24,
    0x00,
    0x00,
    0x18,
    0x00,
};

#define cross_fill_width 24
#define cross_fill_height 24
static const unsigned char cross_fill_bits[] U8X8_PROGMEM = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x18,
    0x00,
    0x18,
    0x64,
    0x00,
    0x26,
    0x84,
    0x00,
    0x21,
    0x08,
    0x81,
    0x10,
    0x08,
    0x42,
    0x10,
    0x10,
    0x3C,
    0x08,
    0x20,
    0x00,
    0x04,
    0x40,
    0x00,
    0x02,
    0x80,
    0x00,
    0x01,
    0x80,
    0x18,
    0x01,
    0x80,
    0x18,
    0x01,
    0x80,
    0x00,
    0x01,
    0x40,
    0x00,
    0x02,
    0x20,
    0x00,
    0x04,
    0x10,
    0x3C,
    0x08,
    0x08,
    0x42,
    0x10,
    0x08,
    0x81,
    0x10,
    0x84,
    0x00,
    0x21,
    0x64,
    0x00,
    0x26,
    0x18,
    0x00,
    0x18,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

#define cross_block_width 14
#define cross_block_height 14
static const unsigned char cross_block_bits[] U8X8_PROGMEM = {
    0xFF,
    0x3F,
    0x01,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0xC1,
    0x20,
    0xC1,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0xFF,
    0x3F,
};

void u8g2_bitmap_overlay(uint8_t a)
{
  uint8_t frame_size = 28;

  u8g2.drawStr(0, 0, "Bitmap overlay");

  u8g2.drawStr(0, frame_size + 12, "Solid / transparent");
  u8g2.setBitmapMode(false /* solid */);
  u8g2.drawFrame(0, 10, frame_size, frame_size);
  u8g2.drawXBMP(2, 12, cross_width, cross_height, cross_bits);
  if (a & 4)
    u8g2.drawXBMP(7, 17, cross_block_width, cross_block_height, cross_block_bits);

  u8g2.setBitmapMode(true /* transparent*/);
  u8g2.drawFrame(frame_size + 5, 10, frame_size, frame_size);
  u8g2.drawXBMP(frame_size + 7, 12, cross_width, cross_height, cross_bits);
  if (a & 4)
    u8g2.drawXBMP(frame_size + 12, 17, cross_block_width, cross_block_height, cross_block_bits);
}

void u8g2_bitmap_modes(uint8_t transparent)
{
  const uint8_t frame_size = 24;

  u8g2.drawBox(0, frame_size * 0.5, frame_size * 5, frame_size);
  u8g2.drawStr(frame_size * 0.5, 50, "Black");
  u8g2.drawStr(frame_size * 2, 50, "White");
  u8g2.drawStr(frame_size * 3.5, 50, "XOR");

  if (!transparent)
  {
    u8g2.setBitmapMode(false /* solid */);
    u8g2.drawStr(0, 0, "Solid bitmap");
  }
  else
  {
    u8g2.setBitmapMode(true /* transparent*/);
    u8g2.drawStr(0, 0, "Transparent bitmap");
  }
  u8g2.setDrawColor(0); // Black
  u8g2.drawXBMP(frame_size * 0.5, 24, cross_width, cross_height, cross_bits);
  u8g2.setDrawColor(1); // White
  u8g2.drawXBMP(frame_size * 2, 24, cross_width, cross_height, cross_bits);
  u8g2.setDrawColor(2); // XOR
  u8g2.drawXBMP(frame_size * 3.5, 24, cross_width, cross_height, cross_bits);
}

uint8_t draw_state = 24;

void draw(void)
{
  u8g2_prepare();
  switch (draw_state >> 3)
  {
  case 0:
    u8g2_box_frame(draw_state & 7);
    break;
  case 1:
    u8g2_disc_circle(draw_state & 7);
    break;
  case 2:
    u8g2_r_frame(draw_state & 7);
    break;
  case 3:
    u8g2_string(draw_state & 7);
    break;
  case 4:
    u8g2_line(draw_state & 7);
    break;
  case 5:
    u8g2_triangle(draw_state & 7);
    break;
  case 6:
    u8g2_ascii_1();
    break;
  case 7:
    u8g2_ascii_2();
    break;
  case 8:
    u8g2_extra_page(draw_state & 7);
    break;
  case 9:
    u8g2_bitmap_modes(0);
    break;
  case 10:
    u8g2_bitmap_modes(1);
    break;
  case 11:
    u8g2_bitmap_overlay(draw_state & 7);
    break;
  }
}

timeval cbtime; // time set in callback
bool cbtime_set = false;
bool updateSholat = false;

bool tick100ms = 0;
bool tick3000ms = 0;

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

  settimeofday_cb(time_is_set);

  // configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  configTime(0, 0, "id.pool.ntp.org", "asia.pool.ntp.org", "pool.ntp.org");
  // setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 3);
  // tzset();
  configTZ(TZ_Asia_Jakarta);
  // configTZ(TZ_Asia_Kathmandu);

  Wire.begin(SDA, SCL);
  u8g2.begin();
  // flip screen, if required
  // u8g2.setRot180();

  // assign default color value
  // draw_color = 1; // pixel on

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
    static uint32_t timer100ms = 0;
    if (millis() >= timer100ms + 50)
    {
      timer100ms = millis();
      tick100ms = true;
    }

    static uint32_t timer3000ms = 0;
    if (millis() >= timer3000ms + 3000)
    {
      timer3000ms = millis();
      tick3000ms = true;
    }

    if (updateSholat)
    {
      updateSholat = false;
      process_sholat();
      // process_sholat_2nd_stage();
    }

    if (tick100ms)
    {
      tick100ms = false;

      // picture loop
      u8g2.clearBuffer();

      static uint16_t x = 0;
      x++;
      if (x >= 256)
      {
        x = 0;
      }
      u8g2_prepare();
      u8g2_string(x);

      u8g2.sendBuffer();

      // // increase the state
      // draw_state++;
      // if (draw_state >= 4 * 8)
      // {
      //   draw_state = 24;
      // }



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

    //update the display every 1000ms
    if (tick1000ms)
    {
      tick1000ms = false;
      ProcessSholatEverySecond();
    }
}
