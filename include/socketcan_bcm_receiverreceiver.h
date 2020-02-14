#ifndef RECEIVER_H_
#define RECEIVER_H_

#include <net/if.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/bcm.h>

class SocketCANBCMReceiver {
 public:
  SocketCANBCMReceiver(const char *iface);
  int Init();
  int Read();

 private:
  int socket_;
  const char *iface_;
  int msgid_;
  struct sockaddr_can addr_;
  struct ifreq ifr_;
  struct CanMsg {
    struct bcm_msg_head msg_head;
    struct can_frame frame[1];
  } msg_;
  void ProcessFrame(const struct can_frame *const frame);
};
#endif // RECEIVER_H_
