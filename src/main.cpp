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


#include <stdint.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "PrayerTimes.h"
#include <Ticker.h>


// Include the correct display library
// For a connection via I2C using Wire include
// #include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
// #include "SH1106.h" // alis for `#include "SH1106Wire.h"`
// For a connection via I2C using brzo_i2c (must be installed) include
#include <brzo_i2c.h> // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Brzo.h"
#include "SH1106Brzo.h"
// For a connection via SPI include
// #include <SPI.h> // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Spi.h"
// #include "SH1106SPi.h"

// Include the UI lib
#include "OLEDDisplayUi.h"

// Include custom images
#include "images.h"

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
SH1106Brzo  display(0x3c, D3, D5);

// Initialize the OLED display using Wire library
// SSD1306  display(0x3c, D3, D5);
// SH1106 display(0x3c, D3, D5);

OLEDDisplayUi ui ( &display );

int screenW = 128;
int screenH = 64;
int clockCenterX = screenW / 2;
int clockCenterY = ((screenH - 16) / 2) + 16; // top yellow part is 16 px height
int clockRadius = 23;


const char ssid[] = "galaxi";  //  your network SSID (name)
const char pass[] = "n1n4iqb4l";       // your network password

// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

const int timeZone = 7;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)


WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

// utility function for digital clock display: prints leading 0
String twoDigits(int digits) {
  if (digits < 10) {
    String i = '0' + String(digits);
    return i;
  }
  else {
    return String(digits);
  }
}

void clockOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {

}

void analogClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  //  ui.disableIndicator();

  // Draw the clock face
  //  display->drawCircle(clockCenterX + x, clockCenterY + y, clockRadius);
  display->drawCircle(clockCenterX + x, clockCenterY + y, 2);
  //
  //hour ticks
  for ( int z = 0; z < 360; z = z + 30 ) {
    //Begin at 0° and stop at 360°
    float angle = z ;
    angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
    int x2 = ( clockCenterX + ( sin(angle) * clockRadius ) );
    int y2 = ( clockCenterY - ( cos(angle) * clockRadius ) );
    int x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 8 ) ) ) );
    int y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 8 ) ) ) );
    display->drawLine( x2 + x , y2 + y , x3 + x , y3 + y);
  }

  // display second hand
  float angle = second() * 6 ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  int x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 5 ) ) ) );
  int y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 5 ) ) ) );
  display->drawLine( clockCenterX + x , clockCenterY + y , x3 + x , y3 + y);
  //
  // display minute hand
  angle = minute() * 6 ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 4 ) ) ) );
  y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 4 ) ) ) );
  display->drawLine( clockCenterX + x , clockCenterY + y , x3 + x , y3 + y);
  //
  // display hour hand
  angle = hour() * 30 + int( ( minute() / 12 ) * 6 )   ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 2 ) ) ) );
  y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 2 ) ) ) );
  display->drawLine( clockCenterX + x , clockCenterY + y , x3 + x , y3 + y);
}

void digitalClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String timenow = String(hour()) + ":" + twoDigits(minute()) + ":" + twoDigits(second());
  //String stringTextTimeNow = String("Time now");
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  //display->drawString(64 + x , 16 + y, stringTextTimeNow );
  //display->drawString(clockCenterX + x , clockCenterY + y, timenow );
  display->drawString(clockCenterX + x , 24, timenow );
  //display->drawStringMaxWidth(clockCenterX + x , 24, 128, timenow );
}

int HOUR;
int MINUTE;
String CURRENTTIMENAME;
String NEXTTIMENAME;
void timeLeft(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String stringTimeLeft = String(HOUR) + " " + "h" + + " " + MINUTE + " " + "m";
  String stringNextTimeName = String("to") + " " + NEXTTIMENAME;
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(64 + x , 14 + y, stringTimeLeft );
  display->drawString(64 + x , 37 + y, stringNextTimeName );
}

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  String timenow = String(hour()) + ":" + twoDigits(minute());
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_16);
  //display->drawString(128, 0, String(millis()));
  display->drawString(128, 0, timenow);
}

// This array keeps function pointers to all frames
// frames are the single views that slide in
//FrameCallback frames[] = { analogClockFrame, digitalClockFrame };
FrameCallback frames[] = { timeLeft, digitalClockFrame };

// how many frames are there?
int frameCount = 2;

// Overlays are statically drawn on top of a frame eg. a clock
//OverlayCallback overlays[] = { clockOverlay };
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

double times[sizeof(TimeName) / sizeof(char*)];

//void p(char *fmt, ... ) {
void p(const char *fmt, ... ) {
  char tmp[128]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(tmp, 128, fmt, args);
  va_end (args);
  Serial.print(tmp);
}


Ticker ticker;

void check_time_status() {
  if (timeStatus() == timeNotSet) {
    Serial.println("TIME NOT SET");
  }
  else if (timeStatus() == timeNeedsSync) {
    Serial.println("TIME NEED SYNC");
  }
  else if (timeStatus() == timeSet) {
    Serial.println("TIME SET");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  //while (!Serial) ; // Needed for Leonardo only
  delay(250);
  Serial.println("TimeNTP Example");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);

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

  //  unsigned long secsSinceStart = millis();
  //  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  //  const unsigned long seventyYears = 2208988800UL;
  //  // subtract seventy years:
  //  unsigned long epoch = secsSinceStart - seventyYears * SECS_PER_HOUR;
  //  setTime(epoch);

  // flip the pin every 0.3s
  ticker.attach(5, check_time_status);

}


time_t prevDisplay = 0; // when the digital clock was displayed

void loop() {
  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.

    if (timeStatus() != timeNotSet) {
      if (now() != prevDisplay) { //update the display only if time has changed
        prevDisplay = now();
        digitalClockDisplay();


        int timezone = 7;

        set_calc_method(Custom);
        set_asr_method(Shafii);
        set_high_lats_adjust_method(AngleBased);
        set_fajr_angle(20);
        set_isha_angle(18);

        //BEKASI
        float latitude = -6.26526191;
        float longitude = 106.97298512;
        //get prayer time on the current year, month and day
        get_prayer_times(year(), month(), day(), latitude, longitude, timezone, times);

        for (int i = 0; i < sizeof(times) / sizeof(double); i++) {
          char tmp[10];
          int hours, minutes;
          get_float_time_parts(times[i], hours, minutes);
          p("%d \t %10s %s \t %02d:%02d \n\r", i, TimeName[i], dtostrf(times[i], 2, 2, tmp), hours, minutes);
        }
        
        for (int i = 0; i < sizeof(times) / sizeof(double); i++) {

          int CURRENTTIMEID, NEXTTIMEID;
          float timeNow = hour() / 1.0 + minute() / 60.0;
          float timeA, timeB;
          int hours, minutes;
          int hoursNext, minutesNext;
          get_float_time_parts(times[i], hours, minutes);
          timeA = times[i];
          //timeA = times[i] + 2.0 / 60;
          CURRENTTIMEID = i;
          if (CURRENTTIMEID == 4) {
            CURRENTTIMEID = 3;
          }
          if (i != 6) {
            get_float_time_parts(times[i + 1], hoursNext, minutesNext);
            timeB = times[i + 1];
            //timeB = times[i + 1]  +  2.0 / 60;
            NEXTTIMEID = i + 1;
            if (NEXTTIMEID == 4) {
              NEXTTIMEID = 5;
            }
          }
          else if (i == 6) {
            get_float_time_parts(times[0], hoursNext, minutesNext);
            timeB = times[0];
            //timeB = times[0] + 2.0 / 60;
            NEXTTIMEID = 0;
          }

          if (timeB > timeA) {
            Serial.println("case A");
            if (timeA < timeNow && timeNow < timeB) {
              Serial.println("case A1");
              CURRENTTIMENAME = TimeName[CURRENTTIMEID];
              NEXTTIMENAME = TimeName[NEXTTIMEID];

              //extract hour and minute from time difference
              get_float_time_parts(time_diff(timeNow, timeB), HOUR, MINUTE);

              break;
            }
          }

          else if (timeB < timeA) {
            Serial.println("case B");
            if ( (timeA < timeNow && timeNow < 24) || (0 <= timeNow && timeNow < timeB) ) {
              Serial.println("case B1");
              CURRENTTIMENAME = TimeName[CURRENTTIMEID];
              NEXTTIMENAME = TimeName[NEXTTIMEID];

              //extract hour and minute from time difference
              get_float_time_parts(time_diff(timeNow, timeB), HOUR, MINUTE);

              break;
            }
          }
        }//end of for loop        
      }
    }
    delay(remainingTimeBudget);
  }
}

void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress & address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}


//float (x) {
//  float t;
//
//  if (!isfinite(x))
//    return (x);
//
//  if (x >= 0.0) {
//    t = floor(x);
//    if (t - x <= -0.5)
//      t += 1.0;
//    return (t);
//  } else {
//    t = floor(-x);
//    if (t + x <= -0.5)
//      t += 1.0;
//    return (-t);
//  }
//}
