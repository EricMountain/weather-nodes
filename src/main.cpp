#include "nodeapp.h"
#include "secrets.h"

void setup()
{
  NodeApp app(WIFI_SSID, WIFI_PASSWORD);
  app.setup();
  app.doApiCalls();
#ifdef HAS_DISPLAY
  app.updateDisplay();
#endif
  app.goToSleep();
}

void loop()
{
  // Nothing
}
