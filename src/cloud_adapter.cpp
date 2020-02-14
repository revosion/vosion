#include <iostream>
#include <exception>
#include <chrono>
#include <iomanip>
#include <chrono>
#include <thread>

#include "cloud_adapter.h"
#include "json.hpp"

using namespace nlohmann;
using namespace std;

CloudAdapter::CloudAdapter(json config_json) {

  config_.cloud_url = config_json.at("cloud_url");
  config_.data_channel_id = config_json.at("data_channel_id");
  config_.control_channel_id = config_json.at("control_channel_id");
  config_.thing_id = config_json.at("thing_id");
  config_.thing_key = config_json.at("thing_key");
  config_.max_buffered_msgs = config_json.at("max_buffered_msgs");
  config_.persist_dir = config_json.at("persist_dir");
  config_.QoS = config_json.at("QoS");
  config_.period_seconds = config_json.at("period_seconds");

  mqtt_client_ = new mqtt::async_client(config_.cloud_url, "",
                                        config_.max_buffered_msgs,
                                        config_.persist_dir);
  mqtt::connect_options conn_opts;
  conn_opts.set_keep_alive_interval(
      config_.max_buffered_msgs * config_.period_seconds);//TODO better plan needed
  conn_opts.set_clean_session(true);  //TODO change to false
  conn_opts.set_automatic_reconnect(true);
  //conn_opts.set_user_name(config_.thing_id);
  //conn_opts.set_password(config_.thing_key);
  //mqtt::ssl_options sslopts;
  //sslopts.set_trust_store("ca.crt");
  //connOpts.set_ssl(sslopts);

  mqtt::message will_msg("test/events", "CloudAdapter mqtt disconnected", 1,
                         true);
  mqtt::will_options will(will_msg);
  conn_opts.set_will(will);
  //connopts.set_automatic_reconnect(1, 10);

  const string topic = "channels/" + config_.data_channel_id + "/messages";

  mqtt_topic_ = new mqtt::topic(*mqtt_client_, topic, config_.QoS, true);

  try {
    //mqtt_client_->connect(conn_opts)->wait();
  } catch (const mqtt::exception &e) {
    cerr << "Unable to connect to mqtt broker:" << e.what() << endl;
  }
  scheduledSendThread_ = thread(&CloudAdapter::ScheduledSendThreadFunction, this);
}

CloudAdapter::~CloudAdapter(void) {
}

void CloudAdapter::Cleanup() {
  is_running_ = false;
  scheduledSendThread_.join();
  if (mqtt_client_->is_connected()) {
    mqtt_client_->disconnect()->wait();
  }
  delete mqtt_topic_;
  delete mqtt_client_;
}

void CloudAdapter::Send(json payload) {
  queuedPayload_.push_back(payload);
}

void CloudAdapter::ScheduledSendThreadFunction() {
  auto tm = chrono::steady_clock::now();

  while (is_running_) {
    this_thread::sleep_until(tm);
    if (!queuedPayload_.is_null()) {
      cout << "Start sending queued payload" << endl;
      SendNow(queuedPayload_);
      queuedPayload_ = nullptr;
    }
    tm += chrono::seconds(config_.period_seconds);
  }
}

void CloudAdapter::SendNow(json payload) {
  try {
    //mqtt_topic_->publish(payload.dump());
    cout << "Sending " << payload.dump() << endl;
  } catch (const mqtt::exception &e) {
    cerr << "Fail to send data:" << e.what() << endl;
  }
}

