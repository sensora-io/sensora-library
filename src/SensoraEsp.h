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

#ifndef SensoraEsp_h
#define SensoraEsp_h

void hwRestart();
#ifdef ESP32
#include <HTTPClient.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#endif
#include <Sensora.h>

const IPAddress WIFI_AP_IP(192, 168, 4, 1);
const IPAddress WIFI_AP_SUBNET(255, 255, 255, 0);

class EspProvision : public Provision {
 public:
  EspProvision(SensoraMqtt& mqtt) : Provision(mqtt), server(80) {}
  void setup() {
    Provision::setup();
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_IP, WIFI_AP_SUBNET);
    WiFi.softAP("Sensora IoT");

    SENSORA_LOGI("Started AP 'Sensora IoT'");
    server.on("/config", HTTP_POST, [this]() {
      String reqBody = server.arg("plain");
      DynamicJsonDocument payload(1024);

      DeserializationError error = deserializeJson(payload, reqBody);
      if (error) {
        SENSORA_LOGE("deserializeJson failed, %s", error.f_str());
        server.send(400, "application/json", R"json({"status": "ERROR", "message": "Invalid JSON"})json");
        return;
      }

      const char* deviceId = payload["deviceId"];
      const char* deviceToken = payload["deviceToken"];
      if (!validateDeviceCredentials(deviceId, deviceToken)) {
        return server.send(400, "application/json", R"json({"status": "ERROR", "message": "Invalid Device credentials"})json");
      }

      initDeviceCredentials(deviceId, deviceToken);
      if (payload.containsKey("wifi")) {
        JsonObject wifiCfg = payload["wifi"];
        if (!wifiCfg.containsKey("ssid") || !wifiCfg.containsKey("password")) {
          return server.send(400, "application/json", R"json({"status": "ERROR", "message": "Missing ssid or password"})json");
        }

        const char* ssid = wifiCfg["ssid"];
        const char* password = wifiCfg["password"];
        if (!validateWiFiCredentials(ssid, password)) {
          return server.send(400, "application/json", R"json({"status": "ERROR", "message": "Invalid WiFi credentials"})json");
        }
        initWiFiConfig(_connConfig.wifi, ssid, password);
        _httpProvision = true;
        _state = ProvisionState::ConnectNetwork;
        server.send(200, "application/json", R"json({"status": "OK"})json");
        return;
      }
      server.send(400, "application/json", R"json({"status": "ERROR", "message": "Invalid configuration"})json");
    });

    server.on("/status", HTTP_GET, [this]() {
      StaticJsonDocument<256> doc;
      bool validDeviceCredentials = validateDeviceCredentials(deviceConfig.deviceId, deviceConfig.deviceToken);
      if (!validDeviceCredentials) {
        _provisionError = CmdError::InvalidNetwCredentials;
      } else {
        if (!validateWiFiCredentials(_connConfig.wifi.ssid, _connConfig.wifi.password)) {
          _provisionError = CmdError::InvalidDeviceCredentials;
        }
      }
      SENSORA_LOGW("current state %d", _state);
      switch (_provisionError) {
        case CmdError::None:
          break;
        case CmdError::NetworkConnMismatch:
          doc["network"] = "Board does not support WiFi connection type";
          break;
        case CmdError::InvalidNetwCredentials:
          doc["network"] = "Invalid WiFi credentials";
          break;
        case CmdError::NetworkConnTimeout:
          doc["network"] = "Network connection timeout, please try again";
          break;
        case CmdError::InvalidDeviceCredentials:
          doc["cloud"] = "Failed to connect to Sensora Cloud, please check device credentials";
          break;
        case CmdError::MqttConnTimeout:
          doc["cloud"] = "Sensora Cloud connection timeout, please try again";
          break;
        case CmdError::InvalidCommand:
          break;
        case CmdError::CRCMismatch:
          break;
        case CmdError::InvalidData:
          break;
      }

      String resp;
      serializeJson(doc, resp);
      server.send(400, "application/json", resp);
    });
    server.begin(80);
    SENSORA_LOGI("started HTTP Server on port %s:%d", WIFI_AP_IP.toString().c_str(), 80);
  }

  void loop() {
    Provision::loop();
    server.handleClient();
    delay(2);
    ProvisionState newState = _state;
    switch (_state) {
      case ProvisionState::WaitNetworkConfig:
        break;
      case ProvisionState::ConnectNetwork:
        newState = handleConnectNetwork();
        break;
      case ProvisionState::WaitNetwConn:
        newState = handleWaitNetwConn();
        break;
      case ProvisionState::NetworkConnFailure:
        newState = handleNetworkConnFailure();
        break;
      case ProvisionState::WaitDeviceCredentials:
        // device credentials may be initialized from HTTP API
        if (_httpProvision && validateDeviceCredentials(deviceConfig.deviceId, deviceConfig.deviceToken)) {
          newState = ProvisionState::ConnectMqtt;
        }
        break;
      case ProvisionState::ConnectMqtt:
        newState = handleConnectMqtt();
        break;
      case ProvisionState::WaitMqttConn:
        newState = handleWaitMqttConn();
        break;
      case ProvisionState::MqttConnFailure:
        newState = handleMqttConnFailure();
        break;
      case ProvisionState::FinishProvision:
        newState = handleFinishProvision();
        break;
    }
    _state = newState;
  }

 private:
#ifdef ESP32
  WebServer server;
#elif defined(ESP8266)
  ESP8266WebServer server;
#endif
  unsigned long _firstNetworkCheck = 0;
  bool _httpProvision = false;
  ProvisionState handleConnectNetwork() {
    _provisionError = CmdError::None;
    if (deviceConfig.connectionType != ConnectionType::WiFi) {
      _provisionError = CmdError::NetworkConnMismatch;
      return ProvisionState::NetworkConnFailure;
    }
    SENSORA_LOGD("connecting to network ssid '%s'", _connConfig.wifi.ssid);
    WiFi.disconnect();
    if (!WiFi.begin(_connConfig.wifi.ssid, _connConfig.wifi.password)) {
      SENSORA_LOGE("failed to connect to ssid '%s'", _connConfig.wifi.ssid);
      _provisionError = CmdError::InvalidNetwCredentials;
      return ProvisionState::NetworkConnFailure;
    }
    return ProvisionState::WaitNetwConn;
  }

  ProvisionState handleWaitNetwConn() {
    if (_firstNetworkCheck == 0) {
      _firstNetworkCheck = millis();
    }
    if (WiFi.status() == WL_CONNECTED) {
      uint8_t cByte = 0x01;
      uint8_t buff[13];
      size_t buffSize = _sensoraLink.buildSerialBuff(SensoraCmd::NetworkStatus, &cByte, 1, buff);
      Serial.write(buff, buffSize);
      _firstNetworkCheck = 0;
      return ProvisionState::WaitDeviceCredentials;
    }
    if (millis() - _firstNetworkCheck >= 10000LU) {
      _provisionError = CmdError::NetworkConnTimeout;
      _firstNetworkCheck = 0;
      return ProvisionState::NetworkConnFailure;
    }
    return ProvisionState::WaitNetwConn;
  }
};

class EspDevice {
 public:
  EspDevice() {}

  void connectNetwork() {
    if (deviceConfig.connectionType != ConnectionType::WiFi) {
      SENSORA_LOGE("Missing WiFi configuration");
      return;
    }
    if (networkConnected()) {
      return;
    }
    WiFiConfig config;
    readConfig(NETWORK_CONFIG_KEY, config);
    SENSORA_LOGI("Connecting to WiFi '%s'", config.ssid);
    if (config.password && strlen(config.password)) {
      WiFi.begin(config.ssid, config.password);
    } else {
      WiFi.begin(config.ssid);
    }
  }

  bool networkConnected() { return WiFi.status() == WL_CONNECTED; }

  void setupProvision(SensoraMqtt& mqtt) {
    if (_provision != nullptr) {
      delete _provision;
    }
    _provision = new EspProvision(mqtt);
    _provision->setup();
  }

  void loopProvision() {
    if (_provision != nullptr) {
      _provision->loop();
    }
  }

  void readInfo(SensoraPayload& p) {
    p.add("mac=");
    p.add(WiFi.macAddress().c_str());
    p.add(";");
  }

  void readStats(SensoraPayload& p) {
    p.add("freeheap=");
    p.add(ESP.getFreeHeap());
    p.add(";");
  }

  IPAddress getLocalIP() {
    return WiFi.localIP();
  }

 private:
  EspProvision* _provision;
};

WiFiClient client;
SensoraDevice<EspDevice> device(client);
SensoraClass<EspDevice> Sensora(device);

void handleMqttMessage(int length) {
  device.onMqttMessage(length);
}

void hwRestart() {
  ESP.restart();
  while (true) {
  }
}

#endif