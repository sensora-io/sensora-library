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

#ifndef SerialProvision_h
#define SerialProvision_h

#include <Arduino.h>

class SerialProvision {
 public:
  SerialProvision() {}

  void setup() {
  }

  void loop() {
    if (Serial.available() > 0) {
      uint8_t b = Serial.read();
      if (!sensoraLink.readByte(b)) {
        if (sensoraLink.parseBytes()) {
          CmdResponse resp = sensoraLink.command();
          handleSerialCmd(resp);
        } else {
          handleCmdError(sensoraLink.error());
        }
        sensoraLink.resetBuff();
      }
    }
  }

 private:
  CmdError _provisionError;
  SensoraLink sensoraLink;

  void handleSerialCmd(CmdResponse data) {
    switch (data.cmd) {
      case SensoraCmd::SaveWiFiCredentials: {
        if (!validateWiFiCredentials(data.wifiCredentials.ssid, data.wifiCredentials.password)) {
          SENSORA_LOGE("Invalid WiFi credentials");
          handleCmdError(CmdError::InvalidNetwCredentials);
          break;
        }
        break;
      }
      case SensoraCmd::SaveDeviceCredentials: {
        if (!validateDeviceCredentials(data.deviceCredentials.deviceId, data.deviceCredentials.deviceToken)) {
          handleCmdError(CmdError::InvalidDeviceCredentials);
          break;
        }
        initDeviceCredentials(data.deviceCredentials.deviceId, data.deviceCredentials.deviceToken);
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

  void handleCmdError(CmdError err) {
    if (err != CmdError::None) {
      SENSORA_LOGE("sensora link command error %d", err);
      uint8_t buff[13];
      uint8_t cByte = static_cast<uint8_t>(err);
      size_t buffSize = sensoraLink.buildSerialBuff(SensoraCmd::CommandError, &cByte, 1, buff);
      Serial.write(buff, buffSize);
    }
  }
};

#endif
