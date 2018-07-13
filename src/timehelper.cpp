#include <Arduino.h>
#include <time.h>
#include "timehelper.h"
#include "sntphelper.h"
#include "rtchelper.h"

#define DEBUGPORT Serial

#define RELEASE

#ifndef RELEASE
#define PROGMEM_T __attribute__((section(".irom.text.template")))
#define DEBUGLOG(fmt, ...)                    \
  {                                           \
    static const char pfmt[] PROGMEM_T = fmt; \
    DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__);  \
  }
#else
#define DEBUGLOG(...)
#endif

bool tick1000ms = false;

bool NTP_OK = false;

time_t utcTime, localTime;
time_t _lastSyncd; ///< Stored time of last successful sync
time_t _firstSync; ///< Stored time of first successful sync after boot
time_t _lastBoot;

uint16_t syncInterval;  ///< Interval to set periodic time sync
uint16_t shortSyncInterval; ///< Interval to set periodic time sync until first synchronization.
uint16_t longSyncInterval;  ///< Interval to set periodic time sync


strConfigTime _configTime;
TIMESOURCE _timeSource;

char *getDateStr()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  static char buf[12];
  time_t rawtime;
  time(&rawtime);
  struct tm *timeinfo = localtime(&rawtime);
  strftime(buf, sizeof(buf), "%a %d %b %Y", timeinfo);

  return buf;
}

char *getTimeStr()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  static char buf[12];
  time_t rawtime;
  time(&rawtime);
  struct tm *timeinfo = localtime(&rawtime);
  strftime(buf, sizeof(buf), "%T", timeinfo);

  return buf;
}

char *getUptimeStr()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  //time_t uptime = utcTime - _lastBoot;
  time_t uptime = millis() / 1000;

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
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  time_t diff = time(nullptr) - _lastSyncd;

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
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  time_t nextsync;

  nextsync = _lastSyncd - utcTime + syncInterval;

  uint16_t days;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  struct tm *tm = gmtime(&nextsync); // convert to broken down time
  days = tm->tm_yday;
  hours = tm->tm_hour;
  minutes = tm->tm_min;
  seconds = tm->tm_sec;

  static char buf[30];
  snprintf(buf, sizeof(buf), "%u days %02d:%02d:%02d", days, hours, minutes, seconds);

  return buf;
}

void timeSetup()
{
  // Synchronize time useing SNTP. This is necessary to verify that
  // the TLS certificates offered by the server are currently valid.
  Serial.print("Setting time using SNTP");
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.println(asctime(&timeinfo));
}
