#include <Arduino.h>
#include <SensoraEsp.h>

Property led("led", "Room LED");

int c = 1;
void handleLedChange(PropertyValue& val) {
  SENSORA_LOGI("received value '%d'", val.Bool());
  if (val.Bool()) {
    SENSORA_LOGI("should turn ON %d", c);
    if (c == 3) {
      c = 1;
    } else {
      c++;
    }
    switch (c)
    {
    case 1:
      neopixelWrite(BUILTIN_LED, 255, 0, 0);
      break;
    case 2:
      neopixelWrite(BUILTIN_LED, 0, 255, 0);
      break;
    case 3:
      neopixelWrite(BUILTIN_LED, 0, 0, 255);
      break;
    default:
      break;
    }
 
  } else {
    SENSORA_LOGI("should turn OFF");
    neopixelWrite(BUILTIN_LED, 0, 0, 0);
  }
  
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