#ifndef VOSION_SOCKETCAN_SENDER_H_
#define VOSION_SOCKETCAN_SENDER_H_

#include <net/if.h>
#include <linux/can/bcm.h>
#include <json.hpp>

class SocketCANSender {
 public:
  SocketCANSender(const char *iface);
  SocketCANSender() {
  }
  ;
  virtual ~SocketCANSender();
  int Init();
  int Close();
  int Send(const int msgid, const nlohmann::json msg);
  int SendCyclic(const int msgid, const int ival, const std::vector<nlohmann::json> msgs);

 private:
  int socket_;
  const char *iface_;
  struct sockaddr_can addr_;
  struct ifreq ifr_;
};
#endif // VOSION_SOCKETCAN_SENDER_H_
