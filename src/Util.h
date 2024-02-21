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

#ifndef Util_h
#define Util_h

void initDeviceCredentials(const char* deviceId, const char* deviceToken) {
  SENSORA_LOGD("init_device_credentials deviceId '%s'", deviceId);
  copyString(deviceId, deviceConfig.deviceId);
  copyString(deviceToken, deviceConfig.deviceToken);
}

bool validateDeviceCredentials(const char* deviceId, const char* deviceToken) {
  if (strlen(deviceId) != 32) {
    SENSORA_LOGD("INVALID deviceId length %d", strlen(deviceId));
    return false;
  }
  if (strlen(deviceToken) != 32) {
    SENSORA_LOGD("INVALID deviceToken");
    return false;
  }
  return true;
}

void initWiFiConfig(WiFiConfig& cfg, const char* ssid, const char* password) {
  deviceConfig.connectionType = ConnectionType::WiFi;
  copyString(ssid, cfg.ssid);
  copyString(password, cfg.password);
}

bool validateWiFiCredentials(const char* ssid, const char* password) {
  if (ssid == NULL || password == NULL) {
    return false;
  }
  size_t ssidLength = strlen(ssid);
  if (ssidLength < 1 || ssidLength > 32) {
    SENSORA_LOGD("INVALID ssidLength %d", ssidLength);
    return false;
  }

  size_t passwordLength = strlen(password);
  if (passwordLength < 8 || passwordLength > 63) {
    return false;
  }
  return true;
}

bool isProvisionMode() {
  if (!validateDeviceCredentials(deviceConfig.deviceId, deviceConfig.deviceToken)) {
    return true;
  }

  return false;
  switch (deviceConfig.connectionType) {
    case ConnectionType::WiFi: {
      WiFiConfig wifi;
      readConfig(NETWORK_CONFIG_KEY, wifi);
      return !validateWiFiCredentials(wifi.ssid, wifi.password);
    }
    default:
      SENSORA_LOGE("Connection %d is not supported yet", deviceConfig.connectionType);
      return true;
  }
}

void parseTopic(const char* topic, char* deviceId, char* category, char* identifier) {
  SENSORA_LOGV("parse topic '%s'", topic);
  const char* cursor = topic + 3;
  size_t length = 0;

  char* slashPtr = strchr(cursor, '/');
  if (slashPtr == NULL) {
    SENSORA_LOGE("Cannot extract deviceId from topic");
    return;
  }
  length = slashPtr - cursor;
  memcpy(deviceId, cursor, length);
  deviceId[length] = '\0';
  cursor = slashPtr + 1;

  slashPtr = strchr(cursor, '/');
  if (slashPtr == NULL) {
    SENSORA_LOGE("Cannot extract category from topic");
    return;
  }
  length = slashPtr - cursor;
  memcpy(category, cursor, length);
  category[length] = '\0';
  cursor = slashPtr + 1;

  slashPtr = strchr(cursor, '/');
  if(slashPtr == NULL) {
    SENSORA_LOGE("Cannot extract identifier from topic");
    return;
  }
  length = slashPtr - cursor;
  memcpy(identifier, cursor, length);
  identifier[length] = '\0';
}

#endif