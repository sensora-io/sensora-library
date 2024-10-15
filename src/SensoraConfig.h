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

#ifndef SensoraConfig_h
#define SensoraConfig_h

#define MQTT_HOST "mqtt.sensora.io"

#define SENSORA_MAX_DEVICE_ID_LEN 32 + 1
#define SENSORA_MAX_DEVICE_TOKEN_LEN 32 + 1
#define SENSORA_MAX_PROPERTY_ID_LEN 32 + 1

#define MAX_WIFI_SSID_LENGTH 32 + 1
#define MAX_WIFI_PASSWORD_LENGTH 64 + 1

#define DEVICE_MAX_ATTRIBUTES 20
#define DEVICE_MAX_PROPERTIES 10

#ifndef DEVICE_STATS_SYNC_INTERVAL_MS
#define DEVICE_STATS_SYNC_INTERVAL_MS 15000
#endif

#ifndef PROPERTY_BUFFER_SIZE
#define PROPERTY_BUFFER_SIZE 64
#endif

#ifndef SENSORA_LOG_LEVEL
#define SENSORA_LOG_LEVEL SensoraLogLevel::DEBUG
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

enum class DeviceStatus {
  Boot = 1,
  Online,
  Offline,
  Sleeping,
  Lost,
  Alert
};

#endif