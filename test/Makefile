# Makefile for running tests locally without PlatformIO
# This is a fallback when PlatformIO native platform cannot be installed

CXX = g++
CXXFLAGS = -std=c++11 -I src -I test/mocks -I .pio/libdeps/native/ArduinoJson/src -I .pio/libdeps/native/fmt/include -D UNIT_TEST
LDFLAGS = -L .pio/libdeps/native/fmt/build -lfmt

# Unity framework (embedded in PlatformIO)
UNITY_SRC = ~/.platformio/packages/framework-unity/unity.c
UNITY_INC = -I ~/.platformio/packages/framework-unity

# Source files
SRC_DIR = src
TEST_DIR = test
MOCK_DIR = test/mocks

# Common sources needed for tests
COMMON_SRCS = $(MOCK_DIR)/Arduino.cpp

# DateTime test
DATETIME_SRCS = $(SRC_DIR)/datetime.cpp
DATETIME_TEST = $(TEST_DIR)/test_datetime/test_datetime.cpp
DATETIME_BIN = test_datetime_bin

# Model test
MODEL_SRCS = $(SRC_DIR)/model.cpp $(SRC_DIR)/datetime.cpp
MODEL_TEST = $(TEST_DIR)/test_model/test_model.cpp
MODEL_BIN = test_model_bin

.PHONY: all clean test test_datetime test_model

all: test

test: test_datetime test_model

test_datetime: $(DATETIME_BIN)
	./$(DATETIME_BIN)

test_model: $(MODEL_BIN)
	./$(MODEL_BIN)

$(DATETIME_BIN): $(DATETIME_TEST) $(DATETIME_SRCS) $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) $(UNITY_INC) -o $@ $^ $(UNITY_SRC) $(LDFLAGS)

$(MODEL_BIN): $(MODEL_TEST) $(MODEL_SRCS) $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) $(UNITY_INC) -o $@ $^ $(UNITY_SRC) $(LDFLAGS)

clean:
	rm -f $(DATETIME_BIN) $(MODEL_BIN)
