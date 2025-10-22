# ESP32 Weather Nodes Testing Framework

This document describes the testing framework for the ESP32 weather nodes project.

## Testing Strategy

The project uses **PlatformIO's native testing framework** which allows running unit tests both locally (Linux/Mac) and in GitHub CI without requiring ESP32 hardware.

### Test Types

1. **Native Platform Tests** - Unit tests that run on the host machine (Linux/Mac)
   - Tests for business logic (DateTime, Model classes)
   - No hardware dependencies
   - Fast execution
   - Can run in development environment and CI

2. **Embedded Tests** (Future) - Tests that run on actual ESP32 hardware
   - For hardware-specific features (sensors, display, WiFi)
   - Requires ESP32 hardware connection

## Test Structure

```
test/
├── mocks/                    # Mock implementations for Arduino APIs
│   ├── Arduino.h            # Mock Arduino.h for native platform
│   └── Arduino.cpp          # Mock Serial implementation
├── test_datetime/           # DateTime class tests
│   └── test_datetime.cpp
└── test_model/              # Model class tests
    └── test_model.cpp
```

## Running Tests Locally

### Prerequisites

1. Install PlatformIO Core:
   ```bash
   pip install platformio
   ```

2. The `native` platform will be automatically installed when running tests

### Run All Tests

From the project root directory:

```bash
pio test -e native
```

### Run Specific Test

```bash
pio test -e native -f test_datetime
```

### Verbose Output

```bash
pio test -e native -v
```

### Watch Mode (Auto-rerun on changes)

PlatformIO doesn't have built-in watch mode, but you can use tools like `entr`:

```bash
find src test -name "*.cpp" -o -name "*.h" | entr -c pio test -e native
```

## Test Configuration

The testing configuration is in `platformio.ini`:

```ini
[env:native]
platform = native
build_flags = 
    -std=c++11
    -D UNIT_TEST
    -I test/mocks
lib_deps = 
    bblanchon/ArduinoJson@^7.4.2
    fmtlib/fmt
test_build_src = yes
```

Key settings:
- `platform = native` - Uses the native platform for running on host machine
- `test_build_src = yes` - Includes source files in test builds
- `-D UNIT_TEST` - Defines UNIT_TEST macro for conditional compilation
- `-I test/mocks` - Includes mock directory for Arduino.h replacement

## Writing New Tests

### Test File Structure

Each test suite should be in its own directory under `test/`:

```
test/
└── test_myfeature/
    └── test_myfeature.cpp
```

### Test Template

```cpp
#include <unity.h>
#include "myfeature.h"

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

void test_feature_basic_functionality(void) {
    // Arrange
    MyFeature feature;
    
    // Act
    int result = feature.doSomething();
    
    // Assert
    TEST_ASSERT_EQUAL(42, result);
}

void test_feature_edge_case(void) {
    // Test edge cases
    TEST_ASSERT_TRUE(someCondition);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_feature_basic_functionality);
    RUN_TEST(test_feature_edge_case);
    UNITY_END();
    return 0;
}
```

### Unity Assertions

Common Unity test assertions:
- `TEST_ASSERT_TRUE(condition)`
- `TEST_ASSERT_FALSE(condition)`
- `TEST_ASSERT_EQUAL(expected, actual)`
- `TEST_ASSERT_EQUAL_INT(expected, actual)`
- `TEST_ASSERT_EQUAL_STRING(expected, actual)`
- `TEST_ASSERT_EQUAL_DOUBLE(expected, actual)`
- `TEST_ASSERT_NULL(pointer)`
- `TEST_ASSERT_NOT_NULL(pointer)`

See [Unity Assertions Reference](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md) for complete list.

## GitHub CI Integration

Tests run automatically on:
- Pull requests to `main` branch
- Pushes to `main` branch

The CI configuration is in `.github/workflows/test.yml`.

### CI Workflow

The workflow:
1. Checks out the code
2. Sets up Python
3. Installs PlatformIO
4. Runs all tests in the native environment
5. Reports results

## Mocking Hardware Dependencies

For native tests, hardware-dependent Arduino APIs are mocked in `test/mocks/`:

### Arduino.h Mock

Provides minimal Arduino API compatibility:
- `Serial.println()` - Prints to stdout
- `Serial.printf()` - Formatted printing
- Basic types (`byte`, etc.)

When code includes `<Arduino.h>` with `UNIT_TEST` defined, it uses the mock version instead of the real Arduino framework.

## Test Coverage

Current test coverage includes:

### DateTime Class
- Default constructor (empty DateTime)
- Parsing valid ISO 8601 timestamps
- Handling invalid timestamps
- Date suffix generation (1st, 2nd, 3rd, 11th, 21st, etc.)
- Date difference calculations
- Nice date formatting

### Model Class
- Default constructor
- Setting and getting datetime
- Sun info (rise, transit, set times)
- Moon info (phase, rise, transit, set times)
- JSON serialization (toJsonString)
- JSON deserialization (fromJsonString)
- Equality/inequality operators
- Invalid JSON handling

## Troubleshooting

### Tests fail to compile

1. Check that source files don't have ESP32-specific includes when `UNIT_TEST` is defined
2. Verify mock implementations provide all needed functions
3. Check that library dependencies are specified in `platformio.ini`

### Tests fail to run

1. Ensure PlatformIO is up to date: `pip install -U platformio`
2. Clean and rebuild: `pio test -e native --clean`
3. Check test output for specific error messages

### Platform installation fails

If you see network errors installing the native platform:
1. Check internet connectivity
2. Try running with verbose output: `pio test -e native -v`
3. The platform will be cached after first successful installation

## Best Practices

1. **Keep tests focused** - Each test should verify one specific behavior
2. **Use descriptive names** - Test names should describe what they test
3. **Test edge cases** - Don't just test the happy path
4. **Mock external dependencies** - Keep tests isolated and fast
5. **Run tests frequently** - Run tests before committing
6. **Keep tests fast** - Unit tests should run in milliseconds

## Future Enhancements

Potential improvements to the testing framework:

1. **Code Coverage** - Add coverage reporting with lcov/gcov
2. **Embedded Tests** - Add tests for hardware-specific features
3. **Integration Tests** - Test interaction with external services
4. **Performance Tests** - Measure and track performance metrics
5. **Continuous Monitoring** - Test on actual hardware in CI
6. **Test Fixtures** - Shared test data and setup code
