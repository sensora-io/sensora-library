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

#include <SensoraLogger.h>

SensoraLogger logger;
SensoraLogger::SensoraLogger() : _printer(&Serial) {}

size_t SensoraLogger::write(uint8_t c) {
  return _printer->write(c);
}

size_t SensoraLogger::write(const uint8_t* buffer, size_t size) {
  return _printer->write(buffer, size);
}

void SensoraLogger::log_print(int level, const __FlashStringHelper* format, ...) {
  if (level >= SENSORA_LOG_LEVEL) {
    _printer->print("\033[");
    switch (level) {
      case SensoraLogLevel::INFO:
        _printer->print("32m");
        break;
      case SensoraLogLevel::WARN:
        _printer->print("33m");
        break;
      case SensoraLogLevel::ERROR:
        _printer->print("31m");
        break;
      default:
        _printer->print("0m");
        break;
    }
    _printer->print("[");
    _printer->print(SENSORA_LOG_TAG);
    _printer->print("] ");
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf_P(buffer, sizeof(buffer), (const char*)format, args);
    va_end(args);
    _printer->write(buffer, strlen(buffer));
    _printer->print("\033[0m");
    _printer->print("\r\n");
  }
}

void SensoraLogger::log_print(int level, const char* format, ...) {
  if (level >= SENSORA_LOG_LEVEL) {
    _printer->print("\033[");
    switch (level) {
      case SensoraLogLevel::INFO:
        _printer->print("32m");
        break;
      case SensoraLogLevel::WARN:
        _printer->print("33m");
        break;
      case SensoraLogLevel::ERROR:
        _printer->print("31m");
        break;
      default:
        _printer->print("0m");
        break;
    }
    _printer->print("[");
    _printer->print(SENSORA_LOG_TAG);
    _printer->print("] ");
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    _printer->write(buffer, strlen(buffer));
    _printer->print("\033[0m");
    _printer->print("\r\n");
  }
}

void SensoraLogger::setPrint(Print* p) {
  _printer = p;
}
