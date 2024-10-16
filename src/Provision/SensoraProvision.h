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

#ifndef SensoraProvision_h
#define SensoraProvision_h

#include <SensoraLink.h>

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

class SerialProvision {
 public:
  SerialProvision() : sensoraLink(), ps(ProvisionState::WaitNetworkConfig) {}

  void setup() {
    SENSORA_LOGD("setup serial at default baud rate 115200");
    // Serial.begin(115200);
  }

  void loop() {
    if (Serial.available() > 0) {
      uint8_t b = Serial.read();
      if (!sensoraLink.readByte(b)) {
        if (sensoraLink.parseBytes()) {
          CmdResponse resp = sensoraLink.command();
          handleSerialCmd(resp);
        } else {
          SENSORA_LOGE("error %d", sensoraLink.error());
          sendCmdError(sensoraLink.error());
        }
        sensoraLink.resetBuff();
      }
    }
  }
  ProvisionState state() { return ps; }
  void setState(ProvisionState state) { ps = state; }
  CmdResponse getCmd() { return cmd; }

 protected:
  SensoraLink sensoraLink;
  void sendCmdError(CmdError err) {
    SENSORA_LOGE("sendCmdError %d", err);
    if (err != CmdError::None) {
      SENSORA_LOGE("Sensora Link command error %d", err);
      uint8_t buff[13];
      uint8_t cByte = static_cast<uint8_t>(err);
      size_t buffSize = sensoraLink.buildSerialBuff(SensoraCmd::CommandError, &cByte, 1, buff);
      Serial.write(buff, buffSize);
    }
  }

 private:
  CmdResponse cmd;
  ProvisionState ps;
  void handleSerialCmd(CmdResponse data) {
    SENSORA_LOGD("received cmd %d", data.cmd);
    switch (data.cmd) {
      case SensoraCmd::SaveWiFiCredentials: {
        if (!validateWiFiCredentials(data.wifiCredentials.ssid, data.wifiCredentials.password)) {
          SENSORA_LOGE("Invalid WiFi credentials");
          sendCmdError(CmdError::InvalidNetwCredentials);
          break;
        }
        memcpy(&cmd, &data, sizeof(CmdResponse));
        setState(ProvisionState::ConnectNetwork);
        break;
      }
      case SensoraCmd::SaveDeviceCredentials: {
        if (!validateDeviceCredentials(data.deviceCredentials.deviceId, data.deviceCredentials.deviceToken)) {
          // TODO: send info that credentials are not correct
          sendCmdError(CmdError::InvalidDeviceCredentials);
          break;
        }
        memcpy(&cmd, &data, sizeof(CmdResponse));
        setState(ProvisionState::ConnectMqtt);
        break;
      }
      case SensoraCmd::EraseConfig: {
        // TODO: erase config
        break;
      }
      default:
        SENSORA_LOGW("Command is not handled %d", data.cmd);
    }
  }
};

#endif