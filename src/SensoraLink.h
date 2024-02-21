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

#ifndef SensoraLink_h
#define SensoraLink_h

static const uint8_t sof[] = {0x73, 0x65, 0x6E, 0x73, 0x6F, 0x72, 0x61, 0x01};

enum class CmdError : uint8_t {
  None = 0x00,
  InvalidData = 0x01,
  CRCMismatch = 0x02,
  InvalidCommand = 0x03,

  NetworkConnMismatch = 0x04,
  InvalidNetwCredentials = 0x05,
  NetworkConnTimeout = 0x07,

  InvalidDeviceCredentials = 0x08,
  MqttConnTimeout = 0x09,
};

enum class SensoraCmd : uint8_t {
  CommandError = 0x00,
  ReadDeviceState = 0x01,
  SaveWiFiCredentials = 0x02,
  SaveDeviceCredentials = 0x03,
  ScanWifiNetworks = 0x04,
  EraseConfig = 0x05,
  NetworkStatus = 0x06,
  MqttStatus = 0x07
};

struct CmdResponse {
  SensoraCmd cmd;
  union {
    DeviceConfig deviceCredentials;
    WiFiConfig wifiCredentials;
  };
};

class SensoraLink {
 public:
  SensoraLink() : buffPos(0), startTime(0), totalBytes(0), cmdError(CmdError::None) {}
  bool readByte(uint8_t b) {
    totalBytes++;
    if (buffPos > 0 && millis() - startTime > 1000LU) {
      SENSORA_LOGE("reseting serial, timed out");
      resetBuff();
      return false;
    }
    if (buffPos == 0) {
      startTime = millis();
    }
    if (buffPos >= 128) {
      resetBuff();
      return false;
    }
    // SENSORA_LOGV("SensoraLink readByte: 0x%02X , buffPos %d, sof@buffPoss 0x%02X", b, buffPos, sof[buffPos]);
    buff[buffPos++] = b;
    if (b == 0x99) {
      return false;
    }
    return true;
  }

  bool parseBytes() {
    if (buffPos < 10) {
      cmdError = CmdError::InvalidData;
      resetBuff();
      return false;
    }

    if (memcmp(buff, sof, 8) != 0) {
      cmdError = CmdError::InvalidData;
      resetBuff();
      return false;
    }

    size_t dataLen = buff[8];
    if (buffPos != 12 + dataLen) {
      cmdError = CmdError::InvalidData;
      resetBuff();
      return false;
    }

    if (dataLen > 255) {
      cmdError = CmdError::InvalidData;
      resetBuff();
      return false;
    }

    uint8_t dataBuffer[dataLen];
    for (size_t i = 0; i < dataLen; i++) {
      dataBuffer[i] = buff[9 + i];
    }

    uint8_t packetCRC = buff[dataLen + 9];
    if (!crcMatch(dataBuffer, dataLen, packetCRC)) {
      resetBuff();
      cmdError = CmdError::CRCMismatch;
      return false;
    }

    SensoraCmd packetCmd = static_cast<SensoraCmd>(buff[dataLen + 10]);
    cmdResp.cmd = packetCmd;
    if (packetCmd == SensoraCmd::SaveWiFiCredentials) {
      WiFiConfig cfg;

      uint8_t ssidLength = dataBuffer[0];
      memcpy(cfg.ssid, &dataBuffer[1], ssidLength);
      cfg.ssid[ssidLength] = '\0';

      uint8_t passwordLength = dataBuffer[ssidLength + 1];
      memcpy(cfg.password, &dataBuffer[ssidLength + 2], passwordLength);
      cfg.password[passwordLength] = '\0';

      cmdResp.wifiCredentials = cfg;
      return true;
    }

    if (packetCmd == SensoraCmd::SaveDeviceCredentials) {
      DeviceConfig cfg;

      uint8_t deviceIdLen = dataBuffer[0];
      SENSORA_LOGD("device id length %d", deviceIdLen);
      memcpy(cfg.deviceId, &dataBuffer[1], deviceIdLen);
      cfg.deviceId[deviceIdLen] = '\0';

      uint8_t deviceTokenLen = dataBuffer[deviceIdLen + 1];
      memcpy(cfg.deviceToken, &dataBuffer[deviceIdLen + 2], deviceTokenLen);
      cfg.deviceToken[deviceTokenLen] = '\0';

      cmdResp.deviceCredentials = cfg;
      return true;
    }

    if (packetCmd == SensoraCmd::ScanWifiNetworks) {
      return true;
    }

    if (packetCmd == SensoraCmd::EraseConfig) {
      return true;
    }

    if (packetCmd == SensoraCmd::ReadDeviceState) {
      return true;
    }

    cmdError = CmdError::InvalidCommand;
    resetBuff();
    return false;
  }

  CmdError error() { return cmdError; }
  CmdResponse command() { return cmdResp; }

  void resetBuff() {
    buffPos = 0;
    totalBytes = 0;
    memset(buff, 0, 128);
  }

  size_t buildSerialBuff(SensoraCmd cmd, const uint8_t* data, size_t dataSize, uint8_t* buff) {
    if (!data || !buff) {
      return 0;
    }
    size_t packets = 12 + dataSize;
    uint8_t crc = 0;

    for (size_t i = 0; i < dataSize; ++i) {
      crc += data[i];
    }
    crc &= 0xFF;

    size_t index = 0;
    for (size_t i = 0; i < 8; ++i) {
      buff[index++] = sof[i];
    }
    buff[index++] = static_cast<uint8_t>(dataSize);
    for (size_t i = 0; i < dataSize; ++i) {
      buff[index++] = data[i];
    }

    buff[index++] = crc;
    buff[index++] = static_cast<uint8_t>(cmd);
    buff[index++] = 0x99;

    return packets;
  }

 private:
  uint8_t buff[128];
  uint8_t buffPos;
  unsigned long startTime;
  int totalBytes;
  CmdResponse cmdResp;
  CmdError cmdError;

  bool crcMatch(const uint8_t* buffer, size_t length, uint8_t packetCRC) {
    uint8_t crc = 0;
    for (size_t i = 0; i < length; i++) {
      crc += buffer[i];
    }
    return (crc & 0xFF) == packetCRC;
  }
};

#endif