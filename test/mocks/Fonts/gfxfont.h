// Common GFXfont definition for mocks
#ifndef MOCK_GFXFONT_H
#define MOCK_GFXFONT_H

#ifdef UNIT_TEST

// Mock Adafruit GFX font structure
typedef struct {
  uint8_t *bitmap;
  void *glyph;
  uint8_t first;
  uint8_t last;
  uint8_t yAdvance;
} GFXfont;

#endif  // UNIT_TEST

#endif  // MOCK_GFXFONT_H
