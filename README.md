# ESP32-based Weather Stations (temperature, hygrometry, pressure…)

## ESP32 nodes

### Configuring

Before building the project:

* Edit `config.h`
* Edit `secrets.h`. Use `secrets.h.example` as a starting point
* Run `certs-for-urls.sh`

### Building

* Use PlatformIO

### Testing

The project includes a comprehensive testing framework for unit testing ESP32 code. Tests run on your local machine (Linux/Mac) without requiring ESP32 hardware, and also run automatically in GitHub CI.

See [test/README.md](test/README.md) for detailed testing documentation.

Quick start:
```shell
# Install PlatformIO
pip install platformio

# Run all tests
pio test -e native

# Run specific test
pio test -e native -f test_datetime
```

## Lambdas

### Send measurements

```shell
curl -v -X POST --data '{"measurements": {"temperature": -15, "humidity": 55}}' -H 'x-api-key: sensor1key' https://xxxx.lambda-url.eu-west-3.on.aws/
```

### Get display

```shell
curl -v -H 'x-api-key: displaydevkey' https://xxxx.lambda-url.eu-west-3.on.aws/
```
