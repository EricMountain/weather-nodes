// Mock moon_phases_48pt.h for native testing
#ifndef MOCK_MOON_PHASES_48PT_H
#define MOCK_MOON_PHASES_48PT_H

#ifdef UNIT_TEST

// Define U8G2_FONT_SECTION if not defined
#ifndef U8G2_FONT_SECTION
#define U8G2_FONT_SECTION(name)
#endif

// Mock moon phases font
static const uint8_t moon_phases_48pt_data[] = {0};
static const uint8_t* moon_phases_48pt = moon_phases_48pt_data;

#endif  // UNIT_TEST

#endif  // MOCK_MOON_PHASES_48PT_H
