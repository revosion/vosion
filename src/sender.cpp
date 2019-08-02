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
#include "sender.h"

Sender::Sender(char* iface) {
  iface_ = iface;
  Init();
}

int Sender::Init() {
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
}

int Sender::Close() {
  if (close(socket_) < 0) {
    perror(": close");
    return errno;
  }
}

int Sender::Send(int msgid, json11::Json msg) {
  struct CanMsg
  {
    struct bcm_msg_head msg_head;
    struct can_frame frame[1];
  } can_msg;
  can_msg.msg_head.opcode  = TX_SEND;
  can_msg.msg_head.can_id  = 0;
  can_msg.msg_head.flags   = 0;
  can_msg.msg_head.nframes = 1;

  int slave_id = msg["slave_id"].int_value();
  int param_addr = msg["param_addr"].int_value();
  int operation = msg["operation"].int_value();
  int param_val = msg["param_val"].int_value();
  can_msg.frame[0].can_dlc = 8;
  can_msg.frame[0].can_id = msgid;
  can_msg.frame[0].data[0] = operation;
  can_msg.frame[0].data[1] = slave_id;
  can_msg.frame[0].data[3] = (param_addr >> 8) & 0xFF;
  can_msg.frame[0].data[2] = param_addr & 0xFF;
  can_msg.frame[0].data[4] = 0;
  can_msg.frame[0].data[5] = 0;
  can_msg.frame[0].data[7] = (param_val >> 8) & 0xFF;
  can_msg.frame[0].data[6] = param_val & 0xFF;

  if (write(socket_, &can_msg, sizeof(can_msg)) < 0)
  {
    perror(": write: TX_SEND");
    return errno;
  }
}

int Sender::SendCyclic(int msgid, int ival, std::vector<json11::Json> msgs) {
  struct CanMsg
  {
    struct bcm_msg_head msg_head;
    struct can_frame frame[4];
  } can_msg;

  int ival_sec = ival / 1000000;
  int ival_usec = ival % 1000000;
  can_msg.msg_head.opcode  = TX_SETUP;
  can_msg.msg_head.can_id  = 0;
  can_msg.msg_head.flags   = SETTIMER | STARTTIMER;
  can_msg.msg_head.count   = 0;
  can_msg.msg_head.ival2.tv_sec = ival_sec;
  can_msg.msg_head.ival2.tv_usec = ival_usec;

  int num_frames = 0;
  for (auto &msg : msgs) {
    int slave_id = msg["slave_id"].int_value();
    int param_addr = msg["param_addr"].int_value();
    int operation = msg["operation"].int_value();
    int param_val = msg["param_val"].int_value();
    can_msg.frame[num_frames].can_dlc = 8;
    can_msg.frame[num_frames].can_id = msgid;
    can_msg.frame[num_frames].data[0] = slave_id;
    can_msg.frame[num_frames].data[1] = operation;
    can_msg.frame[num_frames].data[2] = param_addr & 0xFF;
    can_msg.frame[num_frames].data[3] = (param_addr >> 8) & 0xFF;
    can_msg.frame[num_frames].data[4] = 0;
    can_msg.frame[num_frames].data[5] = 0;
    can_msg.frame[num_frames].data[6] = param_val & 0xFF;
    can_msg.frame[num_frames].data[7] = (param_val >> 8) & 0xFF;
    num_frames++;
  }
  can_msg.msg_head.nframes = num_frames;

  if (write(socket_, &can_msg, sizeof(can_msg)) < 0)
  {
    perror(": write: TX_SETUP");
    return errno;
  }
}
