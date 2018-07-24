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
#include "rtchelper.h"
#include "displayhelper.h"
#include "asyncserver.h"
// #include <Ticker.h>
#include <StreamString.h>
#include "PingAlive.h"

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
// #define LED ESP_PIN_2

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

#define DEBUGPORT Serial

// #define RELEASE

#ifndef RELEASE
#define DEBUGLOG(fmt, ...)                   \
  {                                          \
    static const char pfmt[] PROGMEM = fmt;  \
    DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
  }
#define DEBUGLOGLN(fmt, ...)                 \
  {                                          \
    static const char pfmt[] PROGMEM = fmt;  \
    static const char rn[] PROGMEM = "\r\n"; \
    DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
    DEBUGPORT.printf_P(rn);                  \
  }
#else
#define DEBUGLOG(...)
#define DEBUGLOGLN(...)
#endif

bool syncNtpToRtcFlag = false;
// bool tick100ms = false;
bool tick3000ms = false;

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



  TimeSetup();

  // WiFi.mode(WIFI_OFF);
  // WiFi.mode(WIFI_STA);
  // WiFi.begin("your_wifi_ssid", "your_wifi_password");

  AsyncWSBegin();

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  DEBUGLOGLN("IP number assigned by DHCP is %s", WiFi.localIP().toString().c_str());

  // -------------------------------------------------------------------
  // Setup I2C stuffs
  // -------------------------------------------------------------------
  DEBUGLOGLN("Clearing I2C Bus"); //http://www.forward.com.au/pfod/ArduinoProgramming/I2C_ClearBus/index.html
  int rtn = I2C_ClearBus();       // clear the I2C bus first before calling Wire.begin()
  if (rtn != 0)
  {
    DEBUGLOGLN("I2C bus error. Could not clear");
    if (rtn == 1)
    {
      DEBUGLOGLN("SCL clock line held low");
    }
    else if (rtn == 2)
    {
      DEBUGLOGLN("SCL clock line held low by slave clock stretch");
    }
    else if (rtn == 3)
    {
      DEBUGLOGLN("SDA data line held low");
    }
  }
  else
  {
    DEBUGLOGLN("bus clear, re-enable Wire.begin();");
    Wire.begin(SDA, SCL);
  }

  DisplaySetup();

  RtcSetup();

  Serial.println(F("Setup completed\r\n"));
}

void pingFault(void) {}

void loop()
{
  static bool beginPingAlive = true;
  if (WiFi.status() == WL_CONNECTED && beginPingAlive)
  {
    startPingAlive();
    beginPingAlive = false;
  }

  static uint32_t timer3000ms = 0;
  if (millis() >= timer3000ms + 3000)
  {
    timer3000ms = millis();
    tick3000ms = true;
  }

  TimeLoop();

  // SholatLoop();

  static bool state500ms_old = 0;
  bool tick500ms = false;
  if (state500ms_old != state500ms)
  {
    state500ms_old = state500ms;
    tick500ms = true;
  }

  if(timeSetFlag && tick1000ms)
  {
    lastSync = now;
    process_sholat();
    process_sholat_2nd_stage();
  }

  if (tick1000ms)
  {
    // update sholat must be below / after updating the timestamp
    SholatLoop();
  }

  if (tick500ms)
  {
    DisplayLoop();
  }

  AsyncWSLoop();

  tick500ms = false;
  tick1000ms = false;
  timeSetFlag = false;
}
