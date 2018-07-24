#ifndef rtchelper_h
#define rtchelper_h

#include <RtcDS3231.h>      //RTC library
#include <Wire.h>

extern bool syncRtcEventTriggered; // True if a time even has been triggered
extern bool syncSuccessByRtc;
extern bool RTC_OK;

typedef enum {
  RTC_TIME_VALID,
  RTC_LOST_CONFIDENT,
  CLOCK_NOT_RUNNING
} RTCStatus;

uint8_t GetRtcStatus();
time_t get_time_from_rtc();

extern RtcDS3231<TwoWire> Rtc;

// void printRtcTime(const RtcDateTime &dt);
// char *GetRtcDateTimeStr(const RtcDateTime &dt);
void RtcSetup();

#endif