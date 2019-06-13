#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <fcntl.h>

#include <linux/can.h>
#include <linux/can/bcm.h>
#include "receiver.h"
#include "util.h"

Receiver::Receiver(char* iface, int msgid) {
  iface_ = iface;
  msgid_ = msgid;
}

int Receiver::Init() {
  /* Open the CAN interface */
  socket_ = socket(PF_CAN, SOCK_DGRAM, CAN_BCM);
  if (socket_ < 0)
  {
    perror(": socket");
    return errno;
  }

  strncpy(ifr_.ifr_name, iface_, IFNAMSIZ);
  if (ioctl(socket_, SIOCGIFINDEX, &ifr_) < 0)
  {
    perror(": ioctl");
    return errno;
  }

  addr_.can_family = AF_CAN;
  addr_.can_ifindex = ifr_.ifr_ifindex;
  if (connect(socket_, (struct sockaddr *)&addr_, sizeof(addr_)) < 0)
  {
    perror(": connect");
    return errno;
  }

  /* Set socket to non-blocking */
  int flags = fcntl(socket_, F_GETFL, 0);
  if (flags < 0)
  {
      perror(": fcntl: F_GETFL");
      return errno;
  }

  if (fcntl(socket_, F_SETFL, flags | O_NONBLOCK) < 0)
  {
      perror(": fcntl: F_SETFL");
      return errno;
  }

  msg_.msg_head.opcode  = RX_SETUP;
  msg_.msg_head.can_id  = msgid_;
  msg_.msg_head.flags   = 0;
  msg_.msg_head.nframes = 0;
  if (write(socket_, &msg_, sizeof(msg_)) < 0)
  {
    perror(": write: RX_SETUP");
    return errno;
  }
}

int Receiver::Read() {
  ssize_t nbytes;

  /* Read from the CAN interface */
  nbytes = read(socket_, &msg_, sizeof(msg_));
  if (nbytes < 0)
  {
    if (errno != EAGAIN)
    {
      perror(": read");
    }
  }
  else
  {
    struct can_frame * const frame = msg_.frame;
    unsigned char * const data = frame->data;
    const unsigned int dlc = frame->can_dlc;

    /* Print the received CAN frame */
    printf("RX:  ");
    print_can_frame(frame);
    printf("\n");
  }
}
