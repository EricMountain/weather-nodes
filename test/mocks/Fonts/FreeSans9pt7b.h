// Mock FreeSans9pt7b.h for native testing
#ifndef MOCK_FREESANS9PT7B_H
#define MOCK_FREESANS9PT7B_H

#ifdef UNIT_TEST

#include "gfxfont.h"

// Mock font data
static const GFXfont FreeSans9pt7b = {nullptr, nullptr, 0, 0, 0};

#endif  // UNIT_TEST

#endif  // MOCK_FREESANS9PT7B_H
