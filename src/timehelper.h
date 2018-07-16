#ifndef timehelper_h
#define timehelper_h

// #include <TimeLib.h>
// #include <time.h>

extern bool tick1000ms;



extern time_t now;
extern time_t _lastSyncd; ///< Stored time of last successful sync
extern time_t _firstSync; ///< Stored time of first successful sync after boot
extern time_t _lastBoot;

typedef struct
{
  // int8_t timezone = _configLocation.timezone;
  bool dst = false;
  bool enablertc = true;
  uint32_t syncinterval = 600;
  bool enablentp = true;
  char ntpserver_0[48] = "0.id.pool.ntp.org";
  char ntpserver_1[48] = "0.asia.pool.ntp.org";
  char ntpserver_2[48] = "192.168.10.1";
} strConfigTime;
extern strConfigTime _configTime;

typedef enum timeSource
{
  TIMESOURCE_NOT_AVAILABLE,
  TIMESOURCE_NTP,
  TIMESOURCE_RTC
} TIMESOURCE;
extern TIMESOURCE _timeSource;


char *getDateStr();
char *getTimeStr();
char *getUptimeStr();
char *getLastSyncStr();
char *getNextSyncStr();

time_t tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss);

void timeSetup();

#endif