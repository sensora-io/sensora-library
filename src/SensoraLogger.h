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

#ifndef SENSORA_LOG_LEVEL
#define SENSORA_LOG_LEVEL SensoraLogLevel::INFO
#endif

#define SENSORA_LOG_TAG "SENSORA"

class SensoraLogger : public Print {
 public:
  SensoraLogger();
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t*, size_t);
  void log_print(int level, const __FlashStringHelper* format, ...);
  void log_print(int level, const char* format, ...);

 private:
  void setPrint(Print*);
  Print* _printer;
};

extern SensoraLogger logger;
#define SENSORA_LOGV(format, ...) logger.log_print(SensoraLogLevel::VERBOSE, format, ##__VA_ARGS__)
#define SENSORA_LOGD(format, ...) logger.log_print(SensoraLogLevel::DEBUG, format, ##__VA_ARGS__)
#define SENSORA_LOGI(format, ...) logger.log_print(SensoraLogLevel::INFO, format, ##__VA_ARGS__)
#define SENSORA_LOGW(format, ...) logger.log_print(SensoraLogLevel::WARN, format, ##__VA_ARGS__)
#define SENSORA_LOGE(format, ...) logger.log_print(SensoraLogLevel::ERROR, format, ##__VA_ARGS__)

#endif
