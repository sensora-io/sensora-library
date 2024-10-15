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

#ifndef SensoraUtil_h
#define SensoraUtil_h

template <typename T, size_t N>
void copyString(const char* source, T (&dest)[N]) {
  strncpy(dest, source, N);
  dest[N - 1] = '\0';
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

void extractDeviceId(const char* topic, char* deviceId) {
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
}

bool extractPayload(const uint8_t* bytes, int length, const char* key, char* buff, size_t bufLen) {
  int keyLen = strlen(key);
  int i = 0;
  while (i < length) {
    if (i + keyLen + 1 < length && memcmp(bytes + i, key, keyLen) == 0 && bytes[i + keyLen] == '=') {
      int j = 0;
      i += keyLen + 1;
      while (i < length && bytes[i] != ';' && j < bufLen - 1) {
        buff[j++] = bytes[i++];
      }
      buff[j] = '\0';
      return true;
    }

    while (i < length && bytes[i] != ';') {
      i++;
    }
    i++;
  }

  buff[0] = '\0';
  return false;
}

void printLogo() {
  SENSORA_LOGW("*******************************************************");
  SENSORA_LOGW("*  ____                                               *");
  SENSORA_LOGW("* / ___|    ___   _ __    ___    ___    _ __    __ _  *");
  SENSORA_LOGW("* \\___ \\   / _ \\ | '_ \\  / __|  / _ \\  | '__|  / _` | *");
  SENSORA_LOGW("*  ___) | |  __/ | | | | \\__ \\ | (_) | | |    | (_| | *");
  SENSORA_LOGW("* |____/   \\___| |_| |_| |___/  \\___/  |_|     \\__,_| *");
  SENSORA_LOGW("*                                                     *");
  SENSORA_LOGW("*    Go to https://docs.sensora.io to get started!    *");
  SENSORA_LOGW("*                                                     *");
  SENSORA_LOGW("*******************************************************");
}
#endif