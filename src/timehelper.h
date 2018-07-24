#ifndef timehelper_h
#define timehelper_h

#include <time.h>
#include <sys/time.h>  // struct timeval
#include <coredecls.h> // settimeofday_cb()
#include "rtchelper.h"
#include <Ticker.h>

// for testing purpose:
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

extern bool tick1000ms;
extern bool state500ms;

extern bool timeSetFlag;

extern time_t now;
extern time_t lastSync; ///< Stored time of last successful sync
extern time_t _firstSync; ///< Stored time of first successful sync after boot
extern time_t _lastBoot;

typedef struct
{
  // int8_t timezone = _configLocation.timezone;
  bool dst = false;
  bool enablentp = true;
  char ntpserver_0[48] = "0.id.pool.ntp.org";
  char ntpserver_1[48] = "0.asia.pool.ntp.org";
  char ntpserver_2[48] = "192.168.10.1";
  bool enablertc = true;
  uint32_t syncinterval = 600;
} strConfigTime;
extern strConfigTime _configTime;

typedef enum timeSource
{
  TIMESOURCE_NOT_AVAILABLE,
  TIMESOURCE_NTP,
  TIMESOURCE_RTC
} strTimeSource;
extern strTimeSource timeSource;

char *getDateStr();
char *getTimeStr();
char *getDateTimeStr();
char *GetRtcDateTimeStr(const RtcDateTime &dt);
char *getLastBootStr();
char *getUptimeStr();
char *getLastSyncStr();
char *getNextSyncStr();

/* date strings */ 
// #define dt_MAX_STRING_LEN 9 // length of longest date string (excluding terminating null)
char* monthStr(uint8_t month);
char* dayStr(uint8_t day);
char* monthShortStr(uint8_t month);
char* dayShortStr(uint8_t day);

time_t tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss);

void TimeSetup();
void TimeLoop();

#endif