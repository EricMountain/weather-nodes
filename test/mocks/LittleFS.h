// Mock LittleFS.h for native testing
#ifndef MOCK_LITTLEFS_H
#define MOCK_LITTLEFS_H

#ifdef UNIT_TEST

#include "Arduino.h"
#include <string>
#include <map>

// Forward declaration
class LittleFSClass;

// Mock File class
class File {
 public:
  File() : valid_(false), data_(""), pos_(0) {}
  explicit File(bool valid, const std::string& data = "") : valid_(valid), data_(data), pos_(0) {}
  
  operator bool() const { return valid_; }
  bool available() const { return pos_ < data_.size(); }
  int read() { 
    if (pos_ < data_.size()) {
      return data_[pos_++];
    }
    return -1; 
  }
  size_t write(uint8_t) { return 0; }
  size_t write(const uint8_t* buf, size_t size) { return size; }
  size_t print(const char* str) { return 0; }
  void close() {}
  
  File openNextFile() { return File(false); }
  const char* name() const { return "mock_file"; }
  size_t size() const { return data_.size(); }
  
 private:
  bool valid_;
  std::string data_;
  mutable size_t pos_;
};

// Mock LittleFS class
class LittleFSClass {
 public:
  LittleFSClass() {
    // Pre-populate with a valid JSON for testing
    files_["/last-displayed.json"] = "{}";
  }
  
  bool begin(bool formatOnFail = false, const char * basePath = "/littlefs", uint8_t maxOpenFiles = 10, const char * partitionLabel = "spiffs") {
    return true;
  }
  
  void end() {}
  
  bool exists(const char* path) {
    return files_.find(path) != files_.end();
  }
  
  File open(const char* path, const char* mode = "r", bool create = false) {
    if (mode[0] == 'r') {
      if (exists(path)) {
        return File(true, files_[path]);
      }
      return File(false);
    }
    if (mode[0] == 'w') {
      // Create file entry
      files_[path] = "";
    }
    return File(true);
  }
  
  bool remove(const char* path) {
    if (exists(path)) {
      files_.erase(path);
      return true;
    }
    return false;
  }
  
  bool mkdir(const char* path) { return true; }
  bool rmdir(const char* path) { return true; }
  
 private:
  std::map<std::string, std::string> files_;
};

// Global instance
static LittleFSClass LittleFS;

#endif  // UNIT_TEST

#endif  // MOCK_LITTLEFS_H
