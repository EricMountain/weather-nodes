#include <unity.h>
#include <string>
#include "datetime.h"

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_datetime_default_constructor(void) {
  DateTime dt;
  TEST_ASSERT_FALSE(dt.ok());
  TEST_ASSERT_EQUAL(0, dt.year());
  TEST_ASSERT_EQUAL(0, dt.month());
  TEST_ASSERT_EQUAL(0, dt.day());
}

void test_datetime_parsing_valid_timestamp(void) {
  DateTime dt("2025-10-21T15:30:45");
  TEST_ASSERT_TRUE(dt.ok());
  TEST_ASSERT_EQUAL(2025, dt.year());
  TEST_ASSERT_EQUAL(10, dt.month());
  TEST_ASSERT_EQUAL(21, dt.day());
  TEST_ASSERT_EQUAL(15, dt.hour());
  TEST_ASSERT_EQUAL(30, dt.minute());
  TEST_ASSERT_EQUAL(45, dt.second());
}

void test_datetime_parsing_invalid_timestamp(void) {
  DateTime dt("invalid-date");
  TEST_ASSERT_FALSE(dt.ok());
  TEST_ASSERT_EQUAL(0, dt.year());
}

void test_datetime_date_suffix_first(void) {
  DateTime dt("2025-10-01T12:00:00");
  std::string nice = dt.niceDate();
  // Should contain "1st"
  TEST_ASSERT_TRUE(nice.find("1st") != std::string::npos);
}

void test_datetime_date_suffix_second(void) {
  DateTime dt("2025-10-02T12:00:00");
  std::string nice = dt.niceDate();
  // Should contain "2nd"
  TEST_ASSERT_TRUE(nice.find("2nd") != std::string::npos);
}

void test_datetime_date_suffix_third(void) {
  DateTime dt("2025-10-03T12:00:00");
  std::string nice = dt.niceDate();
  // Should contain "3rd"
  TEST_ASSERT_TRUE(nice.find("3rd") != std::string::npos);
}

void test_datetime_date_suffix_eleventh(void) {
  DateTime dt("2025-10-11T12:00:00");
  std::string nice = dt.niceDate();
  // Should contain "11th" not "11st"
  TEST_ASSERT_TRUE(nice.find("11th") != std::string::npos);
}

void test_datetime_date_suffix_twenty_first(void) {
  DateTime dt("2025-10-21T12:00:00");
  std::string nice = dt.niceDate();
  // Should contain "21st"
  TEST_ASSERT_TRUE(nice.find("21st") != std::string::npos);
}

void test_datetime_diff(void) {
  DateTime dt1("2025-10-21T12:00:00");
  DateTime dt2("2025-10-21T12:00:30");
  double diff = dt2.diff(dt1);
  // Should be 30 seconds difference
  TEST_ASSERT_EQUAL_DOUBLE(30.0, diff);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_datetime_default_constructor);
  RUN_TEST(test_datetime_parsing_valid_timestamp);
  RUN_TEST(test_datetime_parsing_invalid_timestamp);
  RUN_TEST(test_datetime_date_suffix_first);
  RUN_TEST(test_datetime_date_suffix_second);
  RUN_TEST(test_datetime_date_suffix_third);
  RUN_TEST(test_datetime_date_suffix_eleventh);
  RUN_TEST(test_datetime_date_suffix_twenty_first);
  RUN_TEST(test_datetime_diff);
  UNITY_END();

  return 0;
}
