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

#ifndef SensoraDevice_h
#define SensoraDevice_h

enum class DeviceState {
  Boot,
  Provision,

  ConnectNetwork,
  WaitNetworkConn,
  NetworkConnFailure,

  ConnectMqtt,
  WaitMqttConn,
  MqttConnFailure,

  SubscribeMqtt,
  ReadCloudState,
  SyncDeviceInfo,
  SyncPropertyConfig,
  SyncDeviceStats,
  SyncProperties,
};

void handleMqttMessage(int length);
template <class Board>
class SensoraDevice {
 public:
  SensoraDevice(Client& client) : _state(DeviceState::Boot), _status(DeviceStatus::Boot), _mqtt(client), _isProvision(false), _waitTimer(0), _statsSyncedAt(0), _bootMs(millis()) {}

  void setup() {
    SENSORA_LOGD("initializing device");
    initConfig();
    loadConfig();
    bool isProvision = isProvisionMode();

    if (isProvision) {
      board.setupProvision(_mqtt);
      _state = DeviceState::Provision;
    } else {
      _state = handleConnectNetwork();
    }
  }

  void loop() {
    DeviceState newState = _state;
    switch (_state) {
      case DeviceState::Boot:
        break;
      case DeviceState::Provision:
        board.loopProvision();
        break;
      case DeviceState::ConnectNetwork:
        newState = handleConnectNetwork();
        break;
      case DeviceState::WaitNetworkConn:
        newState = handleWaitNetworkConn();
        break;
      case DeviceState::NetworkConnFailure:
        SENSORA_LOGE("Failed to connect to network. Retrying...");
        newState = DeviceState::ConnectNetwork;
        break;
      case DeviceState::ConnectMqtt:
        newState = handleConnectMqtt();
        break;
      case DeviceState::WaitMqttConn:
        newState = handleWaitMqttConn();
        break;
      case DeviceState::MqttConnFailure:
        newState = handleMqttConnFailure();
        break;
      case DeviceState::SubscribeMqtt:
        newState = handleSubscribeMqtt();
        break;
      case DeviceState::ReadCloudState:
        newState = handleReadCloudState();
        break;
      case DeviceState::SyncDeviceInfo:
        newState = handleSyncDeviceInfo();
        break;
      case DeviceState::SyncPropertyConfig:
        newState = handleSyncPropertyConfig();
        break;
      case DeviceState::SyncDeviceStats:
        newState = handleSyncDeviceStats();
        break;
      case DeviceState::SyncProperties:
        newState = handleSyncProperties();
        break;
      default:
        break;
    }
    _state = newState;
    _mqtt.client().poll();
  }

  DeviceStatus status() { return _status; }
  void setStatus(DeviceStatus s) { _status = s; }
  SensoraMqtt& mqtt() { return _mqtt; }

  void onMqttMessage(int length) {
    uint8_t bytes[length];
    String topic = _mqtt.client().messageTopic();
    char deviceId[SENSORA_MAX_DEVICE_ID_LEN];
    char category[SENSORA_MAX_PROPERTY_ID_LEN];
    char identifier[SENSORA_MAX_PROPERTY_ID_LEN];
    for (int i = 0; i < length; i++) {
      bytes[i] = _mqtt.client().read();
    }
    parseTopic(topic.c_str(), deviceId, category, identifier);
    if (strcmp(deviceId, deviceConfig.deviceId)) {
      return;
    }

    if (strcmp(category, "prop") == 0) {
      Property* prop = propertyList.findById(identifier);
      if (prop == nullptr) {
        SENSORA_LOGW("property not found");
        return;
      }
      if (prop->accessMode() == AccessMode::Read) {
        return;
      }
      prop->onMessage(bytes, length);
    }
  }

 protected:
  DeviceConfig _deviceConfig;

 private:
  Board board;
  DeviceState _state;
  DeviceStatus _status;
  SensoraMqtt _mqtt;
  bool _isProvision;
  unsigned long _waitTimer;
  unsigned long _statsSyncedAt;
  unsigned long _bootMs;

  uint32_t uptimeSeconds() const {
    return (millis() - _bootMs) / 1000ULL;
  }

  DeviceState handleConnectNetwork() {
    board.connectNetwork();
    return DeviceState::WaitNetworkConn;
  }

  DeviceState handleWaitNetworkConn() {
    if (_waitTimer == 0) {
      _waitTimer = millis();
    }
    if (board.networkConnected()) {
      _waitTimer = 0;
      return DeviceState::ConnectMqtt;
    }
    if (millis() - _waitTimer >= 10000LU) {
      _waitTimer = 0;
      return DeviceState::NetworkConnFailure;
    }
    return DeviceState::WaitNetworkConn;
  }

  DeviceState handleConnectMqtt() {
    if (_mqtt.connected()) {
      setStatus(DeviceStatus::Online);
      return DeviceState::SubscribeMqtt;
    }
    _mqtt.onMessage(handleMqttMessage);
    _mqtt.setup();
    if (_mqtt.connect()) {
      setStatus(DeviceStatus::Online);
      return DeviceState::SubscribeMqtt;
    }
    return DeviceState::WaitMqttConn;
  }

  DeviceState handleWaitMqttConn() {
    if (_waitTimer == 0) {
      _waitTimer = millis();
    }

    if (_mqtt.connected()) {
      return DeviceState::SubscribeMqtt;
    }
    if (millis() - _waitTimer >= 10000LU) {
      return DeviceState::MqttConnFailure;
    }
    return DeviceState::WaitMqttConn;
  }

  DeviceState handleMqttConnFailure() {
    SENSORA_LOGE("Failed to connect to Sensora Cloud, retrying...");
    return DeviceState::ConnectMqtt;
  }

  DeviceState handleSubscribeMqtt() {
    if (!_mqtt.connected()) {
      return DeviceState::ConnectMqtt;
    }
    char topic[44];
    snprintf(topic, sizeof(topic), "sc/%s/+/+/set", deviceConfig.deviceId);
    SENSORA_LOGI("subscribing to topic '%s'", topic);
    if (!_mqtt.subscribe(topic)) {
      SENSORA_LOGE("Failed to subscribe to topic '%s'", topic);
      return DeviceState::ConnectNetwork;
    }
    SENSORA_LOGI("successfully subscribed to topic '%s'", topic);
    return DeviceState::ReadCloudState;
  }

  DeviceState handleReadCloudState() {
    char topic[43];
    snprintf(topic, sizeof(topic), "sc/%s/action", deviceConfig.deviceId);
    SensoraPayload p;
    p.add("readPropsValue");
    if (!_mqtt.publish(topic, p.buffer(), p.length())) {
      return DeviceState::ConnectNetwork;
    }
    return DeviceState::SyncDeviceInfo;
  }

  DeviceState handleSyncDeviceInfo() {
    if (!_mqtt.connected()) {
      return DeviceState::ConnectMqtt;
    }
    char topic[38];
    snprintf(topic, sizeof(topic), "sc/%s", deviceConfig.deviceId);

    char ipStr[16];
    IPAddress ip = board.getLocalIP();
    snprintf(ipStr, 16, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    char ids[(propertyList.count() * SENSORA_MAX_PROPERTY_ID_LEN) + propertyList.count()];
    propertyList.getIdentifiers(ids, sizeof(ids));
    SensoraPayload p;
    board.readInfo(p);
    p.add("status=");
    p.add(static_cast<uint8_t>(status()));
    p.add(";");
    p.add("ip=");
    p.add(ipStr);
    p.add(";");
    p.add("fwver=");
    p.add("1.0.0;");
    p.add("props=");
    p.add(ids);

    if (!_mqtt.publish(topic, p.buffer(), p.length())) {
      return DeviceState::ConnectNetwork;
    }
    return DeviceState::SyncPropertyConfig;
  }

  DeviceState handleSyncPropertyConfig() {
    if (!_mqtt.connected()) {
      return DeviceState::ConnectMqtt;
    }
    char topic[81];
    SensoraPayload p;
    for (Property* prop : propertyList) {
      if (prop == nullptr) {
        continue;
      }
      snprintf(topic, sizeof(topic), "sc/%s/prop/%s/cfg", deviceConfig.deviceId, prop->ID());
      p.add("dt=");
      p.add(static_cast<uint8_t>(prop->dataType()));
      p.add(";");
      p.add("am=");
      p.add(static_cast<uint8_t>(prop->accessMode()));
      p.add(";");
      p.add("name=");
      p.add(prop->name());
      p.add(";");
      p.add("nid=");
      p.add(prop->nodeId());

      if (!_mqtt.publish(topic, p.buffer(), p.length())) {
        p.clear();
        memset(topic, 0, sizeof(topic));
        return DeviceState::ConnectNetwork;
      }
      p.clear();
      memset(topic, 0, sizeof(topic));
    }
    return DeviceState::SyncDeviceStats;
  }

  DeviceState handleSyncDeviceStats() {
    if (!_mqtt.connected()) {
      return DeviceState::ConnectMqtt;
    }
    SENSORA_LOGI("sync device stats");
    char topic[38];
    snprintf(topic, sizeof(topic), "sc/%s", deviceConfig.deviceId);
    SensoraPayload p;
    board.readStats(p);
    p.add("status=");
    p.add(static_cast<uint8_t>(status()));
    p.add(";");
    p.add("uptime=");
    p.add(uptimeSeconds());

    if (!_mqtt.publish(topic, p.buffer(), p.length())) {
      p.clear();
      memset(topic, 0, sizeof(topic));
      SENSORA_LOGW("failed to sync device stats");
      return DeviceState::ConnectNetwork;
    }
    p.clear();
    memset(topic, 0, sizeof(topic));
    _statsSyncedAt = millis();
    return DeviceState::SyncProperties;
  }

  DeviceState handleSyncProperties() {
    if (!_mqtt.connected()) {
      return DeviceState::ConnectMqtt;
    }

    char topic[74];
    SensoraPayload p;
    for (Property* prop : propertyList) {
      if (prop == nullptr) {
        continue;
      }
      if (prop->shouldSync()) {
        p.add(prop->getBuff());
        snprintf(topic, sizeof(topic), "sc/%s/prop/%s", deviceConfig.deviceId, prop->ID());
        if (_mqtt.publish(topic, p.buffer(), p.length())) {
          prop->onCloudSynced();
        } else {
          SENSORA_LOGE("failed to sync property state, id '%s'", prop->ID());
          prop->onCloudSyncFailed();
        }
      }
      p.clear();
      memset(topic, 0, sizeof(topic));
    }
    if (millis() - _statsSyncedAt >= DEVICE_STATS_SYNC_INTERVAL_MS) {
      return DeviceState::SyncDeviceStats;
    }
    return DeviceState::SyncProperties;
  }
};
#endif