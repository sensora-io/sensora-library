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

#ifndef EspWifi_h
#define EspWifi_h

#include <WiFi.h>
#include <SensoraDevice.h>
#include <Storage/StoragePreferences.h>
#include <Provision/SensoraProvision.h>

class EspProvision : public SerialProvision {
 public:
  EspProvision(Transp& transport) : SerialProvision(), transp(transport) {}

  void setup() {
    SerialProvision::setup();
  }

  void loop() {
    ProvisionState currentState = state();
    switch (currentState) {
      case ProvisionState::WaitNetworkConfig:
        SerialProvision::loop();
        break;
      case ProvisionState::ConnectNetwork:
        handleConnectNetwork();
        break;
      case ProvisionState::WaitNetwConn:
        handleWaitNetwConn();
        break;
      case ProvisionState::NetworkConnFailure:
        handleNetworkConnFailure();
        break;
      case ProvisionState::WaitDeviceCredentials:
        SerialProvision::loop();
        // device credentials may be initialized from HTTP API
        // if (_httpProvision && validateDeviceCredentials(deviceConfig.deviceId, deviceConfig.deviceToken)) {
        //   newState = ProvisionState::ConnectMqtt;
        // }
        break;
      case ProvisionState::ConnectMqtt:
        handleConnectMqtt();
        break;
      case ProvisionState::WaitMqttConn:
        handleWaitMqttConn();
        break;
      case ProvisionState::MqttConnFailure:
        handleMqttConnFailure();
        break;
      case ProvisionState::FinishProvision:
        handleFinishProvision();
        break;
    }
  }

 private:
  WiFiConfig wifiConfig;
  Transp& transp;
  CmdError cmdError;
  unsigned long firstNetworkCheck = 0;
  unsigned long firstMqttCheck = 0;

  void handleConnectNetwork() {
    WiFiConfig cfg = getCmd().wifiCredentials;
    SENSORA_LOGD("connecting to network ssid '%s'", cfg.ssid);
    cmdError = CmdError::None;
    if (deviceConfig.connectionType != ConnectionType::WiFi) {
      sendCmdError(CmdError::NetworkConnMismatch);
      setState(ProvisionState::NetworkConnFailure);
      return;
    }
    SENSORA_LOGD("connecting to network ssid '%s'", cfg.ssid);
    WiFi.disconnect();
    if (!WiFi.begin(cfg.ssid, cfg.password)) {
      SENSORA_LOGE("failed to connect to ssid '%s'", cfg.ssid);
      sendCmdError(CmdError::InvalidNetwCredentials);
      setState(ProvisionState::NetworkConnFailure);
      return;
    }
    copyString(cfg.ssid, wifiConfig.ssid);
    copyString(cfg.password, wifiConfig.password);
    setState(ProvisionState::WaitNetwConn);
  }

  void handleWaitNetwConn() {
    if (firstNetworkCheck == 0) {
      firstNetworkCheck = millis();
    }
    if (WiFi.status() == WL_CONNECTED) {
      SENSORA_LOGD("network connected");
      uint8_t cByte = 0x01;
      uint8_t buff[13];
      size_t buffSize = sensoraLink.buildSerialBuff(SensoraCmd::NetworkStatus, &cByte, 1, buff);
      Serial.write(buff, buffSize);
      firstNetworkCheck = 0;
      setState(ProvisionState::WaitDeviceCredentials);
      return;
    }
    if (millis() - firstNetworkCheck >= 10000LU) {
      sendCmdError(CmdError::NetworkConnTimeout);
      firstNetworkCheck = 0;
      setState(ProvisionState::NetworkConnFailure);
      return;
    }
    setState(ProvisionState::WaitNetwConn);
  }

  void handleNetworkConnFailure() {
    SENSORA_LOGE("network connection failure %d", cmdError);
    uint8_t buff[13];
    uint8_t cByte = static_cast<uint8_t>(cmdError);
    size_t buffSize = sensoraLink.buildSerialBuff(SensoraCmd::CommandError, &cByte, 1, buff);
    Serial.write(buff, buffSize);
    setState(ProvisionState::WaitNetworkConfig);
  }

  void handleConnectMqtt() {
    DeviceConfig cfg = getCmd().deviceCredentials;
    copyString(cfg.deviceId, deviceConfig.deviceId);
    copyString(cfg.deviceToken, deviceConfig.deviceToken);
    cmdError = CmdError::None;
    transp.setup();
    transp.connect();
    setState(ProvisionState::WaitMqttConn);
  }

  void handleWaitMqttConn() {
    if (firstMqttCheck == 0) {
      firstMqttCheck = millis();
    }
    if (transp.connected()) {
      uint8_t cByte = 0x01;
      uint8_t buff[13];
      size_t buffSize = sensoraLink.buildSerialBuff(SensoraCmd::MqttStatus, &cByte, 1, buff);
      Serial.write(buff, buffSize);
      firstMqttCheck = 0;
      setState(ProvisionState::FinishProvision);
      return;
    }
    if (millis() - firstMqttCheck >= 10000LU) {
      SENSORA_LOGI("mqtt connection timeout");
      cmdError = CmdError::MqttConnTimeout;
      firstMqttCheck = 0;
      setState(ProvisionState::MqttConnFailure);
      copyString("", deviceConfig.deviceId);
      copyString("", deviceConfig.deviceToken);
      return;
    }
    setState(ProvisionState::WaitMqttConn);
  }

  void handleMqttConnFailure() {
    SENSORA_LOGE("mqtt connection failure %d", cmdError);
    uint8_t buff[13];
    uint8_t cByte = static_cast<uint8_t>(cmdError);
    size_t buffSize = sensoraLink.buildSerialBuff(SensoraCmd::CommandError, &cByte, 1, buff);
    Serial.write(buff, buffSize);
    setState(ProvisionState::WaitDeviceCredentials);
  }

  void handleFinishProvision() {
    writeConfig("device", deviceConfig);
    writeConfig("netw", wifiConfig);
    ESP.restart();
    while (true) {
    }
  }
};

class EspWiFi {
 public:
  EspWiFi() : provision(nullptr) {
  }

  void setup() {
    initStorage();
    loadConfig();
  }

  bool isProvision() {
    if (validateDeviceCredentials(deviceConfig.deviceId, deviceConfig.deviceToken) &&
        validateWiFiCredentials(wifiConfig.ssid, wifiConfig.password)) {
      return false;
    }
    return true;
  }

  void setupProvision(Transp& transport) {
    SENSORA_LOGD("setting up ESP provision");
    provision = new EspProvision(transport);
    provision->setup();
  }

  void loopProvision() {
    if (provision) {
      provision->loop();
    }
  }

  void connectNetwork() {
    if (isNetworkConnected()) {
      return;
    }
    if (strlen(wifiConfig.password)) {
      WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    } else {
      WiFi.begin(wifiConfig.ssid);
    }
  }

  void readInfo(SensoraPayload& payload) {
    IPAddress ip = WiFi.localIP();
    payload.add("ip", ip.toString());
    payload.add("mac", WiFi.macAddress());
  }

  void readStats(SensoraPayload& payload) {
    payload.add("wifi_signal", WiFi.RSSI());
    payload.add("free_heap", String(ESP.getFreeHeap()));
  }

  bool isNetworkConnected() {
    return WiFi.status() == WL_CONNECTED;
  }

 private:
  void initStorage() {
    SENSORA_LOGD("EspWifi setup storage");
    storageBegin();
  }

  void loadConfig() {
    SENSORA_LOGD("init config");
    readConfig("device", deviceConfig);
    readConfig("netw", wifiConfig);
    deviceConfig.connectionType = ConnectionType::WiFi;
  }

  Preferences preferences;
  WiFiConfig wifiConfig;
  EspProvision* provision;
};

WiFiClient wifiClient;
Transp transport(wifiClient);
SensoraDevice<EspWiFi> Sensora(transport);

template <>
void SensoraDevice<EspWiFi>::onMessage(int len) {
  Sensora.handleMessage(len);
}

#endif