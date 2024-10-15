#include <Arduino.h>
#include <EspWifi.h>

Property led("led");
void handleLedChange(PropertyValue& val) {
  if (val.Bool()) {
    SENSORA_LOGI("should turn ON");
  } else {
    SENSORA_LOGI("should turn OFF");
  }
  digitalWrite(LED_BUILTIN, !val.Bool());
}

void setup() {
  Serial.begin(115200);
  led.setDataType(DataType::Boolean).setAccessMode(AccessMode::Write).subscribe(handleLedChange);
  pinMode(LED_BUILTIN, OUTPUT);
  Sensora.setup();
}

void loop() {
  Sensora.loop();
}