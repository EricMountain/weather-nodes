#include "nodeapp.h"
#include "secrets.h"

NodeApp app(WIFI_SSID, WIFI_PASSWORD);

size_t showHeapInfo(const char* msg) {
  size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  Serial.printf("%s - Free heap: %d bytes\n", msg, free_heap);
  return free_heap;
}

void setupSerial() {
  if (Serial) {
    Serial.println(F("Serial already initialized"));
    return;
  }

  int attempts = 20;
  Serial.begin(115200);
  while (!Serial) {
    delay(100 / (attempts < 1 ? 1 : attempts));
    if (--attempts <= 0) {
      Serial.println(
          F("Gave up waiting for serial monitor, continuing... (so this "
            "shouldn't appear...)"));
      break;
    }
  }
}

bool runApp() {
  bool deepSleepNeeded = false;

  setupSerial();
  showHeapInfo("Initial heap");
  if (!app.setup()) {
    showHeapInfo("Setup failed");
    return true;
  }
  showHeapInfo("After setup");
  app.doApiCalls();
  showHeapInfo("After API calls");
#ifdef HAS_DISPLAY
  deepSleepNeeded = app.updateDisplay();
  showHeapInfo("After display update");
#endif

  return deepSleepNeeded;
}

void goToSleep(bool deepSleepNeeded) {
  bool isLightSleep = !deepSleepNeeded;
  size_t free_heap = showHeapInfo("Before sleep");

  if (free_heap < 100000) {
    Serial.println(
        F("Low memory detected, switching to deep sleep mode forced"));
    isLightSleep = false;
  }

#ifdef LIGHT_SLEEP_ENABLED
  if (isLightSleep) {
    Serial.println(F("Going to light sleep..."));
  } else {
    Serial.println(F("Going to deep sleep..."));
  }
#else
  Serial.println(F("Going to deep sleep..."));
  isLightSleep = false;
#endif

  Serial.printf("Sleeping for %d seconds...\n", SLEEP_SECONDS);
  esp_sleep_enable_timer_wakeup(SLEEP_SECONDS * 1000000ULL);  // microseconds

#ifndef HAS_BATTERY
  delay(100);  // Let serial print
#endif

  if (isLightSleep) {
    esp_light_sleep_start();
  } else {
    esp_deep_sleep_start();
  }
}

void setup() {}

void loop() {
  bool deepSleepNeeded = runApp();
  showHeapInfo("After runApp");
  goToSleep(deepSleepNeeded);
}
