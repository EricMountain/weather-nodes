#include "nodeapp.h"
#include "secrets.h"

void showHeapInfo(const char* msg) {
  size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  Serial.printf("%s - Free heap: %d bytes\n", msg, free_heap);
}

void setupSerial() {
  if (Serial) {
    Serial.println("Serial already initialized");
    return;
  }

  int attempts = 20;
  Serial.begin(115200);
  while (!Serial) {
    delay(100 / (attempts < 1 ? 1 : attempts));
    if (--attempts <= 0) {
      Serial.println(
          "Gave up waiting for serial monitor, continuing... (so this "
          "shouldn't appear...)");
      break;
    }
  }
}

void runApp() {
  setupSerial();
  showHeapInfo("Initial heap");
  NodeApp app(WIFI_SSID, WIFI_PASSWORD);
  showHeapInfo("After NodeApp creation");
  app.setup();
  showHeapInfo("After setup");
  app.doApiCalls();
  showHeapInfo("After API calls");
#ifdef HAS_DISPLAY
  app.updateDisplay();
  showHeapInfo("After display update");
#endif
  // app.goToSleep();
}

void goToSleep() {
#ifdef LIGHT_SLEEP_ENABLED
  Serial.println("Going to light sleep...");
#else
  Serial.println("Going to deep sleep...");
#endif
  Serial.printf("Sleeping for %d seconds...\n", SLEEP_SECONDS);
  esp_sleep_enable_timer_wakeup(SLEEP_SECONDS * 1000000ULL);  // microseconds
#ifndef HAS_BATTERY
  delay(100);  // Let serial print
#endif
#ifdef LIGHT_SLEEP_ENABLED
  esp_light_sleep_start();
#else
  esp_deep_sleep_start();
#endif
}

void setup() {}

void loop() {
  runApp();
  showHeapInfo("After runApp");
  goToSleep();
}
