// Mock Arduino.h for native testing
#ifndef ARDUINO_H
#define ARDUINO_H

#ifdef UNIT_TEST

#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <string>

#include <math.h>

// Define U8G2_FONT_SECTION for fonts
#ifndef U8G2_FONT_SECTION
#define U8G2_FONT_SECTION(name)
#endif

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define radians(deg) ((deg) * DEG_TO_RAD)
#define degrees(rad) ((rad) * RAD_TO_DEG)

#define F(string_literal) (string_literal)

// Basic Arduino types
typedef uint8_t byte;

// Mock String class for native testing
class String : public std::string {
 public:
  using std::string::string;  // Inherit constructors

  // Constructor from std::string
  String(const std::string& str) : std::string(str) {}

  // Constructor from const char*
  String(const char* str) : std::string(str ? str : "") {}

  // Default constructor
  String() : std::string() {}

  double toDouble() const { return std::stod(*this); }

  const char* c_str() const { return std::string::c_str(); }
};

// Mock Serial for tests
class MockSerial {
 public:
  void println(const char* str) { printf("%s\n", str); }

  void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
  }
};

// Create a static instance in each translation unit that includes this
static MockSerial Serial;

#endif  // UNIT_TEST

#endif  // ARDUINO_H
