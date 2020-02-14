#ifndef _CLOUDADAPTER_H_
#define _CLOUDADAPTER_H_

#include <iostream>
#include <thread>
#include "mqtt/async_client.h"
#include "json.hpp"

class CloudAdapter {
 public:
  CloudAdapter(nlohmann::json config_json);
  virtual ~CloudAdapter(void);
  void SendNow(nlohmann::json payload);
  void Send(nlohmann::json payload);
  void Cleanup();

 private:
  void ScheduledSendThreadFunction();
  struct Config {
    std::string cloud_url;
    std::string data_channel_id;
    std::string control_channel_id;
    std::string thing_id;
    std::string thing_key;
    int max_buffered_msgs;
    std::string persist_dir;
    int QoS;
    int period_seconds;
  } config_;
  mqtt::async_client *mqtt_client_;
  mqtt::topic *mqtt_topic_;
  nlohmann::json queuedPayload_;
  std::thread scheduledSendThread_;
  bool is_running_ = true;
};

#endif //_CLOUDADAPTER_H_
