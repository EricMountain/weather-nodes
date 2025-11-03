// Mock U8g2_for_Adafruit_GFX.h for native testing
#ifndef MOCK_U8G2_FOR_ADAFRUIT_GFX_H
#define MOCK_U8G2_FOR_ADAFRUIT_GFX_H

#ifdef UNIT_TEST

#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

// Mock u8g2 font type
typedef const uint8_t* u8g2_font_t;

// Mock font definitions - just use nullptr for testing
static const uint8_t u8g2_font_inb38_mf_data[] = {0};
static const uint8_t u8g2_font_inb24_mf_data[] = {0};
static const uint8_t u8g2_font_inb16_mf_data[] = {0};
static const uint8_t* u8g2_font_inb38_mf = u8g2_font_inb38_mf_data;
static const uint8_t* u8g2_font_inb24_mf = u8g2_font_inb24_mf_data;
static const uint8_t* u8g2_font_inb16_mf = u8g2_font_inb16_mf_data;

// Mock U8G2_FOR_ADAFRUIT_GFX class
class U8G2_FOR_ADAFRUIT_GFX {
 public:
  U8G2_FOR_ADAFRUIT_GFX() 
      : cursor_x_(0), 
        cursor_y_(0),
        font_mode_(0),
        font_direction_(0),
        foreground_color_(0x0000),
        background_color_(0xFFFF),
        current_font_(nullptr) {}

  template<typename T>
  void begin(T& display) {
    // Mock begin - store reference if needed
    display_initialized_ = true;
  }

  void setFont(const uint8_t* font) {
    current_font_ = font;
  }

  void setFontMode(uint8_t mode) {
    font_mode_ = mode;
  }

  void setFontDirection(uint8_t direction) {
    font_direction_ = direction;
  }

  void setForegroundColor(uint16_t color) {
    foreground_color_ = color;
  }

  void setBackgroundColor(uint16_t color) {
    background_color_ = color;
  }

  void setCursor(int16_t x, int16_t y) {
    cursor_x_ = x;
    cursor_y_ = y;
  }

  void print(const char* str) {
    output_buffer_ += str;
  }

  void print(char c) {
    output_buffer_ += c;
  }

  void println(const char* str) {
    output_buffer_ += str;
    output_buffer_ += '\n';
  }

  void printf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    output_buffer_ += buffer;
  }

  uint16_t getUTF8Width(const char* str) {
    // Mock: return simple character count * 10 as approximate width
    if (!str) return 0;
    uint16_t count = 0;
    while (*str) {
      count++;
      str++;
    }
    return count * 10;
  }

  // Mock methods to access internal state for testing
  int16_t getCursorX() const { return cursor_x_; }
  int16_t getCursorY() const { return cursor_y_; }
  uint16_t getForegroundColor() const { return foreground_color_; }
  uint16_t getBackgroundColor() const { return background_color_; }
  const uint8_t* getCurrentFont() const { return current_font_; }
  const std::string& getOutputBuffer() const { return output_buffer_; }
  void clearOutputBuffer() { output_buffer_.clear(); }
  bool isInitialized() const { return display_initialized_; }

 private:
  int16_t cursor_x_;
  int16_t cursor_y_;
  uint8_t font_mode_;
  uint8_t font_direction_;
  uint16_t foreground_color_;
  uint16_t background_color_;
  const uint8_t* current_font_;
  std::string output_buffer_;
  bool display_initialized_ = false;
};

#endif  // UNIT_TEST

#endif  // MOCK_U8G2_FOR_ADAFRUIT_GFX_H
