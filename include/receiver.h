#ifndef RECEIVER_H_
#define RECEIVER_H_

#include <net/if.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/bcm.h>

class Receiver
{
public:
  Receiver(char *iface, int msgid);
  int Init();
  int Read();

private:
  int socket_;
  char *iface_;
  int msgid_;
  struct sockaddr_can addr_;
  struct ifreq ifr_;
  struct CanMsg
  {
    struct bcm_msg_head msg_head;
    struct can_frame frame[1];
  }msg_;
};
#endif // RECEIVER_H_
