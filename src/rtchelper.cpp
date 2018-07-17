#include "rtchelper.h"
#include <time.h>

#define PROGMEM_T __attribute__((section(".irom.text.template")))

#define PRINTPORT Serial
#define DEBUGPORT Serial

// #define RELEASE

#define PRINT(fmt, ...)                       \
  {                                           \
    static const char pfmt[] PROGMEM_T = fmt; \
    PRINTPORT.printf_P(pfmt, ##__VA_ARGS__);  \
  }

#ifndef RELEASE
#define DEBUGLOG(fmt, ...)                    \
  {                                           \
    static const char pfmt[] PROGMEM_T = fmt; \
    DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__);  \
  }
#else
#define DEBUGLOG(...)
#endif

RtcDS3231<TwoWire> Rtc(Wire);

bool syncRtcEventTriggered; // True if a time even has been triggered
bool syncSuccessByRtc;
bool RTC_OK;

char bufCommonRtc[30];

char *GetRtcStatusString()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  byte currentRtcStatus = GetRtcStatus();

  if (currentRtcStatus == RTC_TIME_VALID)
  {
    strcpy_P(bufCommonRtc, PSTR("RTC_TIME_VALID"));
  }
  else if (currentRtcStatus == RTC_LOST_CONFIDENT)
  {
    strcpy_P(bufCommonRtc, PSTR("RTC_LOST_CONFIDENT"));
  }
  else if (currentRtcStatus == CLOCK_NOT_RUNNING)
  {
    strcpy_P(bufCommonRtc, PSTR("CLOCK_NOT_RUNNING"));
  }
  return bufCommonRtc;
}

uint8_t GetRtcStatus()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  // status = 0; RTC time is valid.
  // status = 1; RTC lost confidence in the DateTime!
  // status = 2; Actual clock is NOT running on the RTC

  byte rtcStatus = RTC_LOST_CONFIDENT;

  if (Rtc.GetIsRunning())
  {
    if (Rtc.IsDateTimeValid())
    {
      rtcStatus = RTC_TIME_VALID;
    }
    else if (!Rtc.IsDateTimeValid())
    {
      rtcStatus = RTC_LOST_CONFIDENT;
    }
  }
  else
  {
    rtcStatus = CLOCK_NOT_RUNNING;
  }

  return rtcStatus;
}

time_t get_time_from_rtc()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  syncRtcEventTriggered = true;

  if (GetRtcStatus() == RTC_TIME_VALID)
  {
    syncSuccessByRtc = true;

    RtcDateTime dt = Rtc.GetDateTime();

    time_t t = dt.Epoch32Time();

    // _lastSyncd = t;

    return t;
  }

  return 0; // return 0 if unable to get the time
}

void printRtcTime(const RtcDateTime &dt)
{
  char datestring[20];

  snprintf_P(datestring,
             //countof(datestring),
             (sizeof(datestring) / sizeof(datestring[0])),
             PSTR("%04u/%02u/%02uT%02u:%02u:%02u"),

             dt.Year(),
             dt.Month(),
             dt.Day(),

             dt.Hour(),
             dt.Minute(),
             dt.Second()

  );

  Serial.print(datestring);
}

char *getDateTimeString(time_t moment)
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  static char buf[60];
  strftime(buf, (sizeof(buf) / sizeof(buf[0])), "%a %b %d %Y %X GMT", gmtime(&moment));
  return buf;
}

void rtcSetup()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  Rtc.Begin();

  if (!Rtc.IsDateTimeValid())
  {
    // Common Causes:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing

    RtcDateTime dt = Rtc.GetDateTime();
    time_t timeRtc = dt.Epoch32Time();

    PRINT("RTC LOST CONFIDENCE, timestamp (UTC):%li, %s\r\n", timeRtc, getDateTimeString(timeRtc));

    unsigned long currMillis = millis();
    while (!Rtc.IsDateTimeValid() && millis() - currMillis <= 5000)
    {
      delay(500);
      PRINT(">");
    }
    PRINT("\r\n");

    if (timeRtc < 1514764800)
    {
      PRINT("RTC is older than 1 January 2018 :-(\r\n");
    }
    else if (timeRtc > 1514764800)
    {
      PRINT("but strange.., RTC is seems valid, 1.e. later than 1 January 2018...\r\n");
      //  RtcDateTime timeToSetToRTC;
      //  timeToSetToRTC.InitWithEpoch32Time(timeRtc);
      //  Rtc.SetDateTime(timeToSetToRTC);
    }
  }
  else
  {
    RtcDateTime dt = Rtc.GetDateTime();
    time_t timeRtc = dt.Epoch32Time();
    PRINT("RTC time is VALID, timestamp (UTC):%li, %s\r\n", timeRtc, getDateTimeString(timeRtc));
  }

  if (!Rtc.GetIsRunning())
  {
    PRINT("RTC was not actively running, starting now.\r\n");
    Rtc.SetIsRunning(true);
  }

  // never assume the Rtc was last configured by you, so
  // just clear them to your needed state
  Rtc.Enable32kHzPin(true);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock);
  Rtc.SetSquareWavePinClockFrequency(DS3231SquareWaveClock_1Hz);
}