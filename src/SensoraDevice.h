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

#ifndef SensoraDevice_h
#define SensoraDevice_h

#include <Sensora/SensoraConfig.h>
#include <Sensora/SensoraLogger.h>
#include <Sensora/SensoraUtil.h>
#include <Sensora/SensoraPayload.h>
#include <Sensora/SensoraLink.h>
#include <Sensora/SensoraProperty.h>
#include <Sensora/SensoraTransport.h>

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
  SyncDeviceInfo,
  SyncPropertyInfo,
  SyncDeviceStats,
  SyncPropertyState,
};

template <class Board>
class SensoraDevice {
 public:
  SensoraDevice(Transp& transp) : transp(transp), state(DeviceState::Boot), st(DeviceStatus::Boot), bootedAt(millis()) {
  }

  void setup() {
    SENSORA_LOGI("device setup");
    board.setup();
    if (board.isProvision()) {
      SENSORA_LOGI("Running provision mode");
      board.setupProvision(transp);
      setState(DeviceState::Provision);
    } else {
      SENSORA_LOGI("Running normal mode");
      setState(DeviceState::ConnectNetwork);
      printLogo();
    }
  }

  void loop() {
    DeviceState newState = state;
    switch (state) {
      case DeviceState::Boot:
        break;
      case DeviceState::Provision:
        board.loopProvision();
        break;
      case DeviceState::ConnectNetwork:
        board.connectNetwork();
        newState = DeviceState::WaitNetworkConn;
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
        SENSORA_LOGE("Failed to connect to Sensora Cloud, retrying...");
        newState = DeviceState::ConnectMqtt;
        break;
      case DeviceState::SubscribeMqtt:
        newState = handleSubscribeMqtt();
        break;
      case DeviceState::SyncDeviceInfo:
        newState = handleSyncDeviceInfo();
        break;
      case DeviceState::SyncPropertyInfo:
        newState = handleSyncPropertyInfo();
        break;
      case DeviceState::SyncDeviceStats:
        newState = handleSyncDeviceStats();
        break;
      case DeviceState::SyncPropertyState:
        newState = handleSyncPropertyState();
        break;
      default:
        break;
    }
    setState(newState);
    if (transp.connected()) {
      transp.mqtt().poll();
    }
  }

  DeviceStatus status() { return st; }

  void handleMessage(int length) {
    uint8_t bytes[length];
    String topic = transp.mqtt().messageTopic();
    char deviceId[SENSORA_MAX_DEVICE_ID_LEN];
    for (int i = 0; i < length; i++) {
      bytes[i] = transp.mqtt().read();
    }

    extractDeviceId(topic.c_str(), deviceId);
    if (strcmp(deviceId, deviceConfig.deviceId) != 0) {
      SENSORA_LOGW("invalid device id");
      return;
    }
    char propertyId[SENSORA_MAX_PROPERTY_ID_LEN];
    if (!extractPayload(bytes, length, "id", propertyId, sizeof(propertyId))) {
      SENSORA_LOGW("property id not found in payload");
      return;
    }
    char propertyValue[PROPERTY_BUFFER_SIZE];
    if (!extractPayload(bytes, length, "value", propertyValue, sizeof(propertyValue))) {
      SENSORA_LOGW("property value not found in payload");
      return;
    }
    Property* prop = propertyList.findById(propertyId);
    if (prop == nullptr) {
      SENSORA_LOGW("property not found");
      return;
    }
    if (prop->getAccessMode() == AccessMode::Read) {
      SENSORA_LOGW("cannot update property '%s' because access mode is read only", prop->ID());
      return;
    }
    prop->onMessage(propertyValue, strlen(propertyValue));
  }

  void addAttribute(const char* key, const char* value) {
  }

 protected:
  Board board;
  Transp& transp;

 private:
  DeviceState state;
  DeviceStatus st;
  void setState(DeviceState s) { state = s; }
  void setStatus(DeviceStatus s) { st = s; }
  unsigned long waitTimer;
  unsigned long bootedAt;
  unsigned long statSyncedAt;

  uint32_t uptimeSeconds() const {
    return (millis() - bootedAt) / 1000ULL;
  }

  DeviceState handleWaitNetworkConn() {
    if (waitTimer == 0) {
      waitTimer = millis();
    }
    if (board.isNetworkConnected()) {
      waitTimer = 0;
      return DeviceState::ConnectMqtt;
    }
    if (millis() - waitTimer >= 10000LU) {
      waitTimer = 0;
      return DeviceState::NetworkConnFailure;
    }
    return DeviceState::WaitNetworkConn;
  }

  DeviceState handleConnectMqtt() {
    if (transp.connected()) {
      setStatus(DeviceStatus::Online);
      return DeviceState::SubscribeMqtt;
    }
    transp.mqtt().onMessage(onMessage);
    transp.setup();
    if (transp.connect()) {
      setStatus(DeviceStatus::Online);
      return DeviceState::SubscribeMqtt;
    }
    return DeviceState::WaitMqttConn;
  }

  DeviceState handleWaitMqttConn() {
    if (waitTimer == 0) {
      waitTimer = millis();
    }

    if (transp.connected()) {
      return DeviceState::SubscribeMqtt;
    }
    if (millis() - waitTimer >= 10000LU) {
      return DeviceState::MqttConnFailure;
    }
    return DeviceState::WaitMqttConn;
  }

  DeviceState handleSubscribeMqtt() {
    if (!transp.connected()) {
      return DeviceState::ConnectMqtt;
    }
    char topic[45];
    snprintf(topic, sizeof(topic), "sc/%s/msg/recv", deviceConfig.deviceId);
    SENSORA_LOGI("subscribing to topic '%s'", topic);
    if (!transp.subscribe(topic)) {
      SENSORA_LOGE("Failed to subscribe to topic '%s'", topic);
      return DeviceState::ConnectNetwork;
    }
    SENSORA_LOGI("successfully subscribed to topic '%s'", topic);
    return DeviceState::SyncDeviceInfo;
  }

  DeviceState handleSyncDeviceInfo() {
    if (!transp.connected()) {
      return DeviceState::ConnectMqtt;
    }
    char topic[45];
    snprintf(topic, sizeof(topic), "sc/%s/dev/info", deviceConfig.deviceId);
    SensoraPayload payload;
    payload.add("fw_version", "1.0.0");
    board.readInfo(payload);
    if (!transp.publish(topic, payload.buffer(), payload.length())) {
      SENSORA_LOGE("failed to sync device info");
      return DeviceState::ConnectNetwork;
    }
    return DeviceState::SyncPropertyInfo;
  }

  DeviceState handleSyncPropertyInfo() {
    if (!transp.connected()) {
      return DeviceState::ConnectMqtt;
    }
    char topic[46];
    snprintf(topic, sizeof(topic), "sc/%s/prop/info", deviceConfig.deviceId);
    SensoraPayload payload;
    for (Property* prop : propertyList) {
      if (prop == nullptr) {
        continue;
      }
      payload.add("id", prop->ID());
      payload.add("nodeId", prop->nodeId());
      payload.add("dataType", static_cast<uint8_t>(prop->getDataType()));
      payload.add("accessMode", static_cast<uint8_t>(prop->getAccessMode()));
      payload.add("syncStrategy", static_cast<uint8_t>(prop->getSyncStrategy()));
      if (!transp.publish(topic, payload.buffer(), payload.length())) {
        return DeviceState::ConnectNetwork;
      }
      payload.clear();
    }
    return DeviceState::SyncDeviceStats;
  }

  DeviceState handleSyncDeviceStats() {
    if (!transp.connected()) {
      return DeviceState::ConnectMqtt;
    }
    char topic[45];
    snprintf(topic, sizeof(topic), "sc/%s/dev/info", deviceConfig.deviceId);

    SensoraPayload payload;
    payload.add("status", static_cast<uint8_t>(status()));
    payload.add("uptime", uptimeSeconds());
    board.readStats(payload);
    if (!transp.publish(topic, payload.buffer(), payload.length())) {
      return DeviceState::ConnectNetwork;
    }
    statSyncedAt = millis();
    return DeviceState::SyncPropertyState;
  }

  DeviceState handleSyncPropertyState() {
    if (!transp.connected()) {
      return DeviceState::ConnectMqtt;
    }
    char topic[44];
    snprintf(topic, sizeof(topic), "sc/%s/msg/pub", deviceConfig.deviceId);
    SensoraPayload payload;
    for (Property* prop : propertyList) {
      if (prop == nullptr) {
        continue;
      }
      if (prop->shouldSync()) {
        payload.add("id", prop->ID());
        payload.add("value", prop->getBuff());
        if (transp.publish(topic, payload.buffer(), payload.length())) {
          prop->onCloudSynced();
        } else {
          SENSORA_LOGE("failed to sync property state, id '%s'", prop->ID());
          prop->onCloudSyncFailed();
        }
      }
      payload.clear();
    }
    if (millis() - statSyncedAt >= DEVICE_STATS_SYNC_INTERVAL_MS) {
      return DeviceState::SyncDeviceStats;
    }
    return DeviceState::SyncPropertyState;
  }

  static void onMessage(int len);
};

#endif