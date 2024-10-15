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

#ifndef StoragePreferences_h
#define StoragePreferences_h

#include <Preferences.h>

Preferences preferences;

void storageBegin() {
  preferences.begin("sensora", false);
}

void storageEnd() {
  preferences.end();
}

template<typename T>
bool readConfig(const char* key, T& config) {
  size_t readBytes = preferences.getBytes(key, &config, sizeof(T));
  return readBytes == sizeof(T);
}

template<typename T>
void writeConfig(const char* key, const T& config) {
  preferences.putBytes(key, &config, sizeof(T));
}

#endif
