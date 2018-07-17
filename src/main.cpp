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

#define SDA ESP_PIN_4               //D2
#define SCL ESP_PIN_5               //D1
#define BUZZER ESP_PIN_14           //D5
#define MCU_INTERRUPT_PIN ESP_PIN_0 //D3

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

Ticker state500msTimer;

bool tick100ms = false;
bool tick3000ms = false;
bool state500ms = false;

timeval tv;
timespec tp;
// time_t now;
uint32_t now_ms, now_us;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* clock=*/SCL, /* data=*/SDA, /* reset=*/U8X8_PIN_NONE);

// void u8g2_prepare(void)
// {
//   // u8g2.setFont(u8g2_font_6x10_tf);
//   // u8g2.setFont(u8g2_font_courB10_tf);
//   u8g2.setFont(u8g2_font_profont15_tf);
//   u8g2.setFontRefHeightExtendedText();
//   u8g2.setDrawColor(1);
//   u8g2.setFontPosTop();
//   u8g2.setFontDirection(0);
// }

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

void u8g2_string(uint16_t a)
{
  u8g2.setFontDirection(0);
  char buf[36];
  if (HOUR != 0)
  {
    snprintf(buf, sizeof(buf), "%d jam %2d menit menuju %s.", HOUR, MINUTE, sholatNameStr(NEXTTIMEID));
  }
  else
  {
    snprintf(buf, sizeof(buf), "%d menit %2d detik menuju %s.", MINUTE, SECOND, sholatNameStr(NEXTTIMEID));
  }
  u8g2.drawStr(128 - a, 0, buf);
}

void u8g2_string2(uint16_t a)
{
  u8g2.setFontDirection(0);
  // u8g2.setFont(u8g2_font_profont15_tf);
  u8g2.setFont(u8g2_font_logisoso16_tf);
  char buf[36];
  if (state500ms)
  {
    snprintf(buf, sizeof(buf), "- %d:%02d:%02d", HOUR, MINUTE, SECOND);
  }
  else
  {
    snprintf(buf, sizeof(buf), "  %d:%02d:%02d", HOUR, MINUTE, SECOND);
  }
  u8g2.drawStr(0, 0, buf);

  if (state500ms)
  {
    snprintf(buf, sizeof(buf), "- %d:%02d", ceilHOUR, ceilMINUTE);
  }
  else
  {
    snprintf(buf, sizeof(buf), "  %d:%02d", ceilHOUR, ceilMINUTE);
  }
  u8g2.drawStr(0, 20, buf);
}

void u8g2_string3(uint16_t a)
{
  // time_t rawtime = time(nullptr);
  tm *tm = localtime(&now);

  uint8_t hr, min /*, sec*/;
  hr = tm->tm_hour;
  min = tm->tm_min;
  // sec = tm->tm_sec;

  char buf[36];

  const uint8_t *fontC;

  // fontC = u8g2_font_6x10_mn;
  fontC = u8g2_font_smart_patrol_nbp_tn;
  // fontC = u8g2_font_profont15_mn;
  // fontC = u8g2_font_px437wyse700b_mn;
  // fontC = u8g2_font_profont17_mn;
  // fontC = u8g2_font_t0_18_me;
  // fontC = u8g2_font_balthasar_titling_nbp_tn;
  // fontC = u8g2_font_logisoso16_tn;

  u8g2.setFont(fontC);

  bool format12 = false;
  if (format12)
  {
    if (hr > 12)
    {
      hr = hr - 12;
    }
  }

  int dW;
  int dH;
  const char *str;
  int sW;

  dW = u8g2.getDisplayWidth();
  snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
  sW = u8g2.getStrWidth(buf);

  u8g2.setFontPosTop();
  u8g2.setCursor((dW - sW) / 2, -1);

  u8g2.print(buf);

  bool blinkColon = true;
  if (blinkColon)
  {
    if (!state500ms)
    {
      u8g2.setCursor((dW - sW) / 2, -1);
      u8g2.print(hr);
      u8g2.setFontMode(1);
      u8g2.setDrawColor(0);
      u8g2.print(":");
      u8g2.setFontMode(0);  // default
      u8g2.setDrawColor(1); // default
    }
  }

  // u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  // u8g2.setFontPosTop();
  u8g2.setFontPosBottom();
  // u8g2.setFontDirection(0);

  if (true)
  {
    // u8g2.setFont(u8g2_font_helvB12_tf);
    // snprintf(buf, sizeof(buf), "%d jam %d menit", ceilHOUR, ceilMINUTE);

    const uint8_t *font0;
    const uint8_t *font1;

    // font0 = u8g2_font_helvB12_tn;
    // font0 = u8g2_font_t0_16_mn;
    // font0 = u8g2_font_logisoso16_tn;
    // font0 = u8g2_font_smart_patrol_nbp_tn;
    font0 = u8g2_font_profont15_mn;

    // font1 = u8g2_font_t0_12_mr
    font1 = u8g2_font_t0_12_tr;

    u8g2.setCursor(3, 32);
    u8g2.setFont(font0);
    u8g2.print(HOUR);

    u8g2.setCursor(u8g2.tx + 2, u8g2.ty - 0);
    u8g2.setFont(font1);
    u8g2.print("jam ");

    u8g2.setCursor(u8g2.tx, u8g2.ty + 0);
    snprintf(buf, sizeof(buf), "%2d", MINUTE);
    u8g2.setFont(font0);
    u8g2.print(buf);

    u8g2.setCursor(u8g2.tx + 2, u8g2.ty - 0);
    u8g2.setFont(font1);
    u8g2.print("mnt ");

    u8g2.setCursor(u8g2.tx, u8g2.ty + 0);
    u8g2.setFont(font0);
    snprintf(buf, sizeof(buf), "%2d", SECOND);
    u8g2.print(buf);

    u8g2.setCursor(u8g2.tx + 2, u8g2.ty - 0);
    u8g2.setFont(font1);
    u8g2.print("dtk");
  }
  else if (ceilHOUR == 0)
  {
    u8g2.setFont(u8g2_font_logisoso16_tf);
    snprintf(buf, sizeof(buf), "%d menit", ceilMINUTE);
  }

  u8g2.setFont(u8g2_font_t0_12_mr);

  dW = u8g2.getDisplayWidth();
  dH = u8g2.getDisplayHeight();
  str = "menuju";
  sW = u8g2.getStrWidth(str);
  u8g2.setFontPosTop();
  u8g2.setCursor((dW - sW) / 2, u8g2.ty + 2);
  u8g2.print(str);

  u8g2.setFontPosBottom();
  // u8g2.setCursor(0, 67);
  // u8g2.setCursor(u8g2.tx + 5, 20);
  // u8g2.setFont(u8g2_font_logisoso16_tf);
  // u8g2.setFont(u8g2_font_inb16_mr);
  // u8g2.setFont(u8g2_font_helvB12_tf);
  u8g2.setFont(u8g2_font_smart_patrol_nbp_tr);

  dW = u8g2.getDisplayWidth();
  dH = u8g2.getDisplayHeight();
  str = sholatNameStr(NEXTTIMEID);
  sW = u8g2.getStrWidth(str);

  // Serial.printf("dW=%d, dH=%d, sW=%d\r\n", dW, dH, sW);

  u8g2.setCursor((dW - sW) / 2, dH + 0);
  // u8g2.setCursor(26, 63);

  u8g2.print(str);

  // const uint8_t *f;
  // f = u8g2_font_crox4hb_tr;

  // u8g2.setFontPosTop();
  // u8g2.setFont(f);
  // u8g2.setCursor(0, 0);
  // u8g2.print("AgjIy180");

  // u8g2.drawHLine(0, 0, 127);
  // u8g2.drawHLine(0, 15, 127);
  // u8g2.drawHLine(0, 31, 127);
  // u8g2.drawHLine(0, 47, 127);
  // u8g2.drawHLine(0, 63, 127);

  // u8g2.drawVLine(127, 0, 63);
  // u8g2.drawStr(0, 20, buf);
  // u8g2.setFont(u8g2_font_logisoso16_tf);
  // snprintf(buf, sizeof(buf), "%s", sholatNameStr(NEXTTIMEID));
  // u8g2.drawStr(u8g2.tx, 20, buf);
}

void u8g2_string4(uint16_t a)
{
  tm *tm = localtime(&now);

  uint8_t hr, min /*, sec*/;
  hr = tm->tm_hour;
  min = tm->tm_min;
  // sec = tm->tm_sec;

  char buf[36];

  const uint8_t *fontC;

  // fontC = u8g2_font_6x10_mn;
  // fontC = u8g2_font_smart_patrol_nbp_tn;
  // fontC = u8g2_font_profont15_mn;
  // fontC = u8g2_font_px437wyse700b_mn;
  // fontC = u8g2_font_profont17_mn;
  // fontC = u8g2_font_t0_18_me;
  // fontC = u8g2_font_balthasar_titling_nbp_tn;
  // fontC = u8g2_font_logisoso16_tn;
  fontC = u8g2_font_logisoso46_tn;
  // fontC = u8g2_font_inb33_mn;
  // fontC = u8g2_font_inr33_mn;

  u8g2.setFont(fontC);

  bool format12 = true;
  if (format12)
  {
    if (hr == 0)
    {
      hr = 12;
    }

    if (hr > 12)
    {
      hr = hr - 12;
    }
  }

  int dW;
  int dH;
  // const char *str;
  int sW;
  int ascent;

  int x, y;

  dH = u8g2.getDisplayHeight();
  dW = u8g2.getDisplayWidth();
  snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
  sW = u8g2.getStrWidth(buf);
  ascent = u8g2.getAscent();

  u8g2.setFontPosTop();

  if (buf[0] == char(49)) // number 1
  {
    x = (dW - sW - 10) / 2;
    y = (dH - ascent) / 2;
  }
  else
  {
    x = (dW - sW) / 2;
    y = (dH - ascent) / 2;
  }

  u8g2.setCursor(x, y);

  u8g2.print(buf);

  bool blinkColon = true;
  if (blinkColon)
  {
    if (!state500ms)
    {
      u8g2.setCursor(x, y);
      snprintf(buf, sizeof(buf), "%d", hr);
      u8g2.print(buf);
      u8g2.setFontMode(1);
      u8g2.setDrawColor(0);
      u8g2.print(":");
      u8g2.setFontMode(0);  // default
      u8g2.setDrawColor(1); // default
    }
  }
}

void u8g2_string5(uint16_t a)
{
  tm *tm = localtime(&now);

  uint8_t hr, min /*, sec*/;
  hr = tm->tm_hour;
  min = tm->tm_min;
  // sec = tm->tm_sec;

  char buf[36];

  const uint8_t *fontC;

  // fontC = u8g2_font_6x10_mn;
  // fontC = u8g2_font_smart_patrol_nbp_tn;
  // fontC = u8g2_font_profont15_mn;
  // fontC = u8g2_font_px437wyse700b_mn;
  // fontC = u8g2_font_profont17_mn;
  // fontC = u8g2_font_t0_18_me;
  // fontC = u8g2_font_balthasar_titling_nbp_tn;
  // fontC = u8g2_font_logisoso16_tn;
  fontC = u8g2_font_logisoso42_tn;
  // fontC = u8g2_font_logisoso46_tn;
  // fontC = u8g2_font_inb33_mn;
  // fontC = u8g2_font_inr33_mn;

  u8g2.setFont(fontC);

  bool format12 = true;
  if (format12)
  {
    if (hr == 0)
    {
      hr = 12;
    }

    if (hr > 12)
    {
      hr = hr - 12;
    }
  }

  int dW;
  // int dH;
  // const char *str;
  int sW;
  // int ascent;

  int x, y;

  dW = u8g2.getDisplayWidth();
  snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
  sW = u8g2.getStrWidth(buf);

  u8g2.setFontPosTop();

  if (buf[0] == char(49)) // number 1
  {
    x = (dW - sW - 8) / 2;
    y = 0;
  }
  else
  {
    x = (dW - sW) / 2;
    y = 0;
  }

  u8g2.setCursor(x, y);

  u8g2.print(buf);

  bool blinkColon = true;
  if (blinkColon)
  {
    if (!state500ms)
    {
      u8g2.setCursor(x, y);
      snprintf(buf, sizeof(buf), "%d", hr);
      u8g2.print(buf);
      u8g2.setFontMode(1);
      u8g2.setDrawColor(0);
      u8g2.print(":");
      u8g2.setFontMode(1);  // default
      u8g2.setDrawColor(1); // default
    }
  }

  // snprintf(buf, sizeof(buf), "%s %s", sholatNameStr(NEXTTIMEID), sholatTimeArray[NEXTTIMEID]);

  if (!state500ms)
  {
    snprintf(buf, sizeof(buf), "%s  %dh %2dm", sholatNameStr(NEXTTIMEID), ceilHOUR, ceilMINUTE);
  }
  else
  {
    snprintf(buf, sizeof(buf), "%s  %dh %2dm", sholatNameStr(NEXTTIMEID), ceilHOUR, ceilMINUTE);
  }

  u8g2.setFontPosBottom();

  // fontC = u8g2_font_smart_patrol_nbp_tr;
  // fontC = u8g2_font_8x13_tr;
  fontC = u8g2_font_7x13_tr;

  u8g2.setFont(fontC);

  dW = u8g2.getDisplayWidth();
  sW = u8g2.getStrWidth(buf);

  x = (dW - sW) / 2;
  y = 64;
  u8g2.setCursor(x, y);

  // u8g2.print(buf);

  x = 0;
  y = 64;
  u8g2.setCursor(x, y);
  u8g2.print(sholatNameStr(NEXTTIMEID));

  snprintf(buf, sizeof(buf), "%dh %2dm", ceilHOUR, ceilMINUTE);
  dW = u8g2.getDisplayWidth();
  sW = u8g2.getStrWidth(buf);
  x = (dW - sW);
  y = 64;
  u8g2.setCursor(x, y);
  u8g2.print(buf);
}

void u8g2_string6(uint16_t a)
{
  tm *tm = localtime(&now);

  uint8_t hr, min /*, sec*/;
  hr = tm->tm_hour;
  min = tm->tm_min;
  // sec = tm->tm_sec;

  char buf[36];

  const uint8_t *fontC;

  // fontC = u8g2_font_6x10_mn;
  // fontC = u8g2_font_smart_patrol_nbp_tn;
  // fontC = u8g2_font_profont15_mn;
  // fontC = u8g2_font_px437wyse700b_mn;
  // fontC = u8g2_font_profont17_mn;
  // fontC = u8g2_font_t0_18_me;
  // fontC = u8g2_font_balthasar_titling_nbp_tn;
  // fontC = u8g2_font_logisoso16_tn;
  fontC = u8g2_font_logisoso42_tn;
  // fontC = u8g2_font_logisoso46_tn;
  // fontC = u8g2_font_inb33_mn;
  // fontC = u8g2_font_inr33_mn;

  u8g2.setFont(fontC);

  bool format12 = true;
  if (format12)
  {
    if (hr == 0)
    {
      hr = 12;
    }

    if (hr > 12)
    {
      hr = hr - 12;
    }
  }

  int dW;
  // int dH;
  // const char *str;
  int sW;
  // int ascent;

  int x, y;

  dW = u8g2.getDisplayWidth();
  snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
  sW = u8g2.getStrWidth(buf);

  u8g2.setFontPosTop();

  if (buf[0] == char(49)) // number 1
  {
    x = (dW - sW - 8) / 2;
    y = 0;
  }
  else
  {
    x = (dW - sW) / 2;
    y = 0;
  }

  u8g2.setCursor(x + a, y);

  u8g2.print(buf);

  bool blinkColon = true;
  if (blinkColon)
  {
    if (!state500ms)
    {
      u8g2.setCursor(x + a, y);
      snprintf(buf, sizeof(buf), "%d", hr);
      u8g2.print(buf);
      u8g2.setFontMode(1);
      u8g2.setDrawColor(0);
      u8g2.print(":");
      u8g2.setFontMode(1);  // default
      u8g2.setDrawColor(1); // default
    }
  }

  // snprintf(buf, sizeof(buf), "%s %s", sholatNameStr(NEXTTIMEID), sholatTimeArray[NEXTTIMEID]);

  if (!state500ms)
  {
    snprintf(buf, sizeof(buf), "%s  %dh %2dm", sholatNameStr(NEXTTIMEID), ceilHOUR, ceilMINUTE);
  }
  else
  {
    snprintf(buf, sizeof(buf), "%s  %dh %2dm", sholatNameStr(NEXTTIMEID), ceilHOUR, ceilMINUTE);
  }

  u8g2.setFontPosBottom();

  // fontC = u8g2_font_smart_patrol_nbp_tr;
  // fontC = u8g2_font_8x13_tr;
  fontC = u8g2_font_7x13_tr;

  u8g2.setFont(fontC);

  dW = u8g2.getDisplayWidth();
  sW = u8g2.getStrWidth(buf);

  x = (dW - sW) / 2;
  y = 64;
  u8g2.setCursor(x, y);

  // u8g2.print(buf);

  x = 0;
  y = 64;
  u8g2.setCursor(x, y);
  u8g2.print(sholatNameStr(NEXTTIMEID));

  snprintf(buf, sizeof(buf), "%dh %2dm", ceilHOUR, ceilMINUTE);
  dW = u8g2.getDisplayWidth();
  sW = u8g2.getStrWidth(buf);
  x = (dW - sW);
  y = 64;
  u8g2.setCursor(x, y);
  u8g2.print(buf);
}

void u8g2_string7(uint16_t a)
{
  tm *tm = localtime(&now);

  uint8_t hr, min /*, sec*/;
  hr = tm->tm_hour;
  min = tm->tm_min;
  // sec = tm->tm_sec;

  char buf[36];

  const uint8_t *fontC;

  // fontC = u8g2_font_6x10_mn;
  fontC = u8g2_font_smart_patrol_nbp_tn;
  // fontC = u8g2_font_profont15_mn;
  // fontC = u8g2_font_px437wyse700b_mn;
  // fontC = u8g2_font_profont17_mn;
  // fontC = u8g2_font_t0_18_me;
  // fontC = u8g2_font_balthasar_titling_nbp_tn;
  // fontC = u8g2_font_logisoso16_tn;
  // fontC = u8g2_font_logisoso42_tn;
  // fontC = u8g2_font_logisoso46_tn;
  // fontC = u8g2_font_inb33_mn;
  // fontC = u8g2_font_inr33_mn;

  u8g2.setFont(fontC);

  bool format12 = true;
  if (format12)
  {
    if (hr == 0)
    {
      hr = 12;
    }

    if (hr > 12)
    {
      hr = hr - 12;
    }
  }

  int dW;
  // int dH;
  // const char *str;
  int sW;
  // int ascent;

  int x, y;

  dW = u8g2.getDisplayWidth();
  snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
  sW = u8g2.getStrWidth(buf);

  u8g2.setFontPosTop();

  x = (dW - sW);
  y = 0;

  u8g2.setCursor(x + 0, y);

  u8g2.print(buf);

  // u8g2_2.setFontMode(0);  // default
  // u8g2_2.setDrawColor(1); // default
  u8g2_2.setFontPosTop();
  u8g2_2.setFont(fontC);
  u8g2_2.setCursor(x - 0, y);
  snprintf(buf, sizeof(buf), "963");
  // u8g2_2.print(buf);

  bool blinkColon = true;
  if (blinkColon)
  {
    if (!state500ms)
    {
      u8g2.setCursor(x + 0, y);
      snprintf(buf, sizeof(buf), "%d", hr);
      u8g2.print(buf);
      u8g2.setFontMode(1);
      u8g2.setDrawColor(0);
      u8g2.print(":");
      u8g2.setFontMode(1);  // default
      u8g2.setDrawColor(1); // default
    }
  }

  // print next sholat time
  u8g2.setFontPosTop();
  const uint8_t *font;
  // font = u8g2_font_logisoso22_tn;
  // font = u8g2_font_inb21_mr;
  font = u8g2_font_profont29_mn;
  // font = u8g2_font_freedoomr25_mn;
  u8g2.setFont(font);

  bool blinkNegativeSign = true;
  if (blinkNegativeSign)
  {
    if (!state500ms)
    {
      if (HOUR != 0)
      {
        snprintf(buf, sizeof(buf), " %d:%02d:%02d", HOUR, MINUTE, SECOND);
      }
      else
      {
        snprintf(buf, sizeof(buf), " %d:%02d", MINUTE, SECOND);
      }
    }
    else
    {
      if (HOUR != 0)
      {
        snprintf(buf, sizeof(buf), "-%d:%02d:%02d", HOUR, MINUTE, SECOND);
      }
      else
      {
        snprintf(buf, sizeof(buf), "-%d:%02d", MINUTE, SECOND);
      }
    }
  }
  dW = u8g2.getDisplayWidth();
  sW = u8g2.getStrWidth(buf);
  x = (dW - sW) / 2;
  y = 16;
  u8g2.setCursor(x, y);
  u8g2.print(buf);

  // print next sholat name
  u8g2.setFontPosBottom();
  // font = u8g2_font_7x13_tr;
  font = u8g2_font_profont22_mr;
  u8g2.setFont(font);
  snprintf(buf, sizeof(buf), "to %s", sholatNameStr(NEXTTIMEID));
  dW = u8g2.getDisplayWidth();
  sW = u8g2.getStrWidth(buf);
  x = (dW - sW) / 2;
  y = 64;
  u8g2.setCursor(x, y);
  u8g2.print(buf);
}

void u8g2_string8(uint16_t a)
{
  tm *tm = localtime(&now);

  uint8_t hr, min /*, sec*/;
  hr = tm->tm_hour;
  min = tm->tm_min;
  // sec = tm->tm_sec;

  char buf[36];

  const uint8_t *font;

  // font = u8g2_font_6x10_mn;
  font = u8g2_font_smart_patrol_nbp_tn;
  // font = u8g2_font_profont15_mn;
  // font = u8g2_font_px437wyse700b_mn;
  // font = u8g2_font_profont17_mn;
  // font = u8g2_font_t0_18_me;
  // font = u8g2_font_balthasar_titling_nbp_tn;
  // font = u8g2_font_logisoso16_tn;
  // font = u8g2_font_logisoso42_tn;
  // font = u8g2_font_logisoso46_tn;
  // font = u8g2_font_inb33_mn;
  // font = u8g2_font_inr33_mn;
  font = u8g2_font_freedoomr10_mu;

  u8g2.setFont(font);

  bool format12 = false;
  if (format12)
  {
    if (hr == 0)
    {
      hr = 12;
    }

    if (hr > 12)
    {
      hr = hr - 12;
    }
  }

  int dW;
  // int dH;
  // const char *str;
  int sW;
  // int ascent;

  int x, y;

  dW = u8g2.getDisplayWidth();
  snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
  sW = u8g2.getStrWidth(buf);

  u8g2.setFontPosTop();

  x = (dW - sW);
  y = 0;

  u8g2.setCursor(x + 0, y);

  u8g2.print(buf);

  // u8g2_2.setFontMode(0);  // default
  // u8g2_2.setDrawColor(1); // default
  u8g2_2.setFontPosTop();
  u8g2_2.setFont(font);
  u8g2_2.setCursor(x - 0, y);
  snprintf(buf, sizeof(buf), "963");
  // u8g2_2.print(buf);

  bool blinkColon = true;
  if (blinkColon)
  {
    if (!state500ms)
    {
      u8g2.setCursor(x + 0, y);
      snprintf(buf, sizeof(buf), "%d", hr);
      u8g2.print(buf);
      u8g2.setFontMode(1);
      u8g2.setDrawColor(0);
      u8g2.print(":");
      u8g2.setFontMode(1);  // default
      u8g2.setDrawColor(1); // default
    }
  }

  // print next sholat time
  u8g2.setFontPosTop();
  // font = u8g2_font_logisoso22_tn;
  // font = u8g2_font_inb21_mr;
  font = u8g2_font_profont29_mr;
  // font = u8g2_font_freedoomr25_mn;
  u8g2.setFont(font);

  snprintf(buf, sizeof(buf), "%dh %dm", ceilHOUR, ceilMINUTE);
  dW = u8g2.getDisplayWidth();
  sW = u8g2.getStrWidth(buf);
  x = (dW - sW) / 2;
  y = 16;
  u8g2.setCursor(x, y);
  u8g2.print(buf);

  // print next sholat name
  u8g2.setFontPosBottom();
  // font = u8g2_font_7x13_tr;
  font = u8g2_font_profont22_mr;
  u8g2.setFont(font);
  snprintf(buf, sizeof(buf), "to %s", sholatNameStr(NEXTTIMEID));
  dW = u8g2.getDisplayWidth();
  sW = u8g2.getStrWidth(buf);
  x = (dW - sW) / 2;
  y = 64;
  u8g2.setCursor(x, y);
  u8g2.print(buf);
}

void u8g2_string9(uint16_t a)
{
  tm *tm = localtime(&now);

  uint8_t hr, min /*, sec*/;
  hr = tm->tm_hour;
  min = tm->tm_min;
  // sec = tm->tm_sec;

  char buf[36];

  const uint8_t *font;

  // font = u8g2_font_6x10_mn;
  // font = u8g2_font_smart_patrol_nbp_tn;
  // font = u8g2_font_profont15_mn;
  // font = u8g2_font_px437wyse700b_mn;
  // font = u8g2_font_profont17_mn;
  // font = u8g2_font_t0_18_me;
  // font = u8g2_font_balthasar_titling_nbp_tn;
  // font = u8g2_font_logisoso16_tn;
  // font = u8g2_font_logisoso42_tn;
  // font = u8g2_font_logisoso46_tn;
  // font = u8g2_font_inb33_mn;
  // font = u8g2_font_inr33_mn;
  font = u8g2_font_freedoomr10_mu;

  u8g2.setFont(font);

  bool format12 = false;
  if (format12)
  {
    if (hr == 0)
    {
      hr = 12;
    }

    if (hr > 12)
    {
      hr = hr - 12;
    }
  }

  int dW;
  // int dH;
  // const char *str;
  int sW;
  // int ascent;

  int x, y;

  dW = u8g2.getDisplayWidth();
  snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
  sW = u8g2.getStrWidth(buf);

  u8g2.setFontPosTop();

  x = (dW - sW);
  y = 0;

  u8g2.setCursor(x + 0, y);

  u8g2.print(buf);

  // u8g2_2.setFontMode(0);  // default
  // u8g2_2.setDrawColor(1); // default
  u8g2_2.setFontPosTop();
  u8g2_2.setFont(font);
  u8g2_2.setCursor(x - 0, y);
  snprintf(buf, sizeof(buf), "963");
  // u8g2_2.print(buf);

  bool blinkColon = true;
  if (blinkColon)
  {
    if (!state500ms)
    {
      u8g2.setCursor(x + 0, y);
      snprintf(buf, sizeof(buf), "%d", hr);
      u8g2.print(buf);
      u8g2.setFontMode(1);
      u8g2.setDrawColor(0);
      u8g2.print(":");
      u8g2.setFontMode(1);  // default
      u8g2.setDrawColor(1); // default
    }
  }

  // print abbreviated weekday name
  u8g2_2.setFontPosTop();
  font = u8g2_font_smart_patrol_nbp_tr;
  u8g2.setFont(font);
  strftime(buf, sizeof(buf), "%a", tm);
  dW = u8g2.getDisplayWidth();
  sW = u8g2.getStrWidth(buf);
  x = (dW - sW);
  y = u8g2.getAscent() + 6;
  u8g2.setCursor(x, y);
  u8g2.print(buf);

  // print next sholat time
  u8g2.setFontPosTop();
  // font = u8g2_font_logisoso22_tn;
  // font = u8g2_font_inb21_mr;
  font = u8g2_font_profont22_mr;
  // font = u8g2_font_freedoomr25_mn;
  // font = u8g2_font_osb21_tr;
  u8g2.setFont(font);

  snprintf(buf, sizeof(buf), "%2d hr\r\n", ceilHOUR);
  dW = u8g2.getDisplayWidth();
  sW = u8g2.getStrWidth(buf);
  x = 0;
  y = 0;
  u8g2.setCursor(x, y);
  u8g2.print(buf);
  snprintf(buf, sizeof(buf), "%2d min\r\n", ceilMINUTE);
  x = 0;
  y = 21;
  u8g2.setCursor(x, y);
  u8g2.print(buf);

  // print next sholat name
  u8g2.setFontPosBottom();
  // font = u8g2_font_7x13_tr;
  font = u8g2_font_profont22_mr;
  u8g2.setFont(font);
  snprintf(buf, sizeof(buf), "to %s", sholatNameStr(NEXTTIMEID));
  dW = u8g2.getDisplayWidth();
  sW = u8g2.getStrWidth(buf);
  x = 0;
  y = 63;
  u8g2.setCursor(x, y);
  u8g2.print(buf);
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

// void draw(void)
// {
//   u8g2_prepare();
//   switch (draw_state >> 3)
//   {
//   case 0:
//     u8g2_box_frame(draw_state & 7);
//     break;
//   case 1:
//     u8g2_disc_circle(draw_state & 7);
//     break;
//   case 2:
//     u8g2_r_frame(draw_state & 7);
//     break;
//   case 3:
//     u8g2_string(draw_state & 7);
//     break;
//   case 4:
//     u8g2_line(draw_state & 7);
//     break;
//   case 5:
//     u8g2_triangle(draw_state & 7);
//     break;
//   case 6:
//     u8g2_ascii_1();
//     break;
//   case 7:
//     u8g2_ascii_2();
//     break;
//   case 8:
//     u8g2_extra_page(draw_state & 7);
//     break;
//   case 9:
//     u8g2_bitmap_modes(0);
//     break;
//   case 10:
//     u8g2_bitmap_modes(1);
//     break;
//   case 11:
//     u8g2_bitmap_overlay(draw_state & 7);
//     break;
//   }
// }

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

int I2C_ClearBus()
{
#if defined(TWCR) && defined(TWEN)
  TWCR &= ~(_BV(TWEN)); //Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
#endif

  pinMode(SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
  pinMode(SCL, INPUT_PULLUP);

  // Wait 2.5 secs, i.e. delay(2500). This is strictly only necessary on the first power
  // up of the DS3231 module to allow it to initialize properly,
  // but is also assists in reliable programming of FioV3 boards as it gives the
  // IDE a chance to start uploaded the program
  // before existing sketch confuses the IDE by sending Serial data.
  Serial.println(F("Delay 2.5 secs to allow DS3231 module to initialize properly"));
  delay(2500);
  boolean SCL_LOW = (digitalRead(SCL) == LOW); // Check is SCL is Low.
  if (SCL_LOW)
  {           //If it is held low Arduno cannot become the I2C master.
    return 1; //I2C bus error. Could not clear SCL clock line held low
  }

  boolean SDA_LOW = (digitalRead(SDA) == LOW); // vi. Check SDA input.
  int clockCount = 20;                         // > 2x9 clock

  while (SDA_LOW && (clockCount > 0))
  { //  vii. If SDA is Low,
    clockCount--;
    // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
    pinMode(SCL, INPUT);        // release SCL pullup so that when made output it will be LOW
    pinMode(SCL, OUTPUT);       // then clock SCL Low
    delayMicroseconds(10);      //  for >5uS
    pinMode(SCL, INPUT);        // release SCL LOW
    pinMode(SCL, INPUT_PULLUP); // turn on pullup resistors again
    // do not force high as slave may be holding it low for clock stretching.
    delayMicroseconds(10); //  for >5uS
    // The >5uS is so that even the slowest I2C devices are handled.
    SCL_LOW = (digitalRead(SCL) == LOW); // Check if SCL is Low.
    int counter = 20;
    while (SCL_LOW && (counter > 0))
    { //  loop waiting for SCL to become High only wait 2sec.
      counter--;
      delay(100);
      SCL_LOW = (digitalRead(SCL) == LOW);
    }
    if (SCL_LOW)
    {           // still low after 2 sec error
      return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
    }
    SDA_LOW = (digitalRead(SDA) == LOW); //   and check SDA input again and loop
  }
  if (SDA_LOW)
  {           // still low
    return 3; // I2C bus error. Could not clear. SDA data line held low
  }

  // else pull SDA line low for Start or Repeated Start
  pinMode(SDA, INPUT);  // remove pullup.
  pinMode(SDA, OUTPUT); // and then make it LOW i.e. send an I2C Start or Repeated start control.
  // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
  /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
  delayMicroseconds(10);      // wait >5uS
  pinMode(SDA, INPUT);        // remove output low
  pinMode(SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
  delayMicroseconds(10);      // x. wait >5uS
  pinMode(SDA, INPUT);        // and reset pins as tri-state inputs which is the default state on reset
  pinMode(SCL, INPUT);
  return 0; // all ok
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());

  settimeofday_cb(time_is_set);

  // configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  // configTime(0, 0, "192.168.10.1", "pool.ntp.org");
  configTime(0, 0, "pool.ntp.org", "192.168.10.1");
  // setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 3);
  // tzset();
  configTZ(TZ_Asia_Jakarta);
  // configTZ(TZ_Asia_Kathmandu);

  // -------------------------------------------------------------------
  // Setup I2C stuffs
  // -------------------------------------------------------------------
  Serial.println(F("Executing I2C_ClearBus()")); //http://www.forward.com.au/pfod/ArduinoProgramming/I2C_ClearBus/index.html
  int rtn = I2C_ClearBus();                      // clear the I2C bus first before calling Wire.begin()
  if (rtn != 0)
  {
    Serial.println(F("I2C bus error. Could not clear"));
    if (rtn == 1)
    {
      Serial.println(F("SCL clock line held low"));
    }
    else if (rtn == 2)
    {
      Serial.println(F("SCL clock line held low by slave clock stretch"));
    }
    else if (rtn == 3)
    {
      Serial.println(F("SDA data line held low"));
    }
  }
  else
  { // bus clear
    // re-enable Wire
    // now can start Wire Arduino master
    Serial.println(F("bus clear, re-enable Wire.begin();"));

    //Wire.begin();
    Wire.begin(SDA, SCL);

    //Set clock to 450kHz
    //Wire.setClock(450000L);
  }

  // Wire.begin(SDA, SCL);
  u8g2_2.begin();
  u8g2_2.clear();
  u8g2.begin();
  u8g2.clear();

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

void FlipState500ms()
{
  state500ms = !state500ms;
}

void loop()
{
  static uint32_t timer500ms = 0;
  if (millis() >= timer500ms + 500)
  {
    timer500ms = millis();
    // state500ms = !state500ms;
  }

  static uint32_t timer100ms = 0;
  if (millis() >= timer100ms + 100)
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

  gettimeofday(&tv, nullptr);
  clock_gettime(0, &tp);
  // now = time(nullptr);
  now = tv.tv_sec;
  now_ms = millis();
  now_us = micros();

  // localtime / gmtime every second change
  static time_t lastv = 0;
  if (lastv != tv.tv_sec)
  {
    lastv = tv.tv_sec;

    state500ms = true;
    state500msTimer.once(0.5, FlipState500ms);

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
    Serial.print(" time_t:");
    Serial.print(now);
    Serial.print(" time uint32_t:");
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

  // update sholat must be below / after updating the timestamp
  if (updateSholat)
  {
    updateSholat = false;
    // now = time(nullptr);
    // process_sholat();
    // process_sholat_2nd_stage();
    ProcessSholatEverySecond();
  }

  if (tick100ms)
  {
    // gettimeofday(&tv, nullptr);
    // clock_gettime(0, &tp);
    // now = time(nullptr);
    // now_ms = millis();
    // now_us = micros();
  }

  static bool state500ms_old = 0;
  bool tick500ms = false;
  if (state500ms_old != state500ms)
  {
    state500ms_old = state500ms;
    tick500ms = true;
  }

  //update the display every 1000ms
  if (tick1000ms)
  {
    ProcessSholatEverySecond();
  }

  if (tick500ms)
  {
    // picture loop
    // must be after sholat time has been updated
    // u8g2.clearBuffer();
    u8g2_2.clearBuffer();

    static uint16_t x = 0;
    // u8g2_prepare();
    // u8g2_string(x);
    // u8g2_string2(x);
    // u8g2_string3(x);
    // u8g2_string4(x);
    // u8g2_string5(x);
    // u8g2_string6(0);
    // u8g2_string7(0);
    // u8g2_string8(0);
    u8g2_string9(0);
    x = x - 84;
    if (x <= -127)
    {
      x = 0;
    }

    // u8g2_2.setFontMode(0);  // default
    // u8g2_2.setDrawColor(1); // default
    // u8g2_2.setFontPosTop();
    // u8g2_2.setFont(u8g2_font_logisoso42_tn);
    // u8g2_2.setCursor(0,0);
    // u8g2_2.print("AAA");

    // u8g2.sendBuffer();
    u8g2_2.sendBuffer();
  }

  tick100ms = false;
  tick1000ms = false;
}
