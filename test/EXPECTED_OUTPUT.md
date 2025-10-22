# Expected Test Output

This document shows what successful test execution looks like.

## When Tests Pass

### Command
```bash
pio test -e native -v
```

### Expected Output
```
Verbosity level can be increased via `-v, -vv, or -vvv` option
Collected 2 tests

Processing test_datetime in native environment
--------------------------------------------------------------------
Building... (platform: native, board: native, environment: native)
Looking for test PAD standard
LDF Modes: Finder ~ chain+, Compatibility ~ soft
Found 2 compatible libraries
Scanning dependencies...
Dependency Graph
|-- fmt
|-- ArduinoJson
|-- Unity
Building in test mode
Compiling .pio/build/native/test/test_datetime/test_datetime.o
Compiling .pio/build/native/src/datetime.o
Compiling .pio/build/native/test/mocks/Arduino.o
Archiving .pio/build/native/libfmt.a
Archiving .pio/build/native/libArduinoJson.a
Linking .pio/build/native/program
Testing... (running test cases)

test/test_datetime/test_datetime.cpp:13:test_datetime_default_constructor      [PASSED]
test/test_datetime/test_datetime.cpp:20:test_datetime_parsing_valid_timestamp  [PASSED]
test/test_datetime/test_datetime.cpp:32:test_datetime_parsing_invalid_timestamp        [PASSED]
test/test_datetime/test_datetime.cpp:38:test_datetime_date_suffix_first        [PASSED]
test/test_datetime/test_datetime.cpp:45:test_datetime_date_suffix_second       [PASSED]
test/test_datetime/test_datetime.cpp:52:test_datetime_date_suffix_third        [PASSED]
test/test_datetime/test_datetime.cpp:59:test_datetime_date_suffix_eleventh     [PASSED]
test/test_datetime/test_datetime.cpp:66:test_datetime_date_suffix_twenty_first [PASSED]
test/test_datetime/test_datetime.cpp:73:test_datetime_diff    [PASSED]

9 Tests 0 Failures 0 Ignored
OK

Processing test_model in native environment
--------------------------------------------------------------------
Building... (platform: native, board: native, environment: native)
Looking for test PAD standard
LDF Modes: Finder ~ chain+, Compatibility ~ soft
Found 2 compatible libraries
Scanning dependencies...
Dependency Graph
|-- fmt
|-- ArduinoJson
|-- Unity
Building in test mode
Compiling .pio/build/native/test/test_model/test_model.o
Compiling .pio/build/native/src/model.o
Compiling .pio/build/native/src/datetime.o
Compiling .pio/build/native/test/mocks/Arduino.o
Linking .pio/build/native/program
Testing... (running test cases)

test/test_model/test_model.cpp:13:test_model_default_constructor      [PASSED]
test/test_model/test_model.cpp:19:test_model_set_and_get_datetime     [PASSED]
test/test_model/test_model.cpp:25:test_model_set_sun_info     [PASSED]
test/test_model/test_model.cpp:32:test_model_set_moon_info    [PASSED]
test/test_model/test_model.cpp:41:test_model_to_json_string   [PASSED]
test/test_model/test_model.cpp:51:test_model_from_json_string [PASSED]
test/test_model/test_model.cpp:61:test_model_from_invalid_json        [PASSED]
test/test_model/test_model.cpp:67:test_model_equality_operator        [PASSED]
test/test_model/test_model.cpp:78:test_model_inequality_operator      [PASSED]

9 Tests 0 Failures 0 Ignored
OK

======================================================= SUMMARY =======================================================
Environment    Test           Status    Duration
-------------  -------------  --------  ------------
native         test_datetime  PASSED    00:00:02.456
native         test_model     PASSED    00:00:02.134
====================================== 2 succeeded in 00:00:04.590 ======================================
```

## When a Test Fails

### Example Failed Test Output
```
test/test_datetime/test_datetime.cpp:20:test_datetime_parsing_valid_timestamp  [FAILED]
test/test_datetime/test_datetime.cpp:25: Expected 2025 Was 2024

9 Tests 1 Failure 0 Ignored
FAIL
```

### Detailed Failure Information
The failure shows:
- File and line number: `test/test_datetime/test_datetime.cpp:25`
- Expected value: `2025`
- Actual value: `2024`
- Test name: `test_datetime_parsing_valid_timestamp`

## GitHub Actions Output

### Successful CI Run
```
✓ Checkout code
✓ Set up Python (3.11)
✓ Cache PlatformIO (cache hit)
✓ Install PlatformIO
  PlatformIO Core, version 6.1.18
✓ Run tests
  Collected 2 tests
  Processing test_datetime in native environment
  9 Tests 0 Failures 0 Ignored - OK
  Processing test_model in native environment
  9 Tests 0 Failures 0 Ignored - OK
  2 succeeded in 00:00:05.234
✓ Upload test results
```

### Failed CI Run
```
✓ Checkout code
✓ Set up Python (3.11)
✓ Cache PlatformIO (cache hit)
✓ Install PlatformIO
✗ Run tests
  Collected 2 tests
  Processing test_datetime in native environment
  9 Tests 1 Failure 0 Ignored - FAIL
  Error: Process completed with exit code 1
```

## Interpreting Results

### Test Statistics
- **Passed**: Test assertion succeeded
- **Failed**: Test assertion failed
- **Ignored**: Test was skipped (not used in current setup)

### Common Assertions
```cpp
TEST_ASSERT_EQUAL(expected, actual)     // Values must be exactly equal
TEST_ASSERT_TRUE(condition)              // Condition must be true
TEST_ASSERT_FALSE(condition)             // Condition must be false
TEST_ASSERT_NULL(pointer)                // Pointer must be NULL
TEST_ASSERT_NOT_NULL(pointer)            // Pointer must not be NULL
```

### Exit Codes
- `0`: All tests passed
- `1`: One or more tests failed
- Other: System error (compilation failed, etc.)

## Performance

### Typical Execution Times
- **First run** (with compilation): 10-15 seconds
- **Subsequent runs** (cached): 3-5 seconds
- **Single test suite**: 2-3 seconds
- **CI environment** (with caching): 5-8 seconds

### Build Artifacts
Generated in `.pio/build/native/`:
- `program`: Test executable
- `*.o`: Object files
- `lib*.a`: Library archives

## Troubleshooting Output

### Compilation Errors
```
Compiling .pio/build/native/src/datetime.o
src/datetime.cpp:25:10: error: 'strptime' was not declared in this scope
```
**Solution**: Check includes and platform compatibility

### Linking Errors
```
Linking .pio/build/native/program
undefined reference to `DateTime::niceDate()'
```
**Solution**: Verify all source files are included

### Runtime Errors
```
Segmentation fault (core dumped)
```
**Solution**: Check for null pointer dereferences, memory issues

## Next Steps

After seeing successful test output:
1. Add more test cases
2. Increase test coverage
3. Monitor CI results
4. Refactor with confidence

See `test/README.md` for detailed testing guide.
