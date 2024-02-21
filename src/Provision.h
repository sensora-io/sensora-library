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

#ifndef Provision_h
#define Provision_h

#include <ArduinoJson.h>
#include <SensoraLink.h>
#include <TickTwo.h>

enum class ProvisionState {
  WaitNetworkConfig = 1,

  ConnectNetwork,
  WaitNetwConn,
  NetworkConnFailure,

  WaitDeviceCredentials,

  ConnectMqtt,
  WaitMqttConn,
  MqttConnFailure,

  FinishProvision
};

class Provision {
 public:
  Provision(SensoraMqtt& mqtt) : _state(ProvisionState::WaitNetworkConfig), _mqtt(mqtt) {}
  void setup() {
    SENSORA_LOGI("provision setup");
  }

  void loop() {
    if (Serial.available() > 0) {
      uint8_t b = Serial.read();
      if (!_sensoraLink.readByte(b)) {
        if (_sensoraLink.parseBytes()) {
          CmdResponse resp = _sensoraLink.command();
          handleSerialCmd(resp);
        } else {
          handleCmdError(_sensoraLink.error());
        }
        _sensoraLink.resetBuff();
      }
    }
  }

 protected:
  union ConnectionConfig {
    WiFiConfig wifi;
    GSMConfig gsm;
  } _connConfig;

  ProvisionState _state;
  CmdError _provisionError;
  SensoraLink _sensoraLink;
  StaticJsonDocument<256> apiStatus;

  ProvisionState handleNetworkConnFailure() {
    SENSORA_LOGE("network connection failure %d", _provisionError);
    uint8_t buff[13];
    uint8_t cByte = static_cast<uint8_t>(_provisionError);
    size_t buffSize = _sensoraLink.buildSerialBuff(SensoraCmd::CommandError, &cByte, 1, buff);
    Serial.write(buff, buffSize);
    return ProvisionState::WaitNetworkConfig;
  }

  ProvisionState handleConnectMqtt() {
    _provisionError = CmdError::None;
    _mqtt.setup();
    _mqtt.connect();
    return ProvisionState::WaitMqttConn;
  }

  ProvisionState handleWaitMqttConn() {
    if (_firstMqttCheck == 0) {
      _firstMqttCheck = millis();
    }
    if (_mqtt.connected()) {
      uint8_t cByte = 0x01;
      uint8_t buff[13];
      size_t buffSize = _sensoraLink.buildSerialBuff(SensoraCmd::MqttStatus, &cByte, 1, buff);
      Serial.write(buff, buffSize);
      _firstMqttCheck = 0;
      return ProvisionState::FinishProvision;
    }
    if (millis() - _firstMqttCheck >= 10000LU) {
      SENSORA_LOGI("mqtt connection timeout");
      _provisionError = CmdError::MqttConnTimeout;
      _firstMqttCheck = 0;
      return ProvisionState::NetworkConnFailure;
    }
    return ProvisionState::WaitMqttConn;
  }
  ProvisionState handleMqttConnFailure() {
    SENSORA_LOGE("mqtt connection failure %d", _provisionError);
    uint8_t buff[13];
    uint8_t cByte = static_cast<uint8_t>(_provisionError);
    size_t buffSize = _sensoraLink.buildSerialBuff(SensoraCmd::CommandError, &cByte, 1, buff);
    Serial.write(buff, buffSize);
    return ProvisionState::WaitDeviceCredentials;
  }

  ProvisionState handleFinishProvision() {
    int saved = writeConfig(DEVICE_CONFIG_KEY, deviceConfig);
    saved = writeConfig(NETWORK_CONFIG_KEY, _connConfig.wifi);
    if (saved) {
      SENSORA_LOGI("Provision finished successfully, restarting...");
      hwRestart();
    }
    // wait 1 second and retry again
    delay(1000);
    return ProvisionState::FinishProvision;
  }

 private:
  SensoraMqtt& _mqtt;
  unsigned long _firstMqttCheck = 0;

  void handleSerialCmd(CmdResponse data) {
    switch (data.cmd) {
      case SensoraCmd::SaveWiFiCredentials: {
        if (!validateWiFiCredentials(data.wifiCredentials.ssid, data.wifiCredentials.password)) {
          SENSORA_LOGE("Invalid WiFi credentials");
          handleCmdError(CmdError::InvalidNetwCredentials);
          break;
        }
        initWiFiConfig(_connConfig.wifi, data.wifiCredentials.ssid, data.wifiCredentials.password);
        _state = ProvisionState::ConnectNetwork;
        break;
      }
      case SensoraCmd::SaveDeviceCredentials: {
        if (!validateDeviceCredentials(data.deviceCredentials.deviceId, data.deviceCredentials.deviceToken)) {
          // TODO: send info that credentials are not correct
          handleCmdError(CmdError::InvalidDeviceCredentials);
          break;
        }

        initDeviceCredentials(data.deviceCredentials.deviceId, data.deviceCredentials.deviceToken);
        _state = ProvisionState::ConnectMqtt;
        break;
      }
      case SensoraCmd::EraseConfig: {
        bool erased = resetConfig();
        SENSORA_LOGI("ERASED CONFIG %d", erased);
        break;
      }
      default:
        SENSORA_LOGW("Command is not handled %d", data.cmd);
    }
  }

  void handleCmdError(CmdError err) {
    if (err != CmdError::None) {
      SENSORA_LOGE("sensora link command error %d", err);
      uint8_t buff[13];
      uint8_t cByte = static_cast<uint8_t>(err);
      size_t buffSize = _sensoraLink.buildSerialBuff(SensoraCmd::CommandError, &cByte, 1, buff);
      Serial.write(buff, buffSize);
    }
  }
};
#endif