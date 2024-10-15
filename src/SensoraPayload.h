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

#ifndef SensoraPayload_h
#define SensoraPayload_h

#include <WString.h>
#define SENSORA_PAYLOAD_SIZE 128

class SensoraPayload {
 public:
  SensoraPayload() : bufLen(0) {}

  bool add(const char* key, const String& value) {
    return addSafe(key, value.c_str());
  }

  bool add(const char* key, const char* value) {
    return addSafe(key, value);
  }

  bool add(const char* key, int8_t value) {
    char buffer[5];
    snprintf(buffer, sizeof(buffer), "%d", value);
    return addSafe(key, buffer);
  }

  bool add(const char* key, uint8_t value) {
    char buffer[4];
    snprintf(buffer, sizeof(buffer), "%u", value);
    return addSafe(key, buffer);
  }

  bool add(const char* key, uint32_t value) {
    char buffer[11];
    ultoa(value, buffer, 10);
    return addSafe(key, buffer);
  }

  size_t length() const {
    return bufLen;
  }

  uint8_t* buffer() {
    payload[bufLen] = '\0';
    return payload;
  }

  void clear() {
    memset(payload, 0, sizeof(payload));
    bufLen = 0;
  }

 private:
  uint8_t payload[SENSORA_PAYLOAD_SIZE];
  size_t bufLen;

  bool addSafe(const char* key, const char* value) {
    size_t keyLen = strlen(key);
    size_t valueLen = strlen(value);
    if (keyLen == 0) {
      return false;
    }

    // totalLen = key + '=' + value + ';'
    size_t totalLen = keyLen + 1 + valueLen + 1;
    if (bufLen + totalLen >= SENSORA_PAYLOAD_SIZE) {
      SENSORA_LOGW("failed to add key '%s', not enough space in payload", key);
      return false;
    }
    if (bufLen > 0) {
      add(";");
    }
    add(key);
    add("=");
    add(escape(value).c_str());
    return true;
  }

  void add(const char* s) {
    if (s == NULL) {
      payload[bufLen++] = '\0';
      return;
    }
    add(s, strlen(s));
  }

  void add(const char* s, size_t len) {
    if (bufLen + len >= SENSORA_PAYLOAD_SIZE) {
      return;
    }
    memcpy(payload + bufLen, s, len);
    bufLen += len;
  }

  String escape(const char* s) {
    String result;
    while (*s) {
      if (*s == ';') {
        result += "\\;";
      } else {
        result += *s;
      }
      s++;
    }
    return result;
  }
};

#endif