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

#ifndef SensoraDefinitions_h
#define SensoraDefinitions_h

enum class DeviceStatus {
  Boot = 1,
  Online,
  Offline,
  Sleeping,
  Lost,
  Alert
};

enum class AccessMode {
  Read = 1,
  Write,
  ReadWrite
};

enum class DataType {
  String = 1,
  Integer,
  Float,
  Boolean,
  Color
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
  void updateBuffer(const uint8_t* msg, int length) {
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

class Property;
typedef void (*PropertySubscribeCb)(PropertyValue&);

class Property : public PropertyValue {
 public:
  Property() : PropertyValue() {}
  Property(const char* id, const char* name, const char* nodeId);

  const char* ID() { return _id; }
  const char* name() { return _name; }
  const char* nodeId() { return _nodeId; }
  DataType dataType() const { return _dataType; }
  AccessMode accessMode() const { return _accessMode; }

  Property& setAccessMode(AccessMode m) {
    _accessMode = m;
    return *this;
  }

  Property& setDataType(DataType d) {
    _dataType = d;
    return *this;
  }

  Property& subscribe(PropertySubscribeCb cb) {
    _subscribe_cb = cb;
    return *this;
  }

  Property& setSyncStrategy(SyncStrategy strategy, unsigned long interval = 15000) {
    _syncStrategy = strategy;
    _syncIntervalMs = interval;
    return *this;
  }

  Property& setNodeId(const char* nodeId) {
    _nodeId = nodeId;
    return *this;
  }

  bool shouldSync() {
    // wait for cloud value before syncing
    // property with write capability
    if (_accessMode == AccessMode::Write) {
      return false;
    }
    if (_cloudSyncedAt == 0 && _accessMode != AccessMode::Read) {
      return false;
    }
    if (_accessMode == AccessMode::Read && getLen() == 0) {
      return false;
    }
    unsigned long now = millis();
    if (_syncStrategy == SyncStrategy::Periodic) {
      if (_accessMode == AccessMode::ReadWrite && strcmp(getBuff(), _cloudValue) != 0) {
        return true;
      }

      if (now - _cloudSyncedAt >= _syncIntervalMs) {
        return true;
      }
      return false;
    }

    if (_syncStrategy == SyncStrategy::OnChange) {
      if (strcmp(getBuff(), _cloudValue) == 0) {
        return false;
      }
      return true;
    }
    // unknown sync strategy
    return false;
  }

  void onCloudSynced() {
    memcpy(_cloudValue, getBuff(), getLen());
    _cloudValue[getLen()] = '\0';
    _cloudSyncedAt = millis();
    _cloudSyncFails = 0;
  }

  void onCloudSyncFailed() {
    // max 30 * 500ms = 15 seconds
    if (_cloudSyncFails == 30) {
      return;
    }
    _cloudSyncFails++;
  }

  void onMessage(const uint8_t* msg, const int length) {
    updateBuffer(msg, length);
    onCloudSynced();
    if (_subscribe_cb) {
      _subscribe_cb(value());
    }
  }

 private:
  const char* _id;
  const char* _name;
  DataType _dataType;
  AccessMode _accessMode;
  const char* _nodeId;

  char _cloudValue[PROPERTY_BUFFER_SIZE];
  unsigned long _cloudSyncedAt;
  SyncStrategy _syncStrategy;
  unsigned long _syncIntervalMs;
  int _cloudSyncFails;
  PropertySubscribeCb _subscribe_cb;
};

class PropertyList {
 public:
  PropertyList() : _propertyCount(0) {}

  void add(Property* prop) {
    if (_propertyCount < MAX_PROPERTIES) {
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

  void getIdentifiers(char* buff, size_t buffSize) {
    if (buff == nullptr || buffSize == 0) {
      return;
    }
    buff[0] = '\0';
    for (int i = 0; i < _propertyCount; i++) {
      if (_properties[i] == nullptr) {
        continue;
      }
      const char* id = _properties[i]->ID();
      if (strlen(buff) + strlen(id) + (i < _propertyCount - 1 ? 1 : 0) >= buffSize) {
        return;
      }
      strcat(buff, id);
      if (i < _propertyCount - 1) {
        strcat(buff, ",");
      }
    }
  }

  Property** begin() { return &_properties[0]; }
  Property** end() { return &_properties[_propertyCount]; }

 private:
  int _propertyCount;
  const char* error;
  Property* _properties[MAX_PROPERTIES];
};
PropertyList propertyList;

Property::Property(const char* id, const char* name, const char* nodeId = "")
    : _id(id),
      _name(name),
      _dataType(DataType::Integer),
      _accessMode(AccessMode::Read),
      _nodeId(nodeId),
      _cloudSyncedAt(0),
      _syncStrategy(SyncStrategy::Periodic),
      _syncIntervalMs(10000),
      _cloudSyncFails(0) {
  if (propertyList.findById("id") != nullptr) {
    SENSORA_LOGW("Property with id '%s' already exists");
    return;
  }
  propertyList.add(this);
}

#endif