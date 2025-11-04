# Makefile for running tests locally without PlatformIO
# This is a fallback when PlatformIO native platform cannot be installed

CXX = g++
CXXFLAGS = -std=c++11 -include ./mocks/Arduino.h -I ../lib/datetime -I ../lib/model -I ../lib/config -I ../lib/sunandmoon -I ../lib/SunMoonCalc -I ../src -I ../src/views -I ../src/fonts -I ./mocks -I ./mocks/fonts -I ./mocks/Fonts -I ../.pio/libdeps/native/ArduinoJson/src -I ../.pio/libdeps/native/fmt/include -D UNIT_TEST -D FMT_HEADER_ONLY
LDFLAGS =

# Unity framework (embedded in PlatformIO)
UNITY_SRC = ~/.platformio/packages/framework-unity/unity.c
UNITY_INC = -I ~/.platformio/packages/framework-unity

# Source files
SRC_DIR = ../src
LIB_DIR = ../lib
TEST_DIR = .
MOCK_DIR = ./mocks

# Common sources needed for tests
COMMON_SRCS =

# DateTime test
DATETIME_SRCS = $(LIB_DIR)/datetime/datetime.cpp
DATETIME_TEST = $(TEST_DIR)/test_datetime/test_datetime.cpp
DATETIME_BIN = test_datetime_bin

# Model test
MODEL_SRCS = $(LIB_DIR)/model/model.cpp $(LIB_DIR)/datetime/datetime.cpp $(LIB_DIR)/SunMoonCalc/SunMoonCalc.cpp
MODEL_TEST = $(TEST_DIR)/test_model/test_model.cpp
MODEL_BIN = test_model_bin

# EPDView2 test
EPDVIEW2_SRCS = $(SRC_DIR)/views/epd_view_2.cpp $(SRC_DIR)/views/display_view.cpp $(SRC_DIR)/controller.cpp $(LIB_DIR)/model/model.cpp $(LIB_DIR)/datetime/datetime.cpp $(LIB_DIR)/SunMoonCalc/SunMoonCalc.cpp
EPDVIEW2_TEST = $(TEST_DIR)/test_epd_view_2/test_epd_view_2.cpp
EPDVIEW2_BIN = test_epd_view_2_bin

.PHONY: all clean test test_datetime test_model test_epd_view_2

all: test

test: test_datetime test_model test_epd_view_2

test_datetime: $(DATETIME_BIN)
	./$(DATETIME_BIN)

test_model: $(MODEL_BIN)
	./$(MODEL_BIN)

test_epd_view_2: $(EPDVIEW2_BIN)
	./$(EPDVIEW2_BIN)

$(DATETIME_BIN): $(DATETIME_TEST) $(DATETIME_SRCS) $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) $(UNITY_INC) -o $@ $^ $(UNITY_SRC) $(LDFLAGS)

$(MODEL_BIN): $(MODEL_TEST) $(MODEL_SRCS) $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) $(UNITY_INC) -o $@ $^ $(UNITY_SRC) $(LDFLAGS)

$(EPDVIEW2_BIN): $(EPDVIEW2_TEST) $(EPDVIEW2_SRCS) $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) $(UNITY_INC) -o $@ $^ $(UNITY_SRC) $(LDFLAGS)

clean:
	rm -f $(DATETIME_BIN) $(MODEL_BIN) $(EPDVIEW2_BIN)
