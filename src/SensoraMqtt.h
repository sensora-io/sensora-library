/*
 * Copyright 2019-2023 Sensora LLC
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

#ifndef SensoraMqtt_h
#define SensoraMqtt_h

#include <ArduinoMqttClient.h>

const char* mqttHost = "mqtt.sensora.io";

class SensoraMqtt {
 public:
  SensoraMqtt(Client& client) : _mqttClient(client) {}

  void setup() {
    SENSORA_LOGD("called mqtt setup, deviceId '%s'", deviceConfig.deviceId);
    _mqttClient.setId(deviceConfig.deviceId);
    _mqttClient.setUsernamePassword("", deviceConfig.deviceToken);
    _mqttClient.setKeepAliveInterval(15 * 1000L);

    char willTopic[42];
    snprintf(willTopic, sizeof(willTopic), "sc/%s", deviceConfig.deviceId);
    SensoraPayload p;
    p.add("status=");
    p.add(static_cast<uint8_t>(DeviceStatus::Lost));
    _mqttClient.beginWill(willTopic, true, 1);
    _mqttClient.write(p.buffer(), p.length());
    _mqttClient.endWill();
  }

  void onMessage(void (*cb)(int)) {
    _mqttClient.onMessage(cb);
  }

  bool connect() {
    SENSORA_LOGD("connecting to Sensora Cloud");
    if (_mqttClient.connected()) {
      return true;
    }
    if (!_mqttClient.connect(mqttHost, 1883)) {
      SENSORA_LOGE("failed to connect to Sensora Cloud, code %d", _mqttClient.connectError());
      return false;
    } else {
      SENSORA_LOGI("connected to Sensora Cloud");
      return true;
    }
  }

  bool connected() {
    return _mqttClient.connected();
  }

  bool publish(const char* topic, uint8_t* buf, unsigned long size) {
    if (size == 0) {
      SENSORA_LOGW("cannot publish mqtt paylod with size 0");
      return false;
    }
    if (_mqttClient.beginMessage(topic, size, false, 0)) {
      if (_mqttClient.write(buf, size)) {
        if (_mqttClient.endMessage()) {
          return true;
        }
      }
    }
    return false;
  }

  int subscribe(const char* topic) {
    return _mqttClient.subscribe(topic, 1);
  }

  MqttClient& client() { return _mqttClient; }

 private:
  MqttClient _mqttClient;
};

#endif