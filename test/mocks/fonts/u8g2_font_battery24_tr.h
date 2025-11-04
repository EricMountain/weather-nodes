// Mock u8g2_font_battery24_tr.h for native testing
#ifndef MOCK_U8G2_FONT_BATTERY24_TR_H
#define MOCK_U8G2_FONT_BATTERY24_TR_H

#ifdef UNIT_TEST

#include "../U8g2_for_Adafruit_GFX.h"

// Define U8G2_FONT_SECTION if not defined
#ifndef U8G2_FONT_SECTION
#define U8G2_FONT_SECTION(name)
#endif

// Mock battery font
static const uint8_t u8g2_font_battery24_tr_data[] = {0};
static const uint8_t* u8g2_font_battery24_tr = u8g2_font_battery24_tr_data;

#endif  // UNIT_TEST

#endif  // MOCK_U8G2_FONT_BATTERY24_TR_H
