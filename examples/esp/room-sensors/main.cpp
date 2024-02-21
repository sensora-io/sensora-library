#include <Arduino.h>
#include <SensoraEsp.h>

Property temperatureProperty("temperature", "Room Temperature");
Property humidityProperty("humidity", "Room Humidity");

void setup() {
  Serial.begin(115200);
  temperatureProperty.setDataType(DataType::Integer).setAccessMode(AccessMode::Read);
  humidityProperty.setDataType(DataType::Integer).setAccessMode(AccessMode::Read);
  Sensora.setup();
}

unsigned long lastRead = 0;
void loop() {
  Sensora.loop();

  if (millis() - lastRead > 5000) {
    // simulate a value between 0 and 15 degrees
    int roomTemperature = random(0, 15);
    temperatureProperty.setValue(roomTemperature);

    // simulate a value between 40 and 70 degrees
    int roomHumidity = random(40, 70);
    humidityProperty.setValue(roomHumidity);
    lastRead = millis();
  }
}