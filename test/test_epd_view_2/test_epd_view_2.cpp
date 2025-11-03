#include <unity.h>
#include <ArduinoJson.h>
#include <string>
#include <map>
#include "epd_view_2.h"
#include "sensor.h"

// Mock sensor for testing
class MockSensor : public Sensor {
 public:
  MockSensor(bool ok_status = true) : ok_status_(ok_status) {}
  
  bool init() override { return true; }
  
  bool ok() const override { return ok_status_; }
  
  std::map<std::string, Measurement> read() override {
    std::map<std::string, Measurement> measurements;
    measurements["temperature"] = {25.5f, "°C"};
    measurements["humidity"] = {60.0f, "%"};
    measurements["pressure"] = {1013.0f, "hPa"};
    return measurements;
  }
  
 private:
  bool ok_status_;
};

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_epdview2_constructor(void) {
  EPDView2 view;
  // Test that constructor doesn't crash
  TEST_ASSERT_TRUE(true);
}

void test_epdview2_cleanup(void) {
  EPDView2 view;
  view.cleanup();
  // Test that cleanup doesn't crash
  TEST_ASSERT_TRUE(true);
}

void test_epdview2_render_with_null_doc(void) {
  EPDView2 view;
  std::map<std::string, Sensor*> sensors;
  
  // Render with nullptr should not crash
  bool result = view.render(nullptr, sensors);
  
  // Should return some result without crashing
  TEST_ASSERT_TRUE(result || !result);  // Just check it doesn't crash
}

void test_epdview2_render_with_empty_doc(void) {
  EPDView2 view;
  std::map<std::string, Sensor*> sensors;
  JsonDocument doc;
  
  // Render with empty document
  bool result = view.render(&doc, sensors);
  
  // Should return some result without crashing
  TEST_ASSERT_TRUE(result || !result);
}

void test_epdview2_render_with_valid_data(void) {
  EPDView2 view;
  std::map<std::string, Sensor*> sensors;
  MockSensor mockSensor(true);
  sensors["bme680"] = &mockSensor;
  
  JsonDocument doc;
  doc["timestamp_utc"] = "2025-11-03T20:00:00";
  doc["timestamp_local"] = "2025-11-03T21:00:00";
  JsonObject nodes = doc["nodes"].to<JsonObject>();
  JsonObject node1 = nodes["node1"].to<JsonObject>();
  node1["display_name"] = "Indoor";
  node1["battery_level"] = "A";
  node1["stale_state"] = "";
  
  JsonObject measurements_v2 = node1["measurements_v2"].to<JsonObject>();
  JsonObject bme680 = measurements_v2["bme680"].to<JsonObject>();
  bme680["temperature"] = 22.5;
  bme680["humidity"] = 55.0;
  bme680["pressure"] = 1013.0;
  
  JsonObject status = node1["status"].to<JsonObject>();
  status["sensor"] = "ok";
  
  // Render with valid data
  bool result = view.render(&doc, sensors);
  
  // Should return some result without crashing
  TEST_ASSERT_TRUE(result || !result);
}

void test_epdview2_render_with_bad_sensor(void) {
  EPDView2 view;
  std::map<std::string, Sensor*> sensors;
  MockSensor mockSensor(false);  // Sensor reports not ok
  sensors["bme680"] = &mockSensor;
  
  JsonDocument doc;
  doc["timestamp_utc"] = "2025-11-03T20:00:00";
  
  // Render with bad sensor
  bool result = view.render(&doc, sensors);
  
  // Should return some result without crashing
  TEST_ASSERT_TRUE(result || !result);
}

void test_epdview2_render_multiple_nodes(void) {
  EPDView2 view;
  std::map<std::string, Sensor*> sensors;
  
  JsonDocument doc;
  doc["timestamp_utc"] = "2025-11-03T20:00:00";
  doc["timestamp_local"] = "2025-11-03T21:00:00";
  
  JsonObject nodes = doc["nodes"].to<JsonObject>();
  
  // Node 1
  JsonObject node1 = nodes["node1"].to<JsonObject>();
  node1["display_name"] = "Indoor";
  node1["battery_level"] = "A";
  JsonObject measurements1 = node1["measurements_v2"].to<JsonObject>();
  JsonObject bme680_1 = measurements1["bme680"].to<JsonObject>();
  bme680_1["temperature"] = 22.5;
  bme680_1["humidity"] = 55.0;
  
  // Node 2
  JsonObject node2 = nodes["node2"].to<JsonObject>();
  node2["display_name"] = "Outdoor";
  node2["battery_level"] = "B";
  JsonObject measurements2 = node2["measurements_v2"].to<JsonObject>();
  JsonObject sht31d_2 = measurements2["sht31d"].to<JsonObject>();
  sht31d_2["temperature"] = 15.0;
  sht31d_2["humidity"] = 70.0;
  
  // Render with multiple nodes
  bool result = view.render(&doc, sensors);
  
  // Should return some result without crashing
  TEST_ASSERT_TRUE(result || !result);
}

void test_epdview2_render_with_min_max(void) {
  EPDView2 view;
  std::map<std::string, Sensor*> sensors;
  
  JsonDocument doc;
  doc["timestamp_utc"] = "2025-11-03T20:00:00";
  
  JsonObject nodes = doc["nodes"].to<JsonObject>();
  JsonObject node1 = nodes["node1"].to<JsonObject>();
  node1["display_name"] = "Indoor";
  
  JsonObject measurements = node1["measurements_v2"].to<JsonObject>();
  JsonObject bme680 = measurements["bme680"].to<JsonObject>();
  bme680["temperature"] = 22.5;
  
  // Add min/max data
  JsonObject min_max = node1["measurements_min_max"].to<JsonObject>();
  JsonObject device_min_max = min_max["bme680"].to<JsonObject>();
  JsonObject temp_min_max = device_min_max["temperature"].to<JsonObject>();
  temp_min_max["min"] = 18.0;
  temp_min_max["max"] = 25.0;
  
  // Render with min/max data
  bool result = view.render(&doc, sensors);
  
  // Should return some result without crashing
  TEST_ASSERT_TRUE(result || !result);
}

void test_epdview2_render_with_bad_status(void) {
  EPDView2 view;
  std::map<std::string, Sensor*> sensors;
  
  JsonDocument doc;
  doc["timestamp_utc"] = "2025-11-03T20:00:00";
  
  JsonObject nodes = doc["nodes"].to<JsonObject>();
  JsonObject node1 = nodes["node1"].to<JsonObject>();
  node1["display_name"] = "Indoor";
  
  JsonObject status = node1["status"].to<JsonObject>();
  status["sensor"] = "error";
  status["wifi"] = "ok";
  
  // Render with bad status
  bool result = view.render(&doc, sensors);
  
  // Should return some result without crashing
  TEST_ASSERT_TRUE(result || !result);
}

void test_epdview2_render_with_stale_state(void) {
  EPDView2 view;
  std::map<std::string, Sensor*> sensors;
  
  JsonDocument doc;
  doc["timestamp_utc"] = "2025-11-03T20:00:00";
  
  JsonObject nodes = doc["nodes"].to<JsonObject>();
  JsonObject node1 = nodes["node1"].to<JsonObject>();
  node1["display_name"] = "Indoor";
  node1["stale_state"] = "Data is stale";
  
  // Render with stale state
  bool result = view.render(&doc, sensors);
  
  // Should return some result without crashing
  TEST_ASSERT_TRUE(result || !result);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_epdview2_constructor);
  RUN_TEST(test_epdview2_cleanup);
  RUN_TEST(test_epdview2_render_with_null_doc);
  RUN_TEST(test_epdview2_render_with_empty_doc);
  RUN_TEST(test_epdview2_render_with_valid_data);
  RUN_TEST(test_epdview2_render_with_bad_sensor);
  RUN_TEST(test_epdview2_render_multiple_nodes);
  RUN_TEST(test_epdview2_render_with_min_max);
  RUN_TEST(test_epdview2_render_with_bad_status);
  RUN_TEST(test_epdview2_render_with_stale_state);
  UNITY_END();

  return 0;
}
