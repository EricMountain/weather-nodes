#include <unity.h>
#include <string>
#include "model.h"

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_model_default_constructor(void) {
  Model model;
  TEST_ASSERT_FALSE(model.jsonLoadOK());
  TEST_ASSERT_EQUAL_STRING("", model.getDate().c_str());
}

void test_model_set_and_get_datetime(void) {
  Model model;
  model.setDate("2025-10-21T15:30:45");
  TEST_ASSERT_EQUAL_STRING("2025-10-21T15:30:45", model.getDate().c_str());
}

void test_model_set_sun_info(void) {
  Model model;
  model.setSunInfo("06:30", "12:45", "18:30");
  TEST_ASSERT_EQUAL_STRING("06:30", model.getSunRise().c_str());
  TEST_ASSERT_EQUAL_STRING("12:45", model.getSunTransit().c_str());
  TEST_ASSERT_EQUAL_STRING("18:30", model.getSunSet().c_str());
}

void test_model_set_moon_info(void) {
  Model model;
  model.setMoonInfo("Full Moon", "F", "19:00", "01:30", "07:00");
  TEST_ASSERT_EQUAL_STRING("Full Moon", model.getMoonPhase().c_str());
  TEST_ASSERT_EQUAL('F', model.getMoonPhaseLetter());
  TEST_ASSERT_EQUAL_STRING("19:00", model.getMoonRise().c_str());
  TEST_ASSERT_EQUAL_STRING("01:30", model.getMoonTransit().c_str());
  TEST_ASSERT_EQUAL_STRING("07:00", model.getMoonSet().c_str());
}

void test_model_to_json_string(void) {
  Model model;
  model.setDate("2025-10-21T15:30:45");
  model.setSunInfo("06:30", "12:45", "18:30");

  std::string json = model.toJsonString();
  TEST_ASSERT_TRUE(json.length() > 0);
  TEST_ASSERT_TRUE(json.find("2025-10-21T15:30:45") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("06:30") != std::string::npos);
}

void test_model_from_json_string(void) {
  std::string json =
      R"({"date":"2025-10-21T15:30:45","sun":{"rise":"06:30","transit":"12:45","set":"18:30"}})";
  Model model(json);

  TEST_ASSERT_TRUE(model.jsonLoadOK());
  TEST_ASSERT_EQUAL_STRING("2025-10-21T15:30:45", model.getDate().c_str());
  TEST_ASSERT_EQUAL_STRING("06:30", model.getSunRise().c_str());
  TEST_ASSERT_EQUAL_STRING("12:45", model.getSunTransit().c_str());
  TEST_ASSERT_EQUAL_STRING("18:30", model.getSunSet().c_str());
}

void test_model_from_invalid_json(void) {
  std::string json = "this is not valid json";
  Model model(json);

  TEST_ASSERT_FALSE(model.jsonLoadOK());
}

void test_model_equality_operator(void) {
  Model model1;
  model1.setDate("2025-10-21T15:30:45");

  Model model2;
  model2.setDate("2025-10-21T15:30:45");

  TEST_ASSERT_TRUE(model1 == model2);
  TEST_ASSERT_FALSE(model1 != model2);
}

void test_model_inequality_operator(void) {
  Model model1;
  model1.setDate("2025-10-21T15:30:45");

  Model model2;
  model2.setDate("2025-10-21T16:30:45");

  TEST_ASSERT_FALSE(model1 == model2);
  TEST_ASSERT_TRUE(model1 != model2);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_model_default_constructor);
  RUN_TEST(test_model_set_and_get_datetime);
  RUN_TEST(test_model_set_sun_info);
  RUN_TEST(test_model_set_moon_info);
  RUN_TEST(test_model_to_json_string);
  RUN_TEST(test_model_from_json_string);
  RUN_TEST(test_model_from_invalid_json);
  RUN_TEST(test_model_equality_operator);
  RUN_TEST(test_model_inequality_operator);
  UNITY_END();

  return 0;
}
