#include <iostream>
#include <chrono>
#include <thread>

#include "vfd.h"

using namespace std;
using namespace nlohmann;

VarFreqDrive::VarFreqDrive(const json &config)
    :
    config_(config) {
  for (auto &item : config_.at("devices").items()) {
    const char *can_iface = item.value().at("can_iface").get<string>().c_str();

    can_senders_[item.key()] = new SocketCANSender(can_iface);
    ;
  }
  scheduledSendThread_ = thread(&VarFreqDrive::ScheduledSendThreadFunction,
                                this);
}

VarFreqDrive::~VarFreqDrive(void) {
}

void VarFreqDrive::SendCommand(const std::string vfd_name,
                               const std::string command) {
  json msg = config_.at("commands").at(command);
  msg["slave_id"] =
      config_.at("devices").at(vfd_name).at("slave_id").get<int>();
  can_senders_[vfd_name]->Send(msgid_, msg);
  cout << msg.dump() << endl;
}

void VarFreqDrive::SendCommand(const std::string vfd_name,
                               const std::string command, const int value) {
  json msg = config_.at("commands").at(command);
  msg["slave_id"] =
      config_.at("devices").at(vfd_name).at("slave_id").get<int>();
  msg["param_val"] = value;
  can_senders_[vfd_name]->Send(msgid_, msg);
  cout << msg.dump() << endl;
}

void VarFreqDrive::ScheduledSendThreadFunction(void) {
  auto tm = chrono::steady_clock::now();
  string uiface = "can0";
  const char *can_iface = uiface.c_str();
  SocketCANRawReceiver receiver(can_iface);
  while (is_running_) {
    tm += chrono::milliseconds(500);
    this_thread::sleep_until(tm);
    for (auto &item : config_.at("devices").items()) {
      SendCommand(item.key(), "start");
      SendCommand(item.key(), "set_frequency", 10);  //TODO get frequency from pid controller
    }
    receiver.Read();
  }
}

void VarFreqDrive::Cleanup(void) {
  is_running_ = false;
  scheduledSendThread_.join();
  for (auto const& sender : can_senders_) {
    delete sender.second;
  }
}
