#ifndef sholathelper_h
#define sholathelper_h

extern PrayerTimes sholat;

extern uint8_t HOUR;
extern uint8_t MINUTE;
extern uint8_t SECOND;
extern uint8_t CURRENTTIMEID, NEXTTIMEID;

extern char bufHOUR[2];
extern char bufMINUTE[2];
extern char bufSECOND[2];

extern char sholatTimeYesterdayArray[TimesCount][6];
extern char sholatTimeArray[TimesCount][6];
extern char sholatTimeTomorrowArray[TimesCount][6];

char* sholatNameStr(uint8_t id);
void process_sholat();
void process_sholat_2nd_stage();
void ProcessSholatEverySecond();


#endif