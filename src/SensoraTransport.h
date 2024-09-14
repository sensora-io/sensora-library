/*
 * Copyright 2019-2024 Sensora LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SensoraTransport_h
#define SensoraTransport_h

#include <ArduinoMqttClient.h>

template <typename TClient>
class SensoraTransport {
 public:
  SensoraTransport(TClient& client) : mqttClient(client) {
  }

  void setup() {
    SENSORA_LOGD("setup Sensora transport");
    mqttClient.setId(deviceConfig.deviceId);
    mqttClient.setUsernamePassword("", deviceConfig.deviceToken);
    mqttClient.setKeepAliveInterval(15 * 1000L);
    char willTopic[45];
    snprintf(willTopic, sizeof(willTopic), "sc/%s/dev/info", deviceConfig.deviceId);
    SensoraPayload p;
    p.add("status", static_cast<uint8_t>(DeviceStatus::Lost));
    mqttClient.beginWill(willTopic, true, 1);
    mqttClient.write(p.buffer(), p.length());
    mqttClient.endWill();
  }

  bool connect() {
    SENSORA_LOGD("connecting to Sensora Cloud");
    if (mqttClient.connected()) {
      return true;
    }
    if (!mqttClient.connect(MQTT_HOST, 1883)) {
      SENSORA_LOGE("failed to connect to Sensora Cloud, code %d", mqttClient.connectError());
      return false;
    } else {
      SENSORA_LOGI("connected to Sensora Cloud");
      return true;
    }
  }

  bool publish(const char* topic, const uint8_t* buf, unsigned long size) {
    if (size == 0) {
      SENSORA_LOGW("cannot publish mqtt paylod with size 0");
      return false;
    }
    SENSORA_LOGD("publish to topic '%s'", topic);
    if (mqttClient.beginMessage(topic, size, false, 0)) {
      if (mqttClient.write(buf, size)) {
        if (mqttClient.endMessage()) {
          return true;
        }
      }
    }
    return false;
  }

  int subscribe(const char* topic) {
    return mqttClient.subscribe(topic, 1);
  }

  bool connected() { return mqttClient.connected(); }
  MqttClient& mqtt() { return mqttClient; }

 private:
  MqttClient mqttClient;
};

typedef SensoraTransport<Client> Transp;
#endif