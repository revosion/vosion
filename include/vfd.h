#ifndef VOSION_VFD_H_
#define VOSION_VFD_H_

#include <json.hpp>
#include "socketcan_sender.h"
#include "socketcan_raw_receiver.h"

class VarFreqDrive {
 public:
  VarFreqDrive(const nlohmann::json &config);
  VarFreqDrive(void) {
  }
  ;
  virtual ~VarFreqDrive(void);
  void Cleanup(void);

 private:
  nlohmann::json config_;
  std::map<std::string, SocketCANSender*> can_senders_;
  void SendCommand(const std::string vfd_name, const std::string command,
                    const int value);
  void SendCommand(const std::string vfd_name, const std::string command);
  void ScheduledSendThreadFunction(void);
  const int msgid_ = 819;
  std::thread scheduledSendThread_;
  bool is_running_ = true;
};
#endif // VOSION_VFD_H_
