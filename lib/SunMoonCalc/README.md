# Source

[Originally from](https://github.com/ThingPulse/esp8266-weather-station) `thingpulse/ESP8266 Weather Station@^2.3.0`. The
only reason for this is to make the unit tests possible. Might take it out if I refactor `Model()` or `sunandmoon.h` to avoid the dependency.

Unit tests use the `native` platform that `thingpulse/ESP8266 Weather Station` doesn’t support, so the include files aren’t loaded when building in that environment.
