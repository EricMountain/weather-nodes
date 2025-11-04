// Mock FreeSans24pt7b.h for native testing
#ifndef MOCK_FREESANS24PT7B_H
#define MOCK_FREESANS24PT7B_H

#ifdef UNIT_TEST

#include "gfxfont.h"

// Mock font data
static const GFXfont FreeSans24pt7b = {nullptr, nullptr, 0, 0, 0};

#endif  // UNIT_TEST

#endif  // MOCK_FREESANS24PT7B_H
