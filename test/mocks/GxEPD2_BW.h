// Mock GxEPD2_BW.h for native testing
#ifndef MOCK_GXEPD2_BW_H
#define MOCK_GXEPD2_BW_H

#ifdef UNIT_TEST

#include <cstdint>

// Mock color constants
#define GxEPD_BLACK 0x0000
#define GxEPD_WHITE 0xFFFF

// Mock GxEPD2_750_T7 display driver
class GxEPD2_750_T7 {
 public:
  static const uint16_t WIDTH = 800;
  static const uint16_t HEIGHT = 480;
  static const uint16_t WIDTH_VISIBLE = 800;

  GxEPD2_750_T7(int8_t cs, int8_t dc, int8_t rst, int8_t busy)
      : cs_(cs), dc_(dc), rst_(rst), busy_(busy) {}

 private:
  int8_t cs_;
  int8_t dc_;
  int8_t rst_;
  int8_t busy_;
};

// Mock GxEPD2_BW template class
template <typename GxEPD2_Type, uint16_t page_height>
class GxEPD2_BW {
 public:
  GxEPD2_BW(GxEPD2_Type display)
      : display_(display),
        width_(GxEPD2_Type::WIDTH_VISIBLE),
        height_(GxEPD2_Type::HEIGHT),
        page_index_(0),
        in_page_loop_(false) {}

  void init(uint32_t serial_diag_bitrate = 0, bool initial = true,
            uint16_t reset_duration = 10, bool pulldown_rst_mode = false) {
    // Mock init - do nothing
  }

  void setRotation(uint8_t r) {
    rotation_ = r;
    // For simplicity, don't swap width/height in mock
  }

  void setFullWindow() { full_window_ = true; }

  void setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    full_window_ = false;
    partial_x_ = x;
    partial_y_ = y;
    partial_w_ = w;
    partial_h_ = h;
  }

  void firstPage() {
    page_index_ = 0;
    in_page_loop_ = true;
  }

  bool nextPage() {
    if (!in_page_loop_) {
      return false;
    }
    page_index_++;
    // For mock, simulate only 1 page
    if (page_index_ >= 1) {
      in_page_loop_ = false;
      return false;
    }
    return true;
  }

  uint16_t width() const { return width_; }
  uint16_t height() const { return height_; }

  void hibernate() {
    // Mock hibernate - do nothing
  }

  // Mock methods to access internal state for testing
  bool isInPageLoop() const { return in_page_loop_; }
  bool isFullWindow() const { return full_window_; }
  uint8_t getRotation() const { return rotation_; }

  uint16_t getTextColor() const { return text_color_; }
  void setTextColor(uint16_t color) { text_color_ = color; }

 private:
  GxEPD2_Type display_;
  uint16_t width_;
  uint16_t height_;
  uint8_t rotation_ = 0;
  bool full_window_ = true;
  uint16_t partial_x_ = 0;
  uint16_t partial_y_ = 0;
  uint16_t partial_w_ = 0;
  uint16_t partial_h_ = 0;
  uint16_t page_index_ = 0;
  bool in_page_loop_ = false;
  uint16_t text_color_ = GxEPD_BLACK;
};

#endif  // UNIT_TEST

#endif  // MOCK_GXEPD2_BW_H
