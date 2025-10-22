// Mock Arduino.h for native testing
#ifndef ARDUINO_H
#define ARDUINO_H

#ifdef UNIT_TEST

#include <cstdint>
#include <cstdio>

// Basic Arduino types
typedef uint8_t byte;

// Mock Serial for tests
class MockSerial {
public:
    void println(const char* str) {
        printf("%s\n", str);
    }
    
    void printf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
};

extern MockSerial Serial;

#endif // UNIT_TEST

#endif // ARDUINO_H
