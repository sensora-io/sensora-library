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

#ifndef Config_h
#define Config_h

#ifdef ESP8266
#include <EEPROM.h>
const int EEPROM_START = 0;
#elif defined(ESP32)
#include <Preferences.h>
Preferences preferences;
#endif

#define DEVICE_CONFIG_KEY "device"
#define NETWORK_CONFIG_KEY "netw"

#define SENSORA_MAX_NETWORK_CONNECTION_ATTEMPTS 20
#define SENSORA_MAX_DEVICE_ID_LEN 32 + 1
#define SENSORA_MAX_DEVICE_TOKEN_LEN 32 + 1

#define SENSORA_MAX_PROPERTY_ID_LEN 32 + 1

#define MAX_WIFI_SSID_LENGTH 32 + 1
#define MAX_WIFI_PASSWORD_LENGTH 64 + 1
#define MAX_GSM_APN_LENGTH 100 + 1
#define MAX_GSM_USERNAME_LENGTH 64 + 1
#define MAX_GSM_PASSWORD_LENGTH 64 + 1

#ifndef SERIAL_BAUD_RATE
#define SERIAL_BAUD_RATE 115200
#endif

#ifndef MAX_PROPERTIES
#define MAX_PROPERTIES 20
#endif

#ifndef PROPERTY_BUFFER_SIZE
#define PROPERTY_BUFFER_SIZE 64
#endif

#ifndef DEVICE_STATS_SYNC_INTERVAL_MS
#define DEVICE_STATS_SYNC_INTERVAL_MS 15000
#endif

enum class ConnectionType {
  Unknown,
  WiFi,
  Ethernet,
  GSM
};

struct WiFiConfig {
  char ssid[MAX_WIFI_SSID_LENGTH];
  char password[MAX_WIFI_PASSWORD_LENGTH];
};

struct GSMConfig {
  char apn[MAX_GSM_APN_LENGTH];
  char username[MAX_GSM_APN_LENGTH];
  char password[MAX_GSM_PASSWORD_LENGTH];
};

struct DeviceConfig {
  char deviceId[SENSORA_MAX_DEVICE_ID_LEN];
  char deviceToken[SENSORA_MAX_DEVICE_TOKEN_LEN];
  ConnectionType connectionType;
};

const DeviceConfig defaultConfig = {
    "",
    "",
    ConnectionType::Unknown,
};

DeviceConfig deviceConfig = defaultConfig;

template <typename T, size_t N>
void copyString(const char* source, T (&dest)[N]) {
  strncpy(dest, source, N);
  dest[N - 1] = '\0';
}

template <typename T>
void readConfig(const char* key, T& config) {
#ifdef ESP8266
  if (strcmp(key, DEVICE_CONFIG_KEY) == 0) {
    EEPROM.get(EEPROM_START, config);
    return;
  }

  if (strcmp(key, NETWORK_CONFIG_KEY) == 0) {
    EEPROM.get(EEPROM_START + sizeof(DeviceConfig), config);
    return;
  }
#elif defined(ESP32)
  preferences.getBytes(key, &config, sizeof(T));
#endif
}

template <typename T>
bool writeConfig(const char* key, const T& config) {
#ifdef ESP8266
  if (strcmp(key, DEVICE_CONFIG_KEY) == 0) {
    EEPROM.put(EEPROM_START, config);
    return EEPROM.commit();
  }

  if (strcmp(key, NETWORK_CONFIG_KEY) == 0) {
    EEPROM.put(EEPROM_START + sizeof(DeviceConfig), config);
    return EEPROM.commit();
  }
  return false;
#elif defined(ESP32)
  return preferences.putBytes(key, &config, sizeof(T));
#endif
}

bool resetConfig() {
#ifdef ESP8266
  if (!writeConfig(DEVICE_CONFIG_KEY, defaultConfig)) {
    return false;
  }
  deviceConfig = defaultConfig;
  WiFiConfig wifiConfig = {};
  return writeConfig(NETWORK_CONFIG_KEY, wifiConfig);
#elif defined(ESP32)
  return preferences.clear();
#endif
}

void initConfig() {
#ifdef ESP8266
  EEPROM.begin(sizeof(DeviceConfig) + max(sizeof(WiFiConfig), sizeof(GSMConfig)));
#elif defined(ESP32)
  preferences.begin("sensora", false);
#endif
}

void loadConfig() {
  readConfig(DEVICE_CONFIG_KEY, deviceConfig);
}

#endif