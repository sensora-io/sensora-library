# Sensora library

Designed to simplify IoT development for ESP32 and ESP8266 devices. It enables easy and efficient connectivity and control for a wide range of IoT applications.
For more information, please visit [Sensora library documentation](https://docs.sensora.io)


## Features
-   **Easy Integration**: Seamlessly integrates with ESP32 and ESP8266 boards.
-   **Extensive Compatibility**: Works with a variety of sensors and actuators.
-   **User-Friendly API**: Intuitive and easy-to-use functions.
-   **Customizability**: Flexible to fit various IoT project needs.
-   **Reliable Connectivity**: Supports multiple connectivity options.
-   **Lightweight and Efficient**: Optimized for performance and memory usage.

## Installation

To install Sensora library, we suggest using  **[PlatformIO](https://platformio.org/)**  for IoT development. You can install it in your favourite editor, [How to install PlatformIO](https://platformio.org/platformio-ide).

-   Create new project "PlatformIO Home > New Project"
-   Update  [`platformio.ini`](https://docs.platformio.org/en/latest/projectconf/index.html)  file and add Sensora library as a dependency.

```
[env:myenv]
lib_deps =
  sensora/sensora-library
```

-   Build the project and PlatformIO will automatically install dependencies.

## Quick Start

Here’s a simple example to get you started:

```cpp
#include <Arduino.h>
#include <SensoraEsp.h>

Property temperatureProperty("temperature", "Room Temperature");
Property humidityProperty("humidity", "Room Humidity");

unsigned long lastRead = 0;
int roomTemperature = 20;
int roomHumidity = 40;

void setup() {
  Serial.begin(115200);
  temperatureProperty.setDataType(DataType::Integer).setAccessMode(AccessMode::Read);
  humidityProperty.setDataType(DataType::Integer).setAccessMode(AccessMode::Read);
  Sensora.setup();
}

void loop() {
  Sensora.loop();

  if (millis() - lastRead > 15000) {
    // simulate a value between 0 and 15 degrees
    roomTemperature = random(0, 15);
    temperatureProperty.setValue(roomTemperature);

    // simulate a value between 40 and 70 percent
    roomHumidity = random(40, 70);
    humidityProperty.setValue(roomHumidity);
    lastRead = millis();
  }
}
```

## Documentation

For detailed documentation, visit [library documentation](https://docs.sensora.io/library/overview).

## Supported Hardware

- ESP8266
- ESP32
- More coming soon

## Contributing

Contributions to Sensora library are welcome. Please read our contributing guidelines for more information.

## Get involved
- Follow [@sensora_io on Twitter](https://twitter.com/sensora_io)
- For general discussions, join on the [official Discord](https://discord.gg/cqx6c8fMkM) team.

## Support

For support, questions, or feature requests, open an issue here or visit our community forum.

## License

See [LICENSE](./LICENSE).
