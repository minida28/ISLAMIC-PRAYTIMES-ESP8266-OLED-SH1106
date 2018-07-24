#include <U8g2lib.h>
#include "displayhelper.h"
#include "timehelper.h"
#include "sholathelper.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define DEBUGPORT Serial

// #define RELEASE

#ifndef RELEASE
#define DEBUGLOG(fmt, ...)                       \
    {                                            \
        static const char pfmt[] PROGMEM = fmt;  \
        DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
    }
#define DEBUGLOGLN(fmt, ...)                     \
    {                                            \
        static const char pfmt[] PROGMEM = fmt;  \
        static const char rn[] PROGMEM = "\r\n"; \
        DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
        DEBUGPORT.printf_P(rn);                  \
    }
#else
#define DEBUGLOG(...)
#define DEBUGLOGLN(...)
#endif

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

void u8g2_box_frame(uint8_t a)
{
    u8g2.drawStr(0, 0, "drawBox");
    u8g2.drawBox(5, 10, 20, 10);
    u8g2.drawBox(10 + a, 15, 30, 7);
    u8g2.drawStr(0, 30, "drawFrame");
    u8g2.drawFrame(5, 10 + 30, 20, 10);
    u8g2.drawFrame(10 + a, 15 + 30, 30, 7);
}

void u8g2_disc_circle(uint8_t a)
{
    u8g2.drawStr(0, 0, "drawDisc");
    u8g2.drawDisc(10, 18, 9);
    u8g2.drawDisc(24 + a, 16, 7);
    u8g2.drawStr(0, 30, "drawCircle");
    u8g2.drawCircle(10, 18 + 30, 9);
    u8g2.drawCircle(24 + a, 16 + 30, 7);
}

void u8g2_r_frame(uint8_t a)
{
    u8g2.drawStr(0, 0, "drawRFrame/Box");
    u8g2.drawRFrame(5, 10, 40, 30, a + 1);
    u8g2.drawRBox(50, 10, 25, 40, a + 1);
}

void u8g2_string(uint16_t a)
{
    u8g2.setFontDirection(0);
    char buf[36];
    if (HOUR != 0)
    {
        snprintf(buf, sizeof(buf), "%d jam %2d menit menuju %s.", HOUR, MINUTE, sholatNameStr(NEXTTIMEID));
    }
    else
    {
        snprintf(buf, sizeof(buf), "%d menit %2d detik menuju %s.", MINUTE, SECOND, sholatNameStr(NEXTTIMEID));
    }
    u8g2.drawStr(128 - a, 0, buf);
}

void u8g2_string2(uint16_t a)
{
    u8g2.setFontDirection(0);
    // u8g2.setFont(u8g2_font_profont15_tf);
    u8g2.setFont(u8g2_font_logisoso16_tf);
    char buf[36];
    if (state500ms)
    {
        snprintf(buf, sizeof(buf), "- %d:%02d:%02d", HOUR, MINUTE, SECOND);
    }
    else
    {
        snprintf(buf, sizeof(buf), "  %d:%02d:%02d", HOUR, MINUTE, SECOND);
    }
    u8g2.drawStr(0, 0, buf);

    if (state500ms)
    {
        snprintf(buf, sizeof(buf), "- %d:%02d", ceilHOUR, ceilMINUTE);
    }
    else
    {
        snprintf(buf, sizeof(buf), "  %d:%02d", ceilHOUR, ceilMINUTE);
    }
    u8g2.drawStr(0, 20, buf);
}

void u8g2_string3(uint16_t a)
{
    // time_t rawtime = time(nullptr);
    tm *tm = localtime(&now);

    uint8_t hr, min /*, sec*/;
    hr = tm->tm_hour;
    min = tm->tm_min;
    // sec = tm->tm_sec;

    char buf[36];

    const uint8_t *fontC;

    // fontC = u8g2_font_6x10_mn;
    fontC = u8g2_font_smart_patrol_nbp_tn;
    // fontC = u8g2_font_profont15_mn;
    // fontC = u8g2_font_px437wyse700b_mn;
    // fontC = u8g2_font_profont17_mn;
    // fontC = u8g2_font_t0_18_me;
    // fontC = u8g2_font_balthasar_titling_nbp_tn;
    // fontC = u8g2_font_logisoso16_tn;

    u8g2.setFont(fontC);

    bool format12 = false;
    if (format12)
    {
        if (hr > 12)
        {
            hr = hr - 12;
        }
    }

    int dW;
    int dH;
    const char *str;
    int sW;

    dW = u8g2.getDisplayWidth();
    snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
    sW = u8g2.getStrWidth(buf);

    u8g2.setFontPosTop();
    u8g2.setCursor((dW - sW) / 2, -1);

    u8g2.print(buf);

    bool blinkColon = true;
    if (blinkColon)
    {
        if (!state500ms)
        {
            u8g2.setCursor((dW - sW) / 2, -1);
            u8g2.print(hr);
            u8g2.setFontMode(1);
            u8g2.setDrawColor(0);
            u8g2.print(":");
            u8g2.setFontMode(0);  // default
            u8g2.setDrawColor(1); // default
        }
    }

    // u8g2.setFontRefHeightExtendedText();
    u8g2.setDrawColor(1);
    // u8g2.setFontPosTop();
    u8g2.setFontPosBottom();
    // u8g2.setFontDirection(0);

    if (true)
    {
        // u8g2.setFont(u8g2_font_helvB12_tf);
        // snprintf(buf, sizeof(buf), "%d jam %d menit", ceilHOUR, ceilMINUTE);

        const uint8_t *font0;
        const uint8_t *font1;

        // font0 = u8g2_font_helvB12_tn;
        // font0 = u8g2_font_t0_16_mn;
        // font0 = u8g2_font_logisoso16_tn;
        // font0 = u8g2_font_smart_patrol_nbp_tn;
        font0 = u8g2_font_profont15_mn;

        // font1 = u8g2_font_t0_12_mr
        font1 = u8g2_font_t0_12_tr;

        u8g2.setCursor(3, 32);
        u8g2.setFont(font0);
        u8g2.print(HOUR);

        u8g2.setCursor(u8g2.tx + 2, u8g2.ty - 0);
        u8g2.setFont(font1);
        u8g2.print("jam ");

        u8g2.setCursor(u8g2.tx, u8g2.ty + 0);
        snprintf(buf, sizeof(buf), "%2d", MINUTE);
        u8g2.setFont(font0);
        u8g2.print(buf);

        u8g2.setCursor(u8g2.tx + 2, u8g2.ty - 0);
        u8g2.setFont(font1);
        u8g2.print("mnt ");

        u8g2.setCursor(u8g2.tx, u8g2.ty + 0);
        u8g2.setFont(font0);
        snprintf(buf, sizeof(buf), "%2d", SECOND);
        u8g2.print(buf);

        u8g2.setCursor(u8g2.tx + 2, u8g2.ty - 0);
        u8g2.setFont(font1);
        u8g2.print("dtk");
    }
    else if (ceilHOUR == 0)
    {
        u8g2.setFont(u8g2_font_logisoso16_tf);
        snprintf(buf, sizeof(buf), "%d menit", ceilMINUTE);
    }

    u8g2.setFont(u8g2_font_t0_12_mr);

    dW = u8g2.getDisplayWidth();
    dH = u8g2.getDisplayHeight();
    str = "menuju";
    sW = u8g2.getStrWidth(str);
    u8g2.setFontPosTop();
    u8g2.setCursor((dW - sW) / 2, u8g2.ty + 2);
    u8g2.print(str);

    u8g2.setFontPosBottom();
    // u8g2.setCursor(0, 67);
    // u8g2.setCursor(u8g2.tx + 5, 20);
    // u8g2.setFont(u8g2_font_logisoso16_tf);
    // u8g2.setFont(u8g2_font_inb16_mr);
    // u8g2.setFont(u8g2_font_helvB12_tf);
    u8g2.setFont(u8g2_font_smart_patrol_nbp_tr);

    dW = u8g2.getDisplayWidth();
    dH = u8g2.getDisplayHeight();
    str = sholatNameStr(NEXTTIMEID);
    sW = u8g2.getStrWidth(str);

    // Serial.printf("dW=%d, dH=%d, sW=%d\r\n", dW, dH, sW);

    u8g2.setCursor((dW - sW) / 2, dH + 0);
    // u8g2.setCursor(26, 63);

    u8g2.print(str);

    // const uint8_t *f;
    // f = u8g2_font_crox4hb_tr;

    // u8g2.setFontPosTop();
    // u8g2.setFont(f);
    // u8g2.setCursor(0, 0);
    // u8g2.print("AgjIy180");

    // u8g2.drawHLine(0, 0, 127);
    // u8g2.drawHLine(0, 15, 127);
    // u8g2.drawHLine(0, 31, 127);
    // u8g2.drawHLine(0, 47, 127);
    // u8g2.drawHLine(0, 63, 127);

    // u8g2.drawVLine(127, 0, 63);
    // u8g2.drawStr(0, 20, buf);
    // u8g2.setFont(u8g2_font_logisoso16_tf);
    // snprintf(buf, sizeof(buf), "%s", sholatNameStr(NEXTTIMEID));
    // u8g2.drawStr(u8g2.tx, 20, buf);
}

void u8g2_string4(uint16_t a)
{
    tm *tm = localtime(&now);

    uint8_t hr, min /*, sec*/;
    hr = tm->tm_hour;
    min = tm->tm_min;
    // sec = tm->tm_sec;

    char buf[36];

    const uint8_t *fontC;

    // fontC = u8g2_font_6x10_mn;
    // fontC = u8g2_font_smart_patrol_nbp_tn;
    // fontC = u8g2_font_profont15_mn;
    // fontC = u8g2_font_px437wyse700b_mn;
    // fontC = u8g2_font_profont17_mn;
    // fontC = u8g2_font_t0_18_me;
    // fontC = u8g2_font_balthasar_titling_nbp_tn;
    // fontC = u8g2_font_logisoso16_tn;
    fontC = u8g2_font_logisoso46_tn;
    // fontC = u8g2_font_inb33_mn;
    // fontC = u8g2_font_inr33_mn;

    u8g2.setFont(fontC);

    bool format12 = true;
    if (format12)
    {
        if (hr == 0)
        {
            hr = 12;
        }

        if (hr > 12)
        {
            hr = hr - 12;
        }
    }

    int dW;
    int dH;
    // const char *str;
    int sW;
    int ascent;

    int x, y;

    dH = u8g2.getDisplayHeight();
    dW = u8g2.getDisplayWidth();
    snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
    sW = u8g2.getStrWidth(buf);
    ascent = u8g2.getAscent();

    u8g2.setFontPosTop();

    if (buf[0] == char(49)) // number 1
    {
        x = (dW - sW - 10) / 2;
        y = (dH - ascent) / 2;
    }
    else
    {
        x = (dW - sW) / 2;
        y = (dH - ascent) / 2;
    }

    u8g2.setCursor(x, y);

    u8g2.print(buf);

    bool blinkColon = true;
    if (blinkColon)
    {
        if (!state500ms)
        {
            u8g2.setCursor(x, y);
            snprintf(buf, sizeof(buf), "%d", hr);
            u8g2.print(buf);
            u8g2.setFontMode(1);
            u8g2.setDrawColor(0);
            u8g2.print(":");
            u8g2.setFontMode(0);  // default
            u8g2.setDrawColor(1); // default
        }
    }
}

void u8g2_string5(uint16_t a)
{
    tm *tm = localtime(&now);

    uint8_t hr, min /*, sec*/;
    hr = tm->tm_hour;
    min = tm->tm_min;
    // sec = tm->tm_sec;

    char buf[36];

    const uint8_t *fontC;

    // fontC = u8g2_font_6x10_mn;
    // fontC = u8g2_font_smart_patrol_nbp_tn;
    // fontC = u8g2_font_profont15_mn;
    // fontC = u8g2_font_px437wyse700b_mn;
    // fontC = u8g2_font_profont17_mn;
    // fontC = u8g2_font_t0_18_me;
    // fontC = u8g2_font_balthasar_titling_nbp_tn;
    // fontC = u8g2_font_logisoso16_tn;
    fontC = u8g2_font_logisoso42_tn;
    // fontC = u8g2_font_logisoso46_tn;
    // fontC = u8g2_font_inb33_mn;
    // fontC = u8g2_font_inr33_mn;

    u8g2.setFont(fontC);

    bool format12 = true;
    if (format12)
    {
        if (hr == 0)
        {
            hr = 12;
        }

        if (hr > 12)
        {
            hr = hr - 12;
        }
    }

    int dW;
    // int dH;
    // const char *str;
    int sW;
    // int ascent;

    int x, y;

    dW = u8g2.getDisplayWidth();
    snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
    sW = u8g2.getStrWidth(buf);

    u8g2.setFontPosTop();

    if (buf[0] == char(49)) // number 1
    {
        x = (dW - sW - 8) / 2;
        y = 0;
    }
    else
    {
        x = (dW - sW) / 2;
        y = 0;
    }

    u8g2.setCursor(x, y);

    u8g2.print(buf);

    bool blinkColon = true;
    if (blinkColon)
    {
        if (!state500ms)
        {
            u8g2.setCursor(x, y);
            snprintf(buf, sizeof(buf), "%d", hr);
            u8g2.print(buf);
            u8g2.setFontMode(1);
            u8g2.setDrawColor(0);
            u8g2.print(":");
            u8g2.setFontMode(1);  // default
            u8g2.setDrawColor(1); // default
        }
    }

    // snprintf(buf, sizeof(buf), "%s %s", sholatNameStr(NEXTTIMEID), sholatTimeArray[NEXTTIMEID]);

    if (!state500ms)
    {
        snprintf(buf, sizeof(buf), "%s  %dh %2dm", sholatNameStr(NEXTTIMEID), ceilHOUR, ceilMINUTE);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%s  %dh %2dm", sholatNameStr(NEXTTIMEID), ceilHOUR, ceilMINUTE);
    }

    u8g2.setFontPosBottom();

    // fontC = u8g2_font_smart_patrol_nbp_tr;
    // fontC = u8g2_font_8x13_tr;
    fontC = u8g2_font_7x13_tr;

    u8g2.setFont(fontC);

    dW = u8g2.getDisplayWidth();
    sW = u8g2.getStrWidth(buf);

    x = (dW - sW) / 2;
    y = 64;
    u8g2.setCursor(x, y);

    // u8g2.print(buf);

    x = 0;
    y = 64;
    u8g2.setCursor(x, y);
    u8g2.print(sholatNameStr(NEXTTIMEID));

    snprintf(buf, sizeof(buf), "%dh %2dm", ceilHOUR, ceilMINUTE);
    dW = u8g2.getDisplayWidth();
    sW = u8g2.getStrWidth(buf);
    x = (dW - sW);
    y = 64;
    u8g2.setCursor(x, y);
    u8g2.print(buf);
}

void u8g2_string6(uint16_t a)
{
    tm *tm = localtime(&now);

    uint8_t hr, min /*, sec*/;
    hr = tm->tm_hour;
    min = tm->tm_min;
    // sec = tm->tm_sec;

    char buf[36];

    const uint8_t *fontC;

    // fontC = u8g2_font_6x10_mn;
    // fontC = u8g2_font_smart_patrol_nbp_tn;
    // fontC = u8g2_font_profont15_mn;
    // fontC = u8g2_font_px437wyse700b_mn;
    // fontC = u8g2_font_profont17_mn;
    // fontC = u8g2_font_t0_18_me;
    // fontC = u8g2_font_balthasar_titling_nbp_tn;
    // fontC = u8g2_font_logisoso16_tn;
    fontC = u8g2_font_logisoso42_tn;
    // fontC = u8g2_font_logisoso46_tn;
    // fontC = u8g2_font_inb33_mn;
    // fontC = u8g2_font_inr33_mn;

    u8g2.setFont(fontC);

    bool format12 = true;
    if (format12)
    {
        if (hr == 0)
        {
            hr = 12;
        }

        if (hr > 12)
        {
            hr = hr - 12;
        }
    }

    int dW;
    // int dH;
    // const char *str;
    int sW;
    // int ascent;

    int x, y;

    dW = u8g2.getDisplayWidth();
    snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
    sW = u8g2.getStrWidth(buf);

    u8g2.setFontPosTop();

    if (buf[0] == char(49)) // number 1
    {
        x = (dW - sW - 8) / 2;
        y = 0;
    }
    else
    {
        x = (dW - sW) / 2;
        y = 0;
    }

    u8g2.setCursor(x + a, y);

    u8g2.print(buf);

    bool blinkColon = true;
    if (blinkColon)
    {
        if (!state500ms)
        {
            u8g2.setCursor(x + a, y);
            snprintf(buf, sizeof(buf), "%d", hr);
            u8g2.print(buf);
            u8g2.setFontMode(1);
            u8g2.setDrawColor(0);
            u8g2.print(":");
            u8g2.setFontMode(1);  // default
            u8g2.setDrawColor(1); // default
        }
    }

    // snprintf(buf, sizeof(buf), "%s %s", sholatNameStr(NEXTTIMEID), sholatTimeArray[NEXTTIMEID]);

    if (!state500ms)
    {
        snprintf(buf, sizeof(buf), "%s  %dh %2dm", sholatNameStr(NEXTTIMEID), ceilHOUR, ceilMINUTE);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%s  %dh %2dm", sholatNameStr(NEXTTIMEID), ceilHOUR, ceilMINUTE);
    }

    u8g2.setFontPosBottom();

    // fontC = u8g2_font_smart_patrol_nbp_tr;
    // fontC = u8g2_font_8x13_tr;
    fontC = u8g2_font_7x13_tr;

    u8g2.setFont(fontC);

    dW = u8g2.getDisplayWidth();
    sW = u8g2.getStrWidth(buf);

    x = (dW - sW) / 2;
    y = 64;
    u8g2.setCursor(x, y);

    // u8g2.print(buf);

    x = 0;
    y = 64;
    u8g2.setCursor(x, y);
    u8g2.print(sholatNameStr(NEXTTIMEID));

    snprintf(buf, sizeof(buf), "%dh %2dm", ceilHOUR, ceilMINUTE);
    dW = u8g2.getDisplayWidth();
    sW = u8g2.getStrWidth(buf);
    x = (dW - sW);
    y = 64;
    u8g2.setCursor(x, y);
    u8g2.print(buf);
}

void u8g2_string7(uint16_t a)
{
    tm *tm = localtime(&now);

    uint8_t hr, min /*, sec*/;
    hr = tm->tm_hour;
    min = tm->tm_min;
    // sec = tm->tm_sec;

    char buf[36];

    const uint8_t *fontC;

    // fontC = u8g2_font_6x10_mn;
    fontC = u8g2_font_smart_patrol_nbp_tn;
    // fontC = u8g2_font_profont15_mn;
    // fontC = u8g2_font_px437wyse700b_mn;
    // fontC = u8g2_font_profont17_mn;
    // fontC = u8g2_font_t0_18_me;
    // fontC = u8g2_font_balthasar_titling_nbp_tn;
    // fontC = u8g2_font_logisoso16_tn;
    // fontC = u8g2_font_logisoso42_tn;
    // fontC = u8g2_font_logisoso46_tn;
    // fontC = u8g2_font_inb33_mn;
    // fontC = u8g2_font_inr33_mn;

    u8g2.setFont(fontC);

    bool format12 = true;
    if (format12)
    {
        if (hr == 0)
        {
            hr = 12;
        }

        if (hr > 12)
        {
            hr = hr - 12;
        }
    }

    int dW;
    // int dH;
    // const char *str;
    int sW;
    // int ascent;

    int x, y;

    dW = u8g2.getDisplayWidth();
    snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
    sW = u8g2.getStrWidth(buf);

    u8g2.setFontPosTop();

    x = (dW - sW);
    y = 0;

    u8g2.setCursor(x + 0, y);

    u8g2.print(buf);

    // u8g2_2.setFontMode(0);  // default
    // u8g2_2.setDrawColor(1); // default
    u8g2_2.setFontPosTop();
    u8g2_2.setFont(fontC);
    u8g2_2.setCursor(x - 0, y);
    snprintf(buf, sizeof(buf), "963");
    // u8g2_2.print(buf);

    bool blinkColon = true;
    if (blinkColon)
    {
        if (!state500ms)
        {
            u8g2.setCursor(x + 0, y);
            snprintf(buf, sizeof(buf), "%d", hr);
            u8g2.print(buf);
            u8g2.setFontMode(1);
            u8g2.setDrawColor(0);
            u8g2.print(":");
            u8g2.setFontMode(1);  // default
            u8g2.setDrawColor(1); // default
        }
    }

    // print next sholat time
    u8g2.setFontPosTop();
    const uint8_t *font;
    // font = u8g2_font_logisoso22_tn;
    // font = u8g2_font_inb21_mr;
    font = u8g2_font_profont29_mn;
    // font = u8g2_font_freedoomr25_mn;
    u8g2.setFont(font);

    bool blinkNegativeSign = true;
    if (blinkNegativeSign)
    {
        if (!state500ms)
        {
            if (HOUR != 0)
            {
                snprintf(buf, sizeof(buf), " %d:%02d:%02d", HOUR, MINUTE, SECOND);
            }
            else
            {
                snprintf(buf, sizeof(buf), " %d:%02d", MINUTE, SECOND);
            }
        }
        else
        {
            if (HOUR != 0)
            {
                snprintf(buf, sizeof(buf), "-%d:%02d:%02d", HOUR, MINUTE, SECOND);
            }
            else
            {
                snprintf(buf, sizeof(buf), "-%d:%02d", MINUTE, SECOND);
            }
        }
    }
    dW = u8g2.getDisplayWidth();
    sW = u8g2.getStrWidth(buf);
    x = (dW - sW) / 2;
    y = 16;
    u8g2.setCursor(x, y);
    u8g2.print(buf);

    // print next sholat name
    u8g2.setFontPosBottom();
    // font = u8g2_font_7x13_tr;
    font = u8g2_font_profont22_mr;
    u8g2.setFont(font);
    snprintf(buf, sizeof(buf), "to %s", sholatNameStr(NEXTTIMEID));
    dW = u8g2.getDisplayWidth();
    sW = u8g2.getStrWidth(buf);
    x = (dW - sW) / 2;
    y = 64;
    u8g2.setCursor(x, y);
    u8g2.print(buf);
}

void u8g2_string8(uint16_t a)
{
    tm *tm = localtime(&now);

    uint8_t hr, min /*, sec*/;
    hr = tm->tm_hour;
    min = tm->tm_min;
    // sec = tm->tm_sec;

    char buf[36];

    const uint8_t *font;

    // font = u8g2_font_6x10_mn;
    font = u8g2_font_smart_patrol_nbp_tn;
    // font = u8g2_font_profont15_mn;
    // font = u8g2_font_px437wyse700b_mn;
    // font = u8g2_font_profont17_mn;
    // font = u8g2_font_t0_18_me;
    // font = u8g2_font_balthasar_titling_nbp_tn;
    // font = u8g2_font_logisoso16_tn;
    // font = u8g2_font_logisoso42_tn;
    // font = u8g2_font_logisoso46_tn;
    // font = u8g2_font_inb33_mn;
    // font = u8g2_font_inr33_mn;
    font = u8g2_font_freedoomr10_mu;

    u8g2.setFont(font);

    bool format12 = false;
    if (format12)
    {
        if (hr == 0)
        {
            hr = 12;
        }

        if (hr > 12)
        {
            hr = hr - 12;
        }
    }

    int dW;
    // int dH;
    // const char *str;
    int sW;
    // int ascent;

    int x, y;

    dW = u8g2.getDisplayWidth();
    snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
    sW = u8g2.getStrWidth(buf);

    u8g2.setFontPosTop();

    x = (dW - sW);
    y = 0;

    u8g2.setCursor(x + 0, y);

    u8g2.print(buf);

    // u8g2_2.setFontMode(0);  // default
    // u8g2_2.setDrawColor(1); // default
    u8g2_2.setFontPosTop();
    u8g2_2.setFont(font);
    u8g2_2.setCursor(x - 0, y);
    snprintf(buf, sizeof(buf), "963");
    // u8g2_2.print(buf);

    bool blinkColon = true;
    if (blinkColon)
    {
        if (!state500ms)
        {
            u8g2.setCursor(x + 0, y);
            snprintf(buf, sizeof(buf), "%d", hr);
            u8g2.print(buf);
            u8g2.setFontMode(1);
            u8g2.setDrawColor(0);
            u8g2.print(":");
            u8g2.setFontMode(1);  // default
            u8g2.setDrawColor(1); // default
        }
    }

    // print next sholat time
    u8g2.setFontPosTop();
    // font = u8g2_font_logisoso22_tn;
    // font = u8g2_font_inb21_mr;
    font = u8g2_font_profont29_mr;
    // font = u8g2_font_freedoomr25_mn;
    u8g2.setFont(font);

    snprintf(buf, sizeof(buf), "%dh %dm", ceilHOUR, ceilMINUTE);
    dW = u8g2.getDisplayWidth();
    sW = u8g2.getStrWidth(buf);
    x = (dW - sW) / 2;
    y = 16;
    u8g2.setCursor(x, y);
    u8g2.print(buf);

    // print next sholat name
    u8g2.setFontPosBottom();
    // font = u8g2_font_7x13_tr;
    font = u8g2_font_profont22_mr;
    u8g2.setFont(font);
    snprintf(buf, sizeof(buf), "to %s", sholatNameStr(NEXTTIMEID));
    dW = u8g2.getDisplayWidth();
    sW = u8g2.getStrWidth(buf);
    x = (dW - sW) / 2;
    y = 64;
    u8g2.setCursor(x, y);
    u8g2.print(buf);
}

void u8g2_string9(uint16_t a)
{
    tm *tm = localtime(&now);

    uint8_t hr, min /*, sec*/;
    hr = tm->tm_hour;
    min = tm->tm_min;
    // sec = tm->tm_sec;

    char buf[36];

    const uint8_t *font;

    // font = u8g2_font_6x10_mn;
    // font = u8g2_font_smart_patrol_nbp_tn;
    // font = u8g2_font_profont15_mn;
    // font = u8g2_font_px437wyse700b_mn;
    // font = u8g2_font_profont17_mn;
    // font = u8g2_font_t0_18_me;
    // font = u8g2_font_balthasar_titling_nbp_tn;
    // font = u8g2_font_logisoso16_tn;
    // font = u8g2_font_logisoso42_tn;
    // font = u8g2_font_logisoso46_tn;
    // font = u8g2_font_inb33_mn;
    // font = u8g2_font_inr33_mn;
    font = u8g2_font_freedoomr10_mu;

    u8g2.setFont(font);

    bool format12 = false;
    if (format12)
    {
        if (hr == 0)
        {
            hr = 12;
        }

        if (hr > 12)
        {
            hr = hr - 12;
        }
    }

    int dW;
    // int dH;
    // const char *str;
    int sW;
    // int ascent;

    int x, y;

    dW = u8g2.getDisplayWidth();
    snprintf(buf, sizeof(buf), "%d:%02d", hr, min);
    sW = u8g2.getStrWidth(buf);

    u8g2.setFontPosTop();

    x = (dW - sW);
    y = 0;

    u8g2.setCursor(x + 0, y);

    u8g2.print(buf);

    // u8g2_2.setFontMode(0);  // default
    // u8g2_2.setDrawColor(1); // default
    u8g2_2.setFontPosTop();
    u8g2_2.setFont(font);
    u8g2_2.setCursor(x - 0, y);
    snprintf(buf, sizeof(buf), "963");
    // u8g2_2.print(buf);

    bool blinkColon = true;
    if (blinkColon)
    {
        if (!state500ms)
        {
            u8g2.setCursor(x + 0, y);
            snprintf(buf, sizeof(buf), "%d", hr);
            u8g2.print(buf);
            u8g2.setFontMode(1);
            u8g2.setDrawColor(0);
            u8g2.print(":");
            u8g2.setFontMode(1);  // default
            u8g2.setDrawColor(1); // default
        }
    }

    // print abbreviated weekday name
    u8g2_2.setFontPosTop();
    font = u8g2_font_smart_patrol_nbp_tr;
    u8g2.setFont(font);
    strftime(buf, sizeof(buf), "%a", tm);
    dW = u8g2.getDisplayWidth();
    sW = u8g2.getStrWidth(buf);
    x = (dW - sW);
    y = u8g2.getAscent() + 6;
    u8g2.setCursor(x, y);
    u8g2.print(buf);

    // print next sholat time
    u8g2.setFontPosTop();
    // font = u8g2_font_logisoso22_tn;
    // font = u8g2_font_inb21_mr;
    font = u8g2_font_profont22_mr;
    // font = u8g2_font_freedoomr25_mn;
    // font = u8g2_font_osb21_tr;
    u8g2.setFont(font);

    snprintf(buf, sizeof(buf), "%2d hr\r\n", ceilHOUR);
    dW = u8g2.getDisplayWidth();
    sW = u8g2.getStrWidth(buf);
    x = 0;
    y = 0;
    u8g2.setCursor(x, y);
    u8g2.print(buf);
    snprintf(buf, sizeof(buf), "%2d min\r\n", ceilMINUTE);
    x = 0;
    y = 21;
    u8g2.setCursor(x, y);
    u8g2.print(buf);

    // print next sholat name
    u8g2.setFontPosBottom();
    // font = u8g2_font_7x13_tr;
    font = u8g2_font_profont22_mr;
    u8g2.setFont(font);
    snprintf(buf, sizeof(buf), "to %s", sholatNameStr(NEXTTIMEID));
    dW = u8g2.getDisplayWidth();
    sW = u8g2.getStrWidth(buf);
    x = 0;
    y = 63;
    u8g2.setCursor(x, y);
    u8g2.print(buf);
}

void u8g2_line(uint8_t a)
{
    u8g2.drawStr(0, 0, "drawLine");
    u8g2.drawLine(7 + a, 10, 40, 55);
    u8g2.drawLine(7 + a * 2, 10, 60, 55);
    u8g2.drawLine(7 + a * 3, 10, 80, 55);
    u8g2.drawLine(7 + a * 4, 10, 100, 55);
}

void u8g2_triangle(uint8_t a)
{
    uint16_t offset = a;
    u8g2.drawStr(0, 0, "drawTriangle");
    u8g2.drawTriangle(14, 7, 45, 30, 10, 40);
    u8g2.drawTriangle(14 + offset, 7 - offset, 45 + offset, 30 - offset, 57 + offset, 10 - offset);
    u8g2.drawTriangle(57 + offset * 2, 10, 45 + offset * 2, 30, 86 + offset * 2, 53);
    u8g2.drawTriangle(10 + offset, 40 + offset, 45 + offset, 30 + offset, 86 + offset, 53 + offset);
}

void u8g2_ascii_1()
{
    char s[2] = " ";
    uint8_t x, y;
    u8g2.drawStr(0, 0, "ASCII page 1");
    for (y = 0; y < 6; y++)
    {
        for (x = 0; x < 16; x++)
        {
            s[0] = y * 16 + x + 32;
            u8g2.drawStr(x * 7, y * 10 + 10, s);
        }
    }
}

void u8g2_ascii_2()
{
    char s[2] = " ";
    uint8_t x, y;
    u8g2.drawStr(0, 0, "ASCII page 2");
    for (y = 0; y < 6; y++)
    {
        for (x = 0; x < 16; x++)
        {
            s[0] = y * 16 + x + 160;
            u8g2.drawStr(x * 7, y * 10 + 10, s);
        }
    }
}

void u8g2_extra_page(uint8_t a)
{
    u8g2.drawStr(0, 0, "Unicode");
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.setFontPosTop();
    u8g2.drawUTF8(0, 24, "☀ ☁");
    switch (a)
    {
    case 0:
    case 1:
    case 2:
    case 3:
        u8g2.drawUTF8(a * 3, 36, "☂");
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        u8g2.drawUTF8(a * 3, 36, "☔");
        break;
    }
}

#define cross_width 24
#define cross_height 24
static const unsigned char cross_bits[] U8X8_PROGMEM = {
    0x00,
    0x18,
    0x00,
    0x00,
    0x24,
    0x00,
    0x00,
    0x24,
    0x00,
    0x00,
    0x42,
    0x00,
    0x00,
    0x42,
    0x00,
    0x00,
    0x42,
    0x00,
    0x00,
    0x81,
    0x00,
    0x00,
    0x81,
    0x00,
    0xC0,
    0x00,
    0x03,
    0x38,
    0x3C,
    0x1C,
    0x06,
    0x42,
    0x60,
    0x01,
    0x42,
    0x80,
    0x01,
    0x42,
    0x80,
    0x06,
    0x42,
    0x60,
    0x38,
    0x3C,
    0x1C,
    0xC0,
    0x00,
    0x03,
    0x00,
    0x81,
    0x00,
    0x00,
    0x81,
    0x00,
    0x00,
    0x42,
    0x00,
    0x00,
    0x42,
    0x00,
    0x00,
    0x42,
    0x00,
    0x00,
    0x24,
    0x00,
    0x00,
    0x24,
    0x00,
    0x00,
    0x18,
    0x00,
};

#define cross_fill_width 24
#define cross_fill_height 24
static const unsigned char cross_fill_bits[] U8X8_PROGMEM = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x18,
    0x00,
    0x18,
    0x64,
    0x00,
    0x26,
    0x84,
    0x00,
    0x21,
    0x08,
    0x81,
    0x10,
    0x08,
    0x42,
    0x10,
    0x10,
    0x3C,
    0x08,
    0x20,
    0x00,
    0x04,
    0x40,
    0x00,
    0x02,
    0x80,
    0x00,
    0x01,
    0x80,
    0x18,
    0x01,
    0x80,
    0x18,
    0x01,
    0x80,
    0x00,
    0x01,
    0x40,
    0x00,
    0x02,
    0x20,
    0x00,
    0x04,
    0x10,
    0x3C,
    0x08,
    0x08,
    0x42,
    0x10,
    0x08,
    0x81,
    0x10,
    0x84,
    0x00,
    0x21,
    0x64,
    0x00,
    0x26,
    0x18,
    0x00,
    0x18,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

#define cross_block_width 14
#define cross_block_height 14
static const unsigned char cross_block_bits[] U8X8_PROGMEM = {
    0xFF,
    0x3F,
    0x01,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0xC1,
    0x20,
    0xC1,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0x01,
    0x20,
    0xFF,
    0x3F,
};

void u8g2_bitmap_overlay(uint8_t a)
{
    uint8_t frame_size = 28;

    u8g2.drawStr(0, 0, "Bitmap overlay");

    u8g2.drawStr(0, frame_size + 12, "Solid / transparent");
    u8g2.setBitmapMode(false /* solid */);
    u8g2.drawFrame(0, 10, frame_size, frame_size);
    u8g2.drawXBMP(2, 12, cross_width, cross_height, cross_bits);
    if (a & 4)
        u8g2.drawXBMP(7, 17, cross_block_width, cross_block_height, cross_block_bits);

    u8g2.setBitmapMode(true /* transparent*/);
    u8g2.drawFrame(frame_size + 5, 10, frame_size, frame_size);
    u8g2.drawXBMP(frame_size + 7, 12, cross_width, cross_height, cross_bits);
    if (a & 4)
        u8g2.drawXBMP(frame_size + 12, 17, cross_block_width, cross_block_height, cross_block_bits);
}

void u8g2_bitmap_modes(uint8_t transparent)
{
    const uint8_t frame_size = 24;

    u8g2.drawBox(0, frame_size * 0.5, frame_size * 5, frame_size);
    u8g2.drawStr(frame_size * 0.5, 50, "Black");
    u8g2.drawStr(frame_size * 2, 50, "White");
    u8g2.drawStr(frame_size * 3.5, 50, "XOR");

    if (!transparent)
    {
        u8g2.setBitmapMode(false /* solid */);
        u8g2.drawStr(0, 0, "Solid bitmap");
    }
    else
    {
        u8g2.setBitmapMode(true /* transparent*/);
        u8g2.drawStr(0, 0, "Transparent bitmap");
    }
    u8g2.setDrawColor(0); // Black
    u8g2.drawXBMP(frame_size * 0.5, 24, cross_width, cross_height, cross_bits);
    u8g2.setDrawColor(1); // White
    u8g2.drawXBMP(frame_size * 2, 24, cross_width, cross_height, cross_bits);
    u8g2.setDrawColor(2); // XOR
    u8g2.drawXBMP(frame_size * 3.5, 24, cross_width, cross_height, cross_bits);
}

uint8_t draw_state = 24;

// void draw(void)
// {
//   u8g2_prepare();
//   switch (draw_state >> 3)
//   {
//   case 0:
//     u8g2_box_frame(draw_state & 7);
//     break;
//   case 1:
//     u8g2_disc_circle(draw_state & 7);
//     break;
//   case 2:
//     u8g2_r_frame(draw_state & 7);
//     break;
//   case 3:
//     u8g2_string(draw_state & 7);
//     break;
//   case 4:
//     u8g2_line(draw_state & 7);
//     break;
//   case 5:
//     u8g2_triangle(draw_state & 7);
//     break;
//   case 6:
//     u8g2_ascii_1();
//     break;
//   case 7:
//     u8g2_ascii_2();
//     break;
//   case 8:
//     u8g2_extra_page(draw_state & 7);
//     break;
//   case 9:
//     u8g2_bitmap_modes(0);
//     break;
//   case 10:
//     u8g2_bitmap_modes(1);
//     break;
//   case 11:
//     u8g2_bitmap_overlay(draw_state & 7);
//     break;
//   }
// }

void DisplaySetup()
{
    u8g2_2.begin();
    u8g2_2.clear();
    u8g2.begin();
    u8g2.clear();
}

void DisplayLoop()
{

    // picture loop
    // must be after sholat time has been updated
    // u8g2.clearBuffer();
    u8g2_2.clearBuffer();

    static uint16_t x = 0;
    // u8g2_prepare();
    // u8g2_string(x);
    // u8g2_string2(x);
    // u8g2_string3(x);
    // u8g2_string4(x);
    // u8g2_string5(x);
    // u8g2_string6(0);
    // u8g2_string7(0);
    u8g2_string8(0);
    // u8g2_string9(0);
    x = x - 84;
    if (x <= -127)
    {
        x = 0;
    }

    u8g2_2.sendBuffer();
}