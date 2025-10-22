# Quick Reference - ESP32 Testing

## Prerequisites
```bash
pip install platformio
```

## Run Tests

### All Tests
```bash
pio test -e native
```

### Single Test Suite
```bash
pio test -e native -f test_datetime
```

### Verbose Output
```bash
pio test -e native -v
```

### List Available Tests
```bash
pio test --list-tests -e native
```

## Verify Setup
```bash
./test/verify-tests.sh
```

## Common Unity Assertions

```cpp
// Boolean
TEST_ASSERT_TRUE(condition)
TEST_ASSERT_FALSE(condition)

// Equality
TEST_ASSERT_EQUAL(expected, actual)
TEST_ASSERT_EQUAL_INT(expected, actual)
TEST_ASSERT_EQUAL_STRING(expected, actual)
TEST_ASSERT_EQUAL_DOUBLE(expected, actual)

// Null checks
TEST_ASSERT_NULL(pointer)
TEST_ASSERT_NOT_NULL(pointer)

// String contains
TEST_ASSERT_TRUE(str.find("text") != std::string::npos)
```

## Test Template

```cpp
#include <unity.h>
#include "myclass.h"

void setUp(void) { }
void tearDown(void) { }

void test_feature(void) {
    MyClass obj;
    TEST_ASSERT_EQUAL(42, obj.getValue());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_feature);
    UNITY_END();
    return 0;
}
```

## File Structure

```
test/
├── test_myfeature/
│   └── test_myfeature.cpp
├── mocks/
│   ├── Arduino.h
│   └── Arduino.cpp
└── README.md
```

## Troubleshooting

### Clean Build
```bash
pio test -e native --clean
```

### Update PlatformIO
```bash
pip install -U platformio
```

### Check Platform
```bash
pio pkg list
```

## CI Status

Check test status:
- GitHub Actions tab
- Pull request checks
- Branch protection rules

## Documentation

- Full guide: `test/README.md`
- Implementation: `test/IMPLEMENTATION.md`
- Main README: `README.md`
