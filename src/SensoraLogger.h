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

#ifndef SensoraLogger_h
#define SensoraLogger_h

#include <Arduino.h>

enum SensoraLogLevel {
  NONE = 0,
  VERBOSE,
  DEBUG,
  INFO,
  WARN,
  ERROR,
};

#define SENSORA_LOG_TAG "SENSORA"

class SensoraLogger : public Print {
 public:
  SensoraLogger(): printer(&Serial){};

  size_t write(uint8_t c) {
    return printer->write(c);
  }

  size_t write(const uint8_t* buffer, size_t size) {
    return printer->write(buffer, size);
  }

  void log_print(int level, const __FlashStringHelper* format, ...) {
    if (level >= SENSORA_LOG_LEVEL) {
      printer->print("\033[");
      switch (level) {
        case SensoraLogLevel::INFO:
          printer->print("32m");
          break;
        case SensoraLogLevel::WARN:
          printer->print("33m");
          break;
        case SensoraLogLevel::ERROR:
          printer->print("31m");
          break;
        default:
          printer->print("0m");
          break;
      }
      printer->print("[");
      printer->print(SENSORA_LOG_TAG);
      printer->print("] ");
      char buffer[256];
      va_list args;
      va_start(args, format);
      vsnprintf_P(buffer, sizeof(buffer), (const char*)format, args);
      va_end(args);
      printer->write(buffer, strlen(buffer));
      printer->print("\033[0m");
      printer->print("\r\n");
    }
  }

  void log_print(int level, const char* format, ...) {
    if (level >= SENSORA_LOG_LEVEL) {
      printer->print("\033[");
      switch (level) {
        case SensoraLogLevel::INFO:
          printer->print("32m");
          break;
        case SensoraLogLevel::WARN:
          printer->print("33m");
          break;
        case SensoraLogLevel::ERROR:
          printer->print("31m");
          break;
        default:
          printer->print("0m");
          break;
      }
      printer->print("[");
      printer->print(SENSORA_LOG_TAG);
      printer->print("] ");
      char buffer[256];
      va_list args;
      va_start(args, format);
      vsnprintf(buffer, sizeof(buffer), format, args);
      va_end(args);
      printer->write(buffer, strlen(buffer));
      printer->print("\033[0m");
      printer->print("\r\n");
    }
  }

  void setPrint(Print* p) {
    printer = p;
  }

 private:
  Print* printer;
};

SensoraLogger logger;

#define SENSORA_LOGV(format, ...) logger.log_print(SensoraLogLevel::VERBOSE, format, ##__VA_ARGS__)
#define SENSORA_LOGD(format, ...) logger.log_print(SensoraLogLevel::DEBUG, format, ##__VA_ARGS__)
#define SENSORA_LOGI(format, ...) logger.log_print(SensoraLogLevel::INFO, format, ##__VA_ARGS__)
#define SENSORA_LOGW(format, ...) logger.log_print(SensoraLogLevel::WARN, format, ##__VA_ARGS__)
#define SENSORA_LOGE(format, ...) logger.log_print(SensoraLogLevel::ERROR, format, ##__VA_ARGS__)

#endif