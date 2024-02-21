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

#ifndef SensoraPayload_h
#define SensoraPayload_h

#define SENSORA_PAYLOAD_SIZE 128

class SensoraPayload {
 public:
  SensoraPayload() : bufLen(0) {}

  size_t length() {
    return bufLen;
  }

  uint8_t* buffer() {
    payload[bufLen] = '\0';
    return payload;
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

  void add(int8_t n) {
    char buffer[5];
    snprintf(buffer, sizeof(buffer), "%d", n);
    add(buffer);
  }

  void add(uint8_t n) {
    char buffer[4];
    snprintf(buffer, sizeof(buffer), "%hhu", n);
    add(buffer);
  }

  void add(uint32_t n) {
    char buffer[1 + 3 * sizeof(n)];
    ultoa(n, buffer, 10);
    add(buffer);
  }

  void clear() {
    memset(payload, 0, sizeof(payload));
    bufLen = 0;
  }

 private:
  uint8_t payload[SENSORA_PAYLOAD_SIZE];
  size_t bufLen;
};

#endif