#include "rtchelper.h"

#define DEBUGPORT Serial

// #define RELEASE

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
  // DEBUGCONFIG("%s\r\n", __PRETTY_FUNCTION__);
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
  syncRtcEventTriggered = true;

  Serial.println(F("Execute get_time_from_rtc()"));

  if (GetRtcStatus() == RTC_TIME_VALID)
  {
    syncSuccessByRtc = true;

    RtcDateTime dt = Rtc.GetDateTime();

    /*
      while (dt == Rtc.GetDateTime()) {
      // wait the RTC time to change
      }
      // save the new time
      dt = Rtc.GetDateTime();
    */

    time_t t = dt.Epoch32Time();

    Serial.println(F("RTC time is valid"));

    Serial.print(F("RTC timestamp: "));
    Serial.println(t);

    // _lastSyncd = t;

    return t;
  }

  Serial.println(GetRtcStatusString());
  return 0; // return 0 if unable to get the time
}