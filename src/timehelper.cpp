#include <Arduino.h>
#include <time.h>
#include "timehelper.h"
#include "rtchelper.h"
#include "EspGoodies.h"

#define DEBUGPORT Serial

#define RELEASE

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

bool tick1000ms = false;
bool state500ms = false;

uint32_t now_ms, now_us;
timeval tv;
timespec tp;
timeval cbtime; // time set in callback
bool timeSetFlag;

time_t now;
time_t uptime;
time_t lastSync;   ///< Stored time of last successful sync
time_t _firstSync; ///< Stored time of first successful sync after boot
time_t _lastBoot;

uint16_t syncInterval;      ///< Interval to set periodic time sync
uint16_t shortSyncInterval; ///< Interval to set periodic time sync until first synchronization.
uint16_t longSyncInterval;  ///< Interval to set periodic time sync

Ticker state500msTimer;

strConfigTime _configTime;
strTimeSource timeSource;

char *getDateStr()
{
  static char buf[12];
  time_t rawtime;
  time(&rawtime);
  struct tm *timeinfo = localtime(&rawtime);
  strftime(buf, sizeof(buf), "%a %d %b %Y", timeinfo);

  return buf;
}

char *getTimeStr()
{
  static char buf[12];
  time_t rawtime;
  time(&rawtime);
  struct tm *timeinfo = localtime(&rawtime);
  strftime(buf, sizeof(buf), "%T", timeinfo);

  return buf;
}

char *getDateTimeStr(time_t moment)
{
  static char buf[60];
  strftime(buf, (sizeof(buf) / sizeof(buf[0])), "%a %b %d %Y %X GMT", gmtime(&moment));
  return buf;
}

char *getLastBootStr()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  // time_t uptime = tp.tv_sec;
  time_t lastBoot = now - uptime;
  static char buf[30];
  struct tm *tm = localtime(&lastBoot);
  sprintf_P(buf, PSTR("%s"), asctime(tm));

  return buf;
}

char *getUptimeStr()
{
  //time_t uptime = utcTime - _lastBoot;

  uint16_t days;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  struct tm *tm = gmtime(&uptime); // convert to broken down time
  days = tm->tm_yday;
  hours = tm->tm_hour;
  minutes = tm->tm_min;
  seconds = tm->tm_sec;

  static char buf[30];
  snprintf(buf, sizeof(buf), "%u days %02d:%02d:%02d", days, hours, minutes, seconds);

  return buf;
}

char *getLastSyncStr()
{
  time_t diff = time(nullptr) - lastSync;

  uint16_t days;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  struct tm *tm = gmtime(&diff); // convert to broken down time
  days = tm->tm_yday;
  hours = tm->tm_hour;
  minutes = tm->tm_min;
  seconds = tm->tm_sec;

  static char buf[30];
  if (days > 0)
  {
    snprintf(buf, sizeof(buf), "%u day %d hr ago", days, hours);
  }
  else if (hours > 0)
  {
    snprintf(buf, sizeof(buf), "%d hr %d min ago", hours, minutes);
  }
  else if (minutes > 0)
  {
    snprintf(buf, sizeof(buf), "%d min ago", minutes);
  }
  else
  {
    snprintf(buf, sizeof(buf), "%d sec ago", seconds);
  }

  return buf;
}

char *getNextSyncStr()
{
  time_t _syncInterval = 3601;

  time_t diff = (lastSync + _syncInterval) - now;

  uint16_t days;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  struct tm *tm = gmtime(&diff); // convert to broken down time
  days = tm->tm_yday;
  hours = tm->tm_hour;
  minutes = tm->tm_min;
  seconds = tm->tm_sec;

  static char buf[30];
  snprintf(buf, sizeof(buf), "%u days %02d:%02d:%02d", days, hours, minutes, seconds);

  return buf;
}

char *GetRtcDateTimeStr(const RtcDateTime &dt)
{
  // output format: Sat Jul 21 2018 10:59:32
  static char buf[32];

  snprintf_P(buf,
             //countof(datestring),
             (sizeof(buf) / sizeof(buf[0])),
             PSTR("%s %s %d %d %02d:%02d:%02d"),

             dayShortStr(dt.DayOfWeek()),
             monthShortStr(dt.Month()),
             dt.Day(),
             dt.Year(),

             dt.Hour(),
             dt.Minute(),
             dt.Second());

  return buf;
}

void FlipState500ms()
{
  state500ms = !state500ms;
}

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

void time_is_set()
{
  gettimeofday(&cbtime, NULL);
  timeSetFlag = true;
  Serial.println(F("------------------ settimeofday() was called ------------------"));
}

void TimeSetup()
{
  settimeofday_cb(time_is_set);

  // Rtc.SetDateTime(4102444770);
  // time_t t = 2147483627; // 19 January 2038, 2147483647
  uint32_t t = 2147483627; // 19 January 2038, 2147483647
  // uint32_t t = 4294967275; //max32 = 4294967295;
  // uint64_t t = 4102444780; //2100, January 1 - 20 secs

  // time_t t = 2147483627;

  /*
  RtcDateTime timeToSetToRTC;
  timeToSetToRTC.InitWithEpoch64Time(t);
  Rtc.SetDateTime(timeToSetToRTC);
  */

  if (false)
  {
    // ESP.eraseConfig();
    // time_t rtc = RTC_TEST;
    // WiFi.mode(WIFI_OFF);
    uint32_t rtc = t;
    timeval tv = {rtc, 0};
    timezone tz = {0, 0};
    settimeofday(&tv, &tz);
  }
  else
  {
    // configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    // configTime(0, 0, "192.168.10.1", "pool.ntp.org");
    configTime(0, 0, "pool.ntp.org", "192.168.10.1");
    // setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 3);
    // tzset();
    configTZ(TZ_Asia_Jakarta);
    // configTZ(TZ_Asia_Kathmandu);
  }

  // while (now < 8 * 3600 * 2)
  // {
  //   delay(500);
  //   Serial.print(".");
  //   now = time(nullptr);
  // }

  // // Synchronize time useing SNTP. This is necessary to verify that
  // // the TLS certificates offered by the server are currently valid.
  // Serial.print("Setting time using SNTP");
  // configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  // time_t now = time(nullptr);
  // while (now < 8 * 3600 * 2)
  // {
  //   delay(500);
  //   Serial.print(".");
  //   now = time(nullptr);
  // }
  // Serial.println("");
  // struct tm timeinfo;
  // gmtime_r(&now, &timeinfo);
  // Serial.print("Current time: ");
  // Serial.println(asctime(&timeinfo));
}

void TimeLoop()
{
  gettimeofday(&tv, nullptr);
  clock_gettime(0, &tp);
  // now = time(nullptr);
  now = tv.tv_sec;
  uptime = tp.tv_sec;
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

#ifndef RELEASE
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

    //  RtcDateTime timeToSetToRTC;
    //  timeToSetToRTC.InitWithEpoch32Time(now);
    //  Rtc.SetDateTime(timeToSetToRTC);

    // human readable
    // Printed format: Wed Oct 05 2011 16:48:00 GMT+0200 (CEST)
    char buf[60];
    strftime(buf, sizeof(buf), "%a %b %d %Y %X GMT%z (%Z)", localtime(&now));
    DEBUGLOGLN("NTP LOCAL date/time: %s", buf);
    strftime(buf, sizeof(buf), "%a %b %d %Y %X GMT", gmtime(&now));
    DEBUGLOGLN("NTP GMT   date/time: %s", buf);

    RtcDateTime dt = Rtc.GetDateTime();
    DEBUGLOGLN("RTC GMT   date/time: %s\r\n", GetRtcDateTimeStr(dt));
#endif
  }
}
