#ifndef VOSION_SOCKETCAN_RAW_RECEIVER_H_
#define VOSION_SOCKETCAN_RAW_RECEIVER_H_

#include <net/if.h>
#include <linux/can/bcm.h>
#include <json.hpp>

class SocketCANRawReceiver {
 public:
  SocketCANRawReceiver(const char *iface);
  SocketCANRawReceiver(void) {
  }
  ;
  virtual ~SocketCANRawReceiver(void);
  int Init(void);
  int Close(void);
  int Read(void);
  void ProcessFrame(const struct canfd_frame &frame);

 private:
  int socket_;
  const char *iface_;
  struct sockaddr_can addr_;
  struct ifreq ifr_;
};
#endif // VOSION_SOCKETCAN_RAW_RECEIVER_H_
