#include <Arduino.h>
#include "timehelper.h"
#include "sntphelper.h"
#include "rtchelper.h"
#include <TimeLib.h>

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

uint16_t _shortInterval; ///< Interval to set periodic time sync until first synchronization.
uint16_t _longInterval;  ///< Interval to set periodic time sync

char bufCommon[30];

strConfigTime _configTime;
TIMESOURCE _timeSource;

char *getDateStr(time_t moment)
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  // char dateStr[12];
  // snprintf(bufCommon, sizeof(bufCommon), "%s, %d-%s-%d", dayShortStr(weekday(moment)), day(moment), monthShortStr(month(moment)), year(moment));
  snprintf(bufCommon, sizeof(bufCommon), "%s, %d-%s-%d", dayStr(weekday(moment)), day(moment), monthShortStr(month(moment)), year(moment));

  return bufCommon;
}

char *getTimeStr(time_t moment)
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  //char timeStr[10];
  //sprintf (timeStr, "%02d:%02d:%02d", hour(moment), minute(moment), second(moment));
  static char buf[10];
  snprintf(buf, sizeof(bufCommon), "%02d:%02d:%02d", hour(moment), minute(moment), second(moment));

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

  //  seconds = uptime % SECS_PER_MIN;
  //  uptime -= seconds;
  //  minutes = (uptime % SECS_PER_HOUR) / SECS_PER_MIN;
  //  uptime -= minutes * SECS_PER_MIN;
  //  hours = (uptime % SECS_PER_DAY) / SECS_PER_HOUR;
  //  uptime -= hours * SECS_PER_HOUR;
  //  days = uptime / SECS_PER_DAY;

  seconds = numberOfSeconds(uptime);
  uptime -= seconds;
  minutes = numberOfMinutes(uptime);
  uptime -= minutes * SECS_PER_MIN;
  //uptime -= minutesToTime_t;
  hours = numberOfHours(uptime);
  uptime -= hours * SECS_PER_HOUR;
  //uptime -= hoursToTime_t;
  days = elapsedDays(uptime);

  //char buf[30];
  snprintf(bufCommon, sizeof(bufCommon), "%u days %02d:%02d:%02d", days, hours, minutes, seconds);

  //  for (int i = 0; i < 30; i++) {
  //    bufCommon[i] = str[i];
  //  }
  //  bufCommon[30] = 0;

  return bufCommon;
}

char *getLastSyncStr()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  time_t uptime = utcTime - _lastSyncd;

  uint16_t days;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  seconds = numberOfSeconds(uptime);
  uptime -= seconds;
  minutes = numberOfMinutes(uptime);
  uptime -= minutes * SECS_PER_MIN;
  //uptime -= minutesToTime_t;
  hours = numberOfHours(uptime);
  uptime -= hours * SECS_PER_HOUR;
  //uptime -= hoursToTime_t;
  days = elapsedDays(uptime);

  //char str[30];
  if (days > 0)
  {
    snprintf(bufCommon, sizeof(bufCommon), "%u day %d hr ago", days, hours);
  }
  else if (hours > 0)
  {
    snprintf(bufCommon, sizeof(bufCommon), "%d hr %d min ago", hours, minutes);
  }
  else if (minutes > 0)
  {
    snprintf(bufCommon, sizeof(bufCommon), "%d min ago", minutes);
  }
  else
  {
    snprintf(bufCommon, sizeof(bufCommon), "%d sec ago", seconds);
  }

  return bufCommon;
}

char *getNextSyncStr()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  time_t nextsync;

  if (timeStatus() == timeSet)
  {
    nextsync = _lastSyncd - utcTime + getInterval();
  }
  else
  {
    nextsync = _lastSyncd - utcTime + getShortInterval();
  }

  uint16_t days;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  //  seconds = numberOfSeconds(countdown);
  //  countdown -= seconds;
  //  minutes = numberOfMinutes(countdown);
  //  countdown -= minutes * SECS_PER_MIN;
  //  //uptime -= minutesToTime_t;
  //  hours = numberOfHours(countdown);
  //  countdown -= hours * SECS_PER_HOUR;
  //  //uptime -= hoursToTime_t;
  days = elapsedDays(nextsync);
  hours = numberOfHours(nextsync);
  minutes = numberOfMinutes(nextsync);
  seconds = numberOfSeconds(nextsync);

  //char str[30];
  snprintf(bufCommon, sizeof(bufCommon), "%u days %02d:%02d:%02d", days, hours, minutes, seconds);

  return bufCommon;
}

uint16_t getInterval()
{
  return _longInterval;
}

uint16_t getShortInterval()
{
  return _shortInterval;
}

//https://forum.arduino.cc/index.php?topic=61529.msg2996010#msg2996010
time_t tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss)
{
  tmElements_t tmSet;
  tmSet.Year = YYYY - 1970;
  tmSet.Month = MM;
  tmSet.Day = DD;
  tmSet.Hour = hh;
  tmSet.Minute = mm;
  tmSet.Second = ss;
  return makeTime(tmSet);
}

void digitalClockDisplay()
{
  // digital clock display of the time
  time_t t = now() + _configLocation.timezone / 10.0 * 3600;

  Serial.print(year(t));
  Serial.print(F("-"));
  printDigits(month(t));
  Serial.print(F("-"));
  printDigits(day(t));
  Serial.print(F("T"));
  printDigits(hour(t));
  Serial.print(F(":"));
  printDigits(minute(t));
  Serial.print(F(":"));
  printDigits(second(t));
  //Serial.println();
}

void digitalClockDisplay(time_t t)
{
  Serial.print(year(t));
  Serial.print(F("-"));
  printDigits(month(t));
  Serial.print(F("-"));
  printDigits(day(t));
  Serial.print(F("T"));
  printDigits(hour(t));
  Serial.print(F(":"));
  printDigits(minute(t));
  Serial.print(F(":"));
  printDigits(second(t));
  //Serial.println();
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  //Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

byte PrintTimeStatus()
{
  Serial.print(F("Time status: "));

  byte currentTimeStatus = timeStatus();

  if (currentTimeStatus == timeNotSet)
  {
    Serial.println(F("TIME NOT SET"));
  }
  else if (currentTimeStatus == timeNeedsSync)
  {
    Serial.println(F("TIME NEED SYNC"));
  }
  else if (currentTimeStatus == timeSet)
  {
    Serial.println(F("TIME SET"));
  }
  return currentTimeStatus;
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

void timeLoop()
{
  //check, update and print time status if necessary
  static byte timeStatus_old = timeStatus();
  if (timeStatus() != timeStatus_old)
  {

    PrintTimeStatus();

    if (timeStatus() == timeSet)
    {
      if (_configTime.enablentp == true)
      {
        int longInterval = getInterval();
        setSyncInterval(longInterval);
        digitalClockDisplay();
        Serial.print(F("> "));
        Serial.print(F("Sync interval set to "));
        Serial.print(longInterval);
        Serial.println(F(" seconds"));
      }
    }
    else if (timeStatus() != timeSet)
    {
      if (_configTime.enablentp == true)
      {
        int shortInterval = getShortInterval();
        setSyncInterval(shortInterval);
        digitalClockDisplay();
        Serial.print(F("> "));
        Serial.print(F("Sync interval set to "));
        Serial.print(shortInterval);
        Serial.println(F(" seconds"));
      }
    }

    timeStatus_old = timeStatus();
  }

  if (syncNtpEventTriggered || syncRtcEventTriggered)
  {

    // store first sync and last sync
    _lastSyncd = now();

    if (_firstSync == 0)
    {
      if (timeStatus() == timeSet)
      {

        _firstSync = _lastSyncd;
        Serial.print(F("_firstSync: "));
        Serial.println(_firstSync);

        uint32_t _millisFirstSync = millis();
        Serial.print(F("millisFirstSync: "));
        Serial.println(_millisFirstSync);

        uint32_t _secondsFirstSync = _millisFirstSync / 1000;
        Serial.print(F("_secondsFirstSync: "));
        Serial.println(_secondsFirstSync);

        _lastBoot = _firstSync - _secondsFirstSync;
        Serial.print(F("_lastBoot: "));
        Serial.println(_lastBoot);
      }
    }

    if (syncNtpEventTriggered)
    {
      if (timeStatus() != timeSet)
      {
        digitalClockDisplay();
        Serial.print(F("> "));
        Serial.println(F("NTP has failed setting the time"));
      }
      else
      {
        digitalClockDisplay();
        Serial.print(F("> "));
        Serial.println(F("NTP has succesfully set the time"));
      }
    }

    if (syncRtcEventTriggered)
    {
      if (timeStatus() != timeSet)
      {
        digitalClockDisplay();
        Serial.print(F("> "));
        Serial.println(F("RTC has failed setting the time"));
      }
      else
      {
        digitalClockDisplay();
        Serial.print(F("> "));
        Serial.println(F("RTC has succesfully set the time"));
      }
    }

    syncNtpEventTriggered = false;
    syncRtcEventTriggered = false;
  }

  //Select time source
  static boolean NTP_OK_old = NTP_OK;
  static boolean RTC_OK_old = RTC_OK;

  if (NTP_OK != NTP_OK_old || RTC_OK != RTC_OK_old)
  {

    if (NTP_OK != NTP_OK_old)
    {
      if (NTP_OK)
      {
        digitalClockDisplay();
        Serial.println(F(">"));
        digitalClockDisplay();
        Serial.print(F("> "));
        Serial.println(F("NTP OK"));
      }
      else
      {
        digitalClockDisplay();
        Serial.println(F(">"));
        digitalClockDisplay();
        Serial.print(F("> "));
        Serial.println(F("NTP NOT OK"));
      }
      // update values
      NTP_OK_old = NTP_OK;
    }

    if (RTC_OK != RTC_OK_old)
    {
      if (RTC_OK)
      {
        digitalClockDisplay();
        Serial.println(F(">"));
        digitalClockDisplay();
        Serial.print(F("> "));
        Serial.println(F("RTC OK"));
      }
      else
      {
        digitalClockDisplay();
        Serial.println(F(">"));
        digitalClockDisplay();
        Serial.print(F("> "));
        Serial.println(F("RTC NOT OK"));
      }
      // update values
      RTC_OK_old = RTC_OK;
    }

    digitalClockDisplay();
    Serial.println(F("> "));
    digitalClockDisplay();
    Serial.print(F("> "));
    Serial.print(F("Update selected time source... "));
    if (RTC_OK == true)
    {
      Serial.println(F("RTC"));
      _timeSource = TIMESOURCE_RTC;
    }
    else if (NTP_OK == true && RTC_OK == false)
    {
      Serial.println(F("NTP"));
      _timeSource = TIMESOURCE_NTP;
      // Serial.println(F(", but DONE NOTHING :("));
    }
    // else if (NTP_OK == false && RTC_OK == false) {
    else
    {
      Serial.println(F("*** NO VALID TIME SOURCE IS AVAILABLE ***"));
      _timeSource = TIMESOURCE_NOT_AVAILABLE;
    }
    Serial.println();
  }

  static byte _timeSource_old = _timeSource;

  if (_timeSource != _timeSource_old)
  {

    if (utcTime - _lastSyncd > 15)
    { // NTP server limit sync frequency to max 15 seconds
      digitalClockDisplay();
      Serial.print(F("> "));
      Serial.println(F("Yay.. last sync is more than 15 seconds ago :-)"));

      if (_timeSource == TIMESOURCE_RTC)
      {
        digitalClockDisplay();
        Serial.print(F("> "));
        Serial.println(F("Set sync provider to RTC"));
        setSyncProvider(get_time_from_rtc);
      }
      // else if (_timeSource = TIMESOURCE_NTP) {
      else
      {
        digitalClockDisplay();
        Serial.print(F("> "));
        Serial.println(F("Set sync provider to NTP"));
        setSyncProvider(getNtpTimeSDK);
      }

      // update old values only when 15 seconds has elapsed
      _timeSource_old = _timeSource;
    }
  }
}