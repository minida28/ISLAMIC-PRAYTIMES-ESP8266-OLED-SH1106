#include <Arduino.h>
#include "sholat.h"
#include "sholathelper.h"
// #include "timehelper.h"
#include "progmemmatrix.h"

#define PROGMEM_T __attribute__((section(".irom.text.template")))

#define PRINTPORT Serial
#define DEBUGPORT Serial

#define RELEASE

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

PrayerTimes sholat;

uint8_t HOUR;
uint8_t MINUTE;
uint8_t SECOND;
uint8_t CURRENTTIMEID, NEXTTIMEID;

char bufHOUR[2];
char bufMINUTE[2];
char bufSECOND[2];

// utc
// time_t t_utc = time(nullptr);
// struct tm *tm_utc = gmtime(&t_utc);

// // local
// time_t t_local = t_utc + _configLocation.timezone / 10.0 * 3600;
// struct tm *tm_local = localtime(&t_utc);

time_t currentSholatTime = 0;
time_t nextSholatTime = 0;

char sholatTimeYesterdayArray[TimesCount][6];
char sholatTimeArray[TimesCount][6];
char sholatTimeTomorrowArray[TimesCount][6];

char bufCommonSholat[30];

char *sholatNameStr(uint8_t id)
{
  time_t t_utc = time(nullptr);
  struct tm *tm_local = localtime(&t_utc);

  if (tm_local->tm_wday == 6 && id == Dhuhr)
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

  // timezone in seconds

  // location settings;
  double lat = _configLocation.latitude;
  double lon = _configLocation.longitude;
  float tZ = TimezoneFloat();

  time_t now;
  struct tm *tm;
  int year;
  int month;
  int day;

  time(&now);

  //CALCULATE YESTERDAY'S SHOLAT TIMES    
  tm = localtime(&now);

  tm->tm_mday--; // alter tm struct to yesterday

  time_t t_yesterday = mktime(tm);
  tm = localtime(&t_yesterday);

  year = tm->tm_year + 1900;
  month = tm->tm_mon + 1;
  day = tm->tm_mday;

  sholat.get_prayer_times(year, month, day, lat, lon, tZ, sholat.timesYesterday);

  // print sholat times for yesterday
  PRINT("\r\nYESTERDAY's Schedule - %s", asctime(tm));
  for (unsigned int i = 0; i < sizeof(sholat.timesYesterday) / sizeof(double); i++)
  {
    // Convert sholat time from float to hour and minutes
    // and store to an array (to retrieve if needed)
    const char *temp = sholat.float_time_to_time24(sholat.timesYesterday[i]);
    strlcpy(sholatTimeYesterdayArray[i], temp, sizeof(sholatTimeYesterdayArray[i]));

    // Calculate timestamp of of sholat time

    uint8_t hr, mnt;
    sholat.get_float_time_parts(sholat.timesYesterday[i], hr, mnt);

    // modify time struct
    tm->tm_hour = hr;
    tm->tm_min = mnt;
    tm->tm_sec = 0;

    //store to timestamp array
    sholat.timestampSholatTimesYesterday[i] = mktime(tm);

    //Print all results
    //char tmpFloat[10];
    PRINT("%d\t%-8s  %8.5f  %s  %lu\r\n",
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
  tm = localtime(&now);
  
  year = tm->tm_year + 1900;
  month = tm->tm_mon + 1;
  day = tm->tm_mday;

  sholat.get_prayer_times(year, month, day, lat, lon, tZ, sholat.times);

  // print sholat times
  PRINT("\r\nTODAY's Schedule - %s", asctime(tm));
  for (unsigned int i = 0; i < sizeof(sholat.times) / sizeof(double); i++)
  {
    //Convert sholat time from float to hour and minutes
    //and store to an array (to retrieve if needed)
    const char *temp = sholat.float_time_to_time24(sholat.times[i]);
    strlcpy(sholatTimeArray[i], temp, sizeof(sholatTimeArray[i]));

    //Calculate timestamp of of sholat time
    uint8_t hr, mnt;
    sholat.get_float_time_parts(sholat.times[i], hr, mnt);

    // modify time struct
    tm->tm_hour = hr;
    tm->tm_min = mnt;
    tm->tm_sec = 0;

    //store to timestamp array
    sholat.timestampSholatTimesToday[i] = mktime(tm);

    //Print all results
    //char tmpFloat[10];
    PRINT("%d\t%-8s  %8.5f  %s  %lu\r\n",
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

  // CALCULATE TOMORROW'S SHOLAT TIMES
  tm = localtime(&now);

  tm->tm_mday++; // alter tm struct to tomorrow
  time_t t_tomorrow = mktime(tm);
  tm = localtime(&t_tomorrow);

  year = tm->tm_year + 1900;
  month = tm->tm_mon + 1;
  day = tm->tm_mday;

  sholat.get_prayer_times(year, month, day, lat, lon, tZ, sholat.timesTomorrow);

  // print sholat times for Tomorrow
  PRINT("\r\nTOMORROW's Schedule - %s", asctime(tm));
  for (unsigned int i = 0; i < sizeof(sholat.timesTomorrow) / sizeof(double); i++)
  {
    //Convert sholat time from float to hour and minutes
    //and store to an array (to retrieve if needed)
    const char *temp = sholat.float_time_to_time24(sholat.times[i]);
    strlcpy(sholatTimeTomorrowArray[i], temp, sizeof(sholatTimeTomorrowArray[i]));

    //Calculate timestamp of of sholat time
    uint8_t hr, mnt;
    sholat.get_float_time_parts(sholat.timesTomorrow[i], hr, mnt);

    // modify time struct
    tm->tm_hour = hr;
    tm->tm_min = mnt;
    tm->tm_sec = 0;

    //store to timestamp array
    sholat.timestampSholatTimesTomorrow[i] = mktime(tm);
    ;

    //Print all results
    //char tmpFloat[10];
    PRINT("%d\t%-8s  %8.5f  %s  %lu\r\n",
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
  // DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  time_t s_tm = 0;

  time_t t_utc = time(nullptr);

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

      DEBUGLOG("timestamp_current_today=%lu, timestamp_next_today=%lu, timestamp_next_tomorrow=%lu\r\n", timestamp_current_today, timestamp_next_today, timestamp_next_tomorrow);

      if (timestamp_current_today < timestamp_next_today)
      {
        if (t_utc <= timestamp_current_today && t_utc < timestamp_next_today)
        {
          CURRENTTIMEID = tempPreviousID;
          NEXTTIMEID = tempCurrentID;
          s_tm = timestamp_current_today;

          break;
        }
        else if (t_utc > timestamp_current_today && t_utc <= timestamp_next_today)
        {
          CURRENTTIMEID = tempCurrentID;
          NEXTTIMEID = tempNextID;
          s_tm = timestamp_next_today;

          break;
        }
      }
      else if (timestamp_current_today > timestamp_next_today)
      {
        if (t_utc >= timestamp_current_today && t_utc < timestamp_next_tomorrow)
        {
          CURRENTTIMEID = tempCurrentID;
          NEXTTIMEID = tempNextID;
          s_tm = timestamp_next_tomorrow;

          break;
        }
      }
    }
  } //end of for loop

  DEBUGLOG("CURRENTTIMEID=%d, NEXTTIMEID=%d\r\n", CURRENTTIMEID, NEXTTIMEID);

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
    DEBUGLOG("NEXTTIMEID > CURRENTTIMEID, currentSholatTime=%lu, nextSholatTime=%lu\r\n", currentSholatTime, nextSholatTime);
  }
  else if (NEXTTIMEID < CURRENTTIMEID)
  {
    time_t t_utc = time(nullptr);
    struct tm *tm_utc = gmtime(&t_utc);

    if (tm_utc->tm_hour >= 12) // is PM ?
    {
      currentSholatTime = timestamp_current_today;
      nextSholatTime = timestamp_next_tomorrow;
      DEBUGLOG("NEXTTIMEID < CURRENTTIMEID, currentSholatTime=%lu, nextSholatTime=%lu Hour: %d, is PM\r\n", currentSholatTime, nextSholatTime, tm_utc->tm_hour);
    }
    if (tm_utc->tm_hour < 12) // is AM ?
    {
      currentSholatTime = timestamp_current_yesterday;
      nextSholatTime = timestamp_next_today;
      //PRINT("%s %lu %lu\n", "Case 2c", currentSholatTime, nextSholatTime);
      DEBUGLOG("NEXTTIMEID < CURRENTTIMEID, currentSholatTime=%lu, nextSholatTime=%lu Hour: %d, is PM\r\n", currentSholatTime, nextSholatTime, tm_utc->tm_hour);
    }
  }

  time_t timeDiff = s_tm - t_utc;
  DEBUGLOG("s_tm: %lu, t_utc: %lu, timeDiff: %lu\r\n", s_tm, t_utc, timeDiff);

  // uint16_t days;
  // uint8_t hr;
  // uint8_t mnt;
  // uint8_t sec;

  // // METHOD 2
  // // days = elapsedDays(timeDiff);
  // HOUR = numberOfHours(timeDiff);
  // MINUTE = numberOfMinutes(timeDiff);
  // SECOND = numberOfSeconds(timeDiff);

  // METHOD 3 -> https://stackoverflow.com/questions/2419562/convert-seconds-to-days-minutes-and-seconds/17590511#17590511
  tm *diff = gmtime(&timeDiff); // convert to broken down time
  // DAYS = diff->tm_yday;
  HOUR = diff->tm_hour;
  MINUTE = diff->tm_min;
  SECOND = diff->tm_sec;

  // METHOD 4 -> https://stackoverflow.com/questions/2419562/convert-seconds-to-days-minutes-and-seconds/2419597#2419597
  // HOUR = floor(timeDiff / 3600.0);
  // MINUTE = floor(fmod(timeDiff, 3600.0) / 60.0);
  // SECOND = fmod(timeDiff, 60.0);

  static int SECOND_old = 100;

  if (SECOND != SECOND_old)
  {
    SECOND_old = SECOND;
    dtostrf(SECOND, 1, 0, bufSECOND);
    dtostrf(MINUTE, 1, 0, bufMINUTE);
    dtostrf(HOUR, 1, 0, bufHOUR);
  }

  if (SECOND == 0)
  {
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
  time_t now = time(nullptr);
  if (now >= nextSholatTime)
  {
    process_sholat();
  }

  process_sholat_2nd_stage();

  static int NEXTTIMEID_old = 100;

  if (NEXTTIMEID != NEXTTIMEID_old)
  {
    NEXTTIMEID_old = NEXTTIMEID;
  }
}

float TimezoneFloat()
{
  time_t rawtime;
  struct tm *timeinfo;
  char buffer[6];

  time(&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(buffer, 6, "%z", timeinfo);

  char bufTzHour[4];
  strncpy(bufTzHour, buffer, 3);
  int8_t hour = atoi(bufTzHour);

  char bufTzMin[4];
  bufTzMin[0] = buffer[0]; // sign
  bufTzMin[1] = buffer[3];
  bufTzMin[2] = buffer[4];
  float min = atoi(bufTzMin) / 60.0;

  float TZ_FLOAT = hour + min;
  return TZ_FLOAT;
}

int32_t TimezoneMinutes()
{
  return TimezoneFloat() * 60;
}

int32_t TimezoneSeconds()
{
  return TimezoneMinutes() * 60;
}