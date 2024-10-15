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

#ifndef SensoraProperty_h
#define SensoraProperty_h

enum class DataType {
  String = 1,
  Integer,
  Float,
  Boolean,
  Enum,
  Color,
  Location
};

enum class AccessMode {
  Read = 1,
  Write,
  ReadWrite
};
enum class SyncStrategy {
  Periodic,
  OnChange
};

class PropertyValue {
 public:
  PropertyValue() : len(0) {
    buff[0] = '\0';
  }

  void setValue(int val) {
    len = snprintf(buff, PROPERTY_BUFFER_SIZE, "%i", val);
  }

  void setValue(float val) {
    len = snprintf(buff, PROPERTY_BUFFER_SIZE, "%.3f", val);
  }

  void setValue(double val) {
    len = snprintf(buff, PROPERTY_BUFFER_SIZE, "%.8f", val);
  }

  void setValue(const char* s) {
    strncpy(buff, s, PROPERTY_BUFFER_SIZE - 1);
    buff[PROPERTY_BUFFER_SIZE - 1] = '\0';
    len = strlen(buff);
  }

  void setValue(bool b) {
    if (b) {
      setValue("true");
    } else {
      setValue("false");
    }
  }

  int Int() {
    return atoi(buff);
  }

  bool Bool() {
    return strncmp(buff, "true", 4) == 0;
  }

  const char* getBuff() { return buff; }
  const size_t getLen() { return len; }

 protected:
  void updateBuffer(const char* msg, int length) {
    int cLen = length > PROPERTY_BUFFER_SIZE - 1 ? PROPERTY_BUFFER_SIZE - 1 : length;
    memcpy(buff, msg, cLen);
    buff[cLen] = '\0';
    len = cLen;
  }

  PropertyValue& value() {
    return *this;
  }

 private:
  char buff[PROPERTY_BUFFER_SIZE];
  size_t len;
};

class Property : public PropertyValue {
 public:
  typedef void (*PropertySubscribeCb)(PropertyValue&);
  Property() {}
  Property(const char* id, const char* nodeId);
  const char* ID() { return id; }
  const char* nodeId() { return node; }

  DataType getDataType() const { return dataType; }
  AccessMode getAccessMode() const { return accessMode; }
  SyncStrategy getSyncStrategy() { return syncStrategy; }

  Property& setAccessMode(AccessMode m) {
    accessMode = m;
    return *this;
  }

  Property& setDataType(DataType d) {
    dataType = d;
    switch (d) {
      case DataType::String:
        this->setValue("");
        break;
      case DataType::Integer:
        this->setValue(0);
        break;
      case DataType::Float:
        this->setValue(0.0);
        break;
      case DataType::Boolean:
        this->setValue(false);
        break;
      default:
        break;
    }
    return *this;
  }

  Property& subscribe(PropertySubscribeCb callback) {
    cb = callback;
    return *this;
  }

  Property& setSyncStrategy(SyncStrategy strategy, unsigned long interval = 15000) {
    syncStrategy = strategy;
    syncIntervalMs = interval;
    return *this;
  }

  bool shouldSync() {
    if (cloudSyncedAt == 0) {
      return true;
    }

    if (syncStrategy == SyncStrategy::OnChange) {
      return (strcmp(getBuff(), cloudValue) != 0);
    }
    unsigned long now = millis();
    if (syncStrategy == SyncStrategy::Periodic) {
      return now - cloudSyncedAt >= syncIntervalMs && strcmp(getBuff(), cloudValue) != 0;
    }
    return false;
  }

  void onCloudSynced() {
    size_t len = getLen();
    if (len >= PROPERTY_BUFFER_SIZE) {
      len = PROPERTY_BUFFER_SIZE - 1;
    }
    memcpy(cloudValue, getBuff(), len);
    cloudValue[len] = '\0';
    cloudSyncedAt = millis();
    cloudSyncFails = 0;
  }

  void onCloudSyncFailed() {
    // max 30 * 500ms = 15 seconds
    if (cloudSyncFails == 30) {
      // TODO
      return;
    }
    cloudSyncFails++;
  }

  void onMessage(const char* msg, size_t length) {
    updateBuffer(msg, length);
    onCloudSynced();
    if (cb != nullptr) {
      cb(value());
    }
  }

 private:
  const char* id;
  const char* node;
  DataType dataType;
  AccessMode accessMode;
  SyncStrategy syncStrategy;
  PropertySubscribeCb cb;

  int cloudSyncFails;
  unsigned long cloudSyncedAt;
  unsigned long syncIntervalMs;
  char cloudValue[PROPERTY_BUFFER_SIZE];
};

class PropertyList {
 public:
  PropertyList() : _propertyCount(0) {}

  void add(Property* prop) {
    if (_propertyCount < DEVICE_MAX_PROPERTIES) {
      _properties[_propertyCount++] = prop;
    } else {
      SENSORA_LOGE("Maximum properties reached. Please change MAX_PROPERTIES");
    }
  }

  int count() { return _propertyCount; }

  Property* findById(const char* id) {
    for (int i = 0; i < _propertyCount; i++) {
      Property* c = _properties[i];
      if (c == nullptr) {
        continue;
      }
      if (strcmp(c->ID(), id) == 0) {
        return c;
      }
    }
    return nullptr;
  }

  Property** begin() { return &_properties[0]; }
  Property** end() { return &_properties[_propertyCount]; }

 private:
  int _propertyCount;
  const char* error;
  Property* _properties[DEVICE_MAX_PROPERTIES];
};
PropertyList propertyList;

Property::Property(const char* id, const char* nodeId = "")
    : id(id), node(nodeId), accessMode(AccessMode::Read), dataType(DataType::String), syncStrategy(SyncStrategy::OnChange) {
  if (propertyList.findById("id") != nullptr) {
    SENSORA_LOGW("Property with id '%s' already exists");
    return;
  }
  this->setValue("");
  propertyList.add(this);
}

#endif