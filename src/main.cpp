#include "nodeapp.h"
#include "secrets.h"

NodeApp app(WIFI_SSID, WIFI_PASSWORD);

void setup() {}

void loop() {
  app.setup();
  size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  Serial.printf("Free heap: %d bytes\n", free_heap);
  app.doApiCalls();
#ifdef HAS_DISPLAY
  app.updateDisplay();
#endif
  app.goToSleep();
}
