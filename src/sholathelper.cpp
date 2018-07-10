#include <Arduino.h>
#include "sholat.h"
#include "sholathelper.h"
#include "timehelper.h"
#include "progmemmatrix.h"

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

PrayerTimes sholat;

uint8_t HOUR;
uint8_t MINUTE;
uint8_t SECOND;
uint8_t CURRENTTIMEID, NEXTTIMEID;

char bufHOUR[2];
char bufMINUTE[2];
char bufSECOND[2];

time_t currentSholatTime = 0;
time_t nextSholatTime = 0;

char sholatTimeYesterdayArray[TimesCount][6];
char sholatTimeArray[TimesCount][6];
char sholatTimeTomorrowArray[TimesCount][6];

char bufCommonSholat[30];

char *sholatNameStr(uint8_t id)
{
  if (weekday(localTime) == 6 && id == Dhuhr)
  {
    char JUMUAH[] = "JUMAT";
    strcpy(bufCommonSholat, JUMUAH);
  }
  else
  {
    strcpy_P(bufCommonSholat, sholatName_P[id]);
  }

  return bufCommonSholat;
}

void process_sholat()
{
  DEBUGLOG("\n%s\r\n", __PRETTY_FUNCTION__);

  sholat.set_calc_method(_sholatConfig.calcMethod);
  sholat.set_asr_method(_sholatConfig.asrJuristic);
  sholat.set_high_lats_adjust_method(_sholatConfig.highLatsAdjustMethod);

  if (_sholatConfig.calcMethod == Custom)
  {
    sholat.set_fajr_angle(_sholatConfig.fajrAngle);
    //sholat.set_maghrib_angle(_sholatConfig.maghribAngle);
    sholat.set_maghrib_minutes(_sholatConfig.maghribAngle);
    sholat.set_isha_angle(_sholatConfig.ishaAngle);
  }

  // apply offset to timeOffsets array
  double timeOffsets[TimesCount] = {
      //_sholatConfig.offsetImsak,
      _sholatConfig.offsetFajr,
      _sholatConfig.offsetSunrise,
      _sholatConfig.offsetDhuhr,
      _sholatConfig.offsetAsr,
      _sholatConfig.offsetSunset,
      _sholatConfig.offsetMaghrib,
      _sholatConfig.offsetIsha};

  // Tuning SHOLAT TIME
  sholat.tune(timeOffsets);

  //CALCULATE YESTERDAY'S SHOLAT TIMES
  time_t t_yesterday = localTime - 86400;
  sholat.get_prayer_times(t_yesterday, _configLocation.latitude, _configLocation.longitude, _configLocation.timezone / 10, sholat.timesYesterday);

  // print sholat times for Tomorrow
  Serial.println();
  digitalClockDisplay(t_yesterday);
  Serial.print(F(" - YESTERDAY's Schedule - "));
  Serial.println(dayStr(weekday(t_yesterday)));
  for (unsigned int i = 0; i < sizeof(sholat.timesYesterday) / sizeof(double); i++)
  {
    //Convert sholat time from float to hour and minutes
    //and store to an array (to retrieve if needed)
    const char *temp = sholat.float_time_to_time24(sholat.timesYesterday[i]);
    strlcpy(sholatTimeYesterdayArray[i], temp, sizeof(sholatTimeYesterdayArray[i]));

    //Calculate timestamp of of sholat time

    uint8_t hr, mnt;
    sholat.get_float_time_parts(sholat.timesYesterday[i], hr, mnt);

    //determine yesterday's year, month and day based on today's time
    TimeElements tm;
    breakTime(t_yesterday, tm);

    int yr, mo, d;
    yr = tm.Year + 1970;
    mo = tm.Month;
    d = tm.Day;

    time_t s_tm;
    s_tm = tmConvert_t(yr, mo, d, hr, mnt, 0);

    //store to timestamp array
    sholat.timestampSholatTimesYesterday[i] = s_tm;

    //Print all results
    //char tmpFloat[10];
    Serial.printf_P(PSTR("%d\t%-8s  %8.5f  %s  %d\r\n"),
                    i,
                    //TimeName[i],
                    sholatNameStr(i),
                    //dtostrf(sholat.timesYesterday[i], 8, 5, tmpFloat),
                    sholat.timesYesterday[i],
                    sholatTimeYesterdayArray[i],
                    sholat.timestampSholatTimesYesterday[i]);
    //Note:
    //snprintf specifier %f for float or double is not available in embedded device avr-gcc.
    //Therefore, float or double must be converted to string first. For this case I've used dtosrf to achive that.
  }

  // CALCULATE TODAY'S SHOLAT TIMES
  sholat.get_prayer_times(localTime, _configLocation.latitude, _configLocation.longitude, _configLocation.timezone / 10, sholat.times);

  // print sholat times
  Serial.println();
  digitalClockDisplay();
  Serial.print(F(" - TODAY's Schedule - "));
  Serial.println(dayStr(weekday(localTime)));
  for (unsigned int i = 0; i < sizeof(sholat.times) / sizeof(double); i++)
  {
    //Convert sholat time from float to hour and minutes
    //and store to an array (to retrieve if needed)
    const char *temp = sholat.float_time_to_time24(sholat.times[i]);
    strlcpy(sholatTimeArray[i], temp, sizeof(sholatTimeArray[i]));

    //Calculate timestamp of of sholat time
    time_t s_tm;
    uint8_t hr, mnt;
    sholat.get_float_time_parts(sholat.times[i], hr, mnt);
    s_tm = tmConvert_t(year(localTime), month(localTime), day(localTime), hr, mnt, 0);

    //store to timestamp array
    sholat.timestampSholatTimesToday[i] = s_tm;

    //Print all results
    //char tmpFloat[10];
    Serial.printf_P(PSTR("%d\t%-8s  %8.5f  %s  %d\r\n"),
                    i,
                    sholatNameStr(i),
                    //dtostrf(sholat.times[i], 8, 5, tmpFloat),
                    sholat.times[i],
                    sholatTimeArray[i],
                    sholat.timestampSholatTimesToday[i]);
    //Note:
    //snprintf specifier %f for float or double is not available in embedded device avr-gcc.
    //Therefore, float or double must be converted to string first. For this case I've used dtosrf to achive that.
  }

  //CALCULATE TOMORROW'S SHOLAT TIMES
  time_t t = localTime + 86400;
  sholat.get_prayer_times(t, _configLocation.latitude, _configLocation.longitude, _configLocation.timezone / 10, sholat.timesTomorrow);

  // print sholat times for Tomorrow
  Serial.println();
  digitalClockDisplay(t);
  Serial.print(F(" - TOMORROW's Schedule - "));
  Serial.println(dayStr(weekday(t)));
  for (unsigned int i = 0; i < sizeof(sholat.timesTomorrow) / sizeof(double); i++)
  {
    //Convert sholat time from float to hour and minutes
    //and store to an array (to retrieve if needed)
    const char *temp = sholat.float_time_to_time24(sholat.times[i]);
    strlcpy(sholatTimeTomorrowArray[i], temp, sizeof(sholatTimeTomorrowArray[i]));

    //Calculate timestamp of of sholat time

    uint8_t hr, mnt;
    sholat.get_float_time_parts(sholat.timesTomorrow[i], hr, mnt);

    //determine tomorrow's year, month and day based on today's time
    time_t t = localTime + 86400;
    TimeElements tm;
    breakTime(t, tm);

    int yr, mo, d;
    yr = tm.Year + 1970;
    mo = tm.Month;
    d = tm.Day;

    time_t s_tm;
    s_tm = tmConvert_t(yr, mo, d, hr, mnt, 0);

    //store to timestamp array
    sholat.timestampSholatTimesTomorrow[i] = s_tm;

    //Print all results
    //char tmpFloat[10];
    Serial.printf_P(PSTR("%d\t%-8s  %8.5f  %s  %d\r\n"),
                    i,
                    sholatNameStr(i),
                    //dtostrf(sholat.timesTomorrow[i], 8, 5, tmpFloat),
                    sholat.timesTomorrow[i],
                    sholatTimeTomorrowArray[i],
                    sholat.timestampSholatTimesTomorrow[i]);
    //Note:
    //snprintf specifier %f for float or double is not available in embedded device avr-gcc.
    //Therefore, float or double must be converted to string first. For this case I've used dtosrf to achive that.
  }
  Serial.println();

  //config_save_sholat("Bekasi", latitude, longitude, timezoneSholat, Egypt, Shafii, AngleBased, 20, 1, 18);
}

void process_sholat_2nd_stage()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  time_t timestamp_now = 0;

  time_t s_tm = 0;

  timestamp_now = localTime;

  //int hrNextTime, mntNextTime;

  //for (unsigned int i = 0; i < sizeof(sholat.times) / sizeof(double); i++) {
  for (int i = 0; i < TimesCount; i++)
  {
    if (i != Sunset)
    {

      //First we decide, what the ID for current and next sholat time are.
      int tempCurrentID, tempPreviousID, tempNextID;

      tempCurrentID = i;
      tempPreviousID = i - 1;
      tempNextID = i + 1;

      //check NextID
      if (tempNextID == Sunset)
      {
        tempNextID = Maghrib;
      }
      if (tempCurrentID == Isha)
      {
        tempNextID = Fajr;
      }

      //check PreviousID
      if (tempPreviousID == Sunset)
      {
        tempPreviousID = Asr;
      }
      if (tempCurrentID == Fajr)
      {
        tempPreviousID = Isha;
      }

      //then
      time_t timestamp_current_today;
      time_t timestamp_next_today;
      time_t timestamp_next_tomorrow;

      timestamp_current_today = sholat.timestampSholatTimesToday[tempCurrentID];

      timestamp_next_today = sholat.timestampSholatTimesToday[tempNextID];

      timestamp_next_tomorrow = sholat.timestampSholatTimesTomorrow[tempNextID];

      if (timestamp_current_today < timestamp_next_today)
      {
        if (timestamp_now <= timestamp_current_today && timestamp_now < timestamp_next_today)
        {
          CURRENTTIMEID = tempPreviousID;
          NEXTTIMEID = tempCurrentID;
          s_tm = timestamp_current_today;

          break;
        }
        else if (timestamp_now > timestamp_current_today && timestamp_now <= timestamp_next_today)
        {
          CURRENTTIMEID = tempCurrentID;
          NEXTTIMEID = tempNextID;
          s_tm = timestamp_next_today;

          break;
        }
      }
      else if (timestamp_current_today > timestamp_next_today)
      {
        if (timestamp_now >= timestamp_current_today && timestamp_now < timestamp_next_tomorrow)
        {
          CURRENTTIMEID = tempCurrentID;
          NEXTTIMEID = tempNextID;
          s_tm = timestamp_next_tomorrow;

          break;
        }
      }
    }
  } //end of for loop

  time_t timestamp_current_yesterday;
  time_t timestamp_current_today;
  time_t timestamp_next_today;
  time_t timestamp_next_tomorrow;

  timestamp_current_yesterday = sholat.timestampSholatTimesYesterday[CURRENTTIMEID];
  timestamp_current_today = sholat.timestampSholatTimesToday[CURRENTTIMEID];
  timestamp_next_today = sholat.timestampSholatTimesToday[NEXTTIMEID];
  timestamp_next_tomorrow = sholat.timestampSholatTimesTomorrow[NEXTTIMEID];

  if (NEXTTIMEID > CURRENTTIMEID)
  {
    currentSholatTime = timestamp_current_today;
    nextSholatTime = timestamp_next_today;
    //PRINT("%s %lu %lu\n", "Case 2a", currentSholatTime, nextSholatTime);
  }
  else if (NEXTTIMEID < CURRENTTIMEID)
  {
    if (isPM(localTime))
    {
      currentSholatTime = timestamp_current_today;
      nextSholatTime = timestamp_next_tomorrow;
      //PRINT("%s %lu %lu\n", "Case 2b", currentSholatTime, nextSholatTime);
    }
    if (isAM(localTime))
    {
      currentSholatTime = timestamp_current_yesterday;
      nextSholatTime = timestamp_next_today;
      //PRINT("%s %lu %lu\n", "Case 2c", currentSholatTime, nextSholatTime);
    }
  }

  //uint8_t lenCURRENTTIMENAME = strCURRENTTIMENAME.length();
  //char bufCURRENTTIMENAME[lenCURRENTTIMENAME + 1];

  //lenNEXTTIMENAME = NEXTTIMENAME.length();
  //char bufNEXTTIMENAME[lenNEXTTIMENAME + 1];

  //  uint8_t len1 = strlen(CURRENTTIMENAME);
  //  char bufCURRENTTIMENAME[len1 + 1];
  //
  //  uint8_t len2 = strlen(NEXTTIMENAME);
  //  char bufNEXTTIMENAME[len2 + 1];

  time_t timeDiff = s_tm - localTime;

  // uint16_t days;
  uint8_t hr;
  uint8_t mnt;
  uint8_t sec;

  /* // METHOD 1

    char *s;
    s = TimeToString(timeDiff, &hr, &mnt, &sec);

    //  DEBUGLOG(getDateTimeString(localTime).c_str());
    //  DEBUGLOG("> ");
    //  DEBUGLOG(TimeName[CURRENTTIMEID]);
    //  DEBUGLOG("->");
    //  DEBUGLOG(TimeName[NEXTTIMEID]);
    //  DEBUGLOG(", ");
    //  DEBUGLOG("ts_Now: ");
    //  DEBUGLOG("%ld", localTime;
    //  DEBUGLOG(", ");
    //  DEBUGLOG("ts_Next: ");
    //  DEBUGLOG("%ld", s_tm);
    //  DEBUGLOG(", ");
    //  DEBUGLOG("diff: ");
    //  DEBUGLOG("%ld", timeDiff);
    //  DEBUGLOG(", ");
    //  DEBUGLOG("h,m,s: %d:%d:%d\n\r", hr, mnt, sec);
    //  //DEBUGLOG(s);
  */

  // METHOD 2
  //days = elapsedDays(timeDiff);
  hr = numberOfHours(timeDiff);
  mnt = numberOfMinutes(timeDiff);
  sec = numberOfSeconds(timeDiff);

  HOUR = hr;
  MINUTE = mnt;
  SECOND = sec;

  static int HOUR_old = 100;

  if (HOUR != HOUR_old)
  {
    HOUR_old = HOUR;
    dtostrf(HOUR, 1, 0, bufHOUR);
  }

  static int MINUTE_old = 100;

  if (MINUTE != MINUTE_old)
  {
    MINUTE_old = MINUTE;
    dtostrf(MINUTE, 1, 0, bufMINUTE);
  }

  static int SECOND_old = 100;

  if (SECOND != SECOND_old)
  {
    SECOND_old = SECOND;
    dtostrf(SECOND, 1, 0, bufSECOND);
  }

  if (SECOND == 0)
  {
    digitalClockDisplay();
    Serial.print(F("> "));

    if (HOUR != 0 || MINUTE != 0)
    {
      if (HOUR != 0)
      {
        Serial.print(bufHOUR);
        Serial.print(F(" jam "));
      }
      Serial.print(bufMINUTE);
      Serial.print(F(" min menuju "));
      Serial.println(sholatNameStr(NEXTTIMEID));
    }
    //else if (HOUR == 0 && MINUTE == 0) {
    else if (HOUR == 0 && MINUTE == 0)
    {
      Serial.print(F("Waktu "));
      Serial.print(sholatNameStr(NEXTTIMEID));
      Serial.println(F(" telah masuk!"));
      Serial.println(F("Dirikanlah sholat tepat waktu."));
    }
  }
}

void ProcessSholatEverySecond()
{
  static time_t t_nextMidnight = 0;

  time_t t = nextMidnight(localTime);

  if (t != t_nextMidnight)
  {
    t_nextMidnight = t;

    process_sholat();
  }

  process_sholat_2nd_stage();

  static int NEXTTIMEID_old = 100;

  if (NEXTTIMEID != NEXTTIMEID_old)
  {
    NEXTTIMEID_old = NEXTTIMEID;
  }
}