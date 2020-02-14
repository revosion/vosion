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
#include "socketcan_sender.h"

using namespace nlohmann;

SocketCANSender::SocketCANSender(const char *iface) {
  iface_ = iface;
  Init();
}

SocketCANSender::~SocketCANSender() {
  Close();
}

int SocketCANSender::Init() {
  socket_ = socket(PF_CAN, SOCK_DGRAM, CAN_BCM);
  if (socket_ < 0) {
    perror(": socket");
    return errno;
  }

  strncpy(ifr_.ifr_name, iface_, IFNAMSIZ);
  if (ioctl(socket_, SIOCGIFINDEX, &ifr_) < 0) {
    perror(": ioctl");
    return errno;
  }

  addr_.can_family = AF_CAN;
  addr_.can_ifindex = ifr_.ifr_ifindex;
  if (connect(socket_, (struct sockaddr*) &addr_, sizeof(addr_)) < 0) {
    perror(": connect");
    return errno;
  }
}

int SocketCANSender::Close() {
  if (close(socket_) < 0) {
    perror(": close");
    return errno;
  }
}

int SocketCANSender::Send(const int msgid, const json msg) {
  struct CanMsg {
    struct bcm_msg_head msg_head;
    struct can_frame frame[1];
  } can_msg;
  can_msg.msg_head.opcode = TX_SEND;
  can_msg.msg_head.can_id = 0;
  can_msg.msg_head.flags = 0;
  can_msg.msg_head.nframes = 1;

  int slave_id = msg.at("slave_id").get<int>();
  int param_addr = msg.at("param_addr").get<int>();
  int operation = msg.at("operation").get<int>();
  int param_val = msg.at("param_val").get<int>();
  can_msg.frame[0].can_dlc = 8;
  can_msg.frame[0].can_id = msgid;
  can_msg.frame[0].data[0] = operation;
  can_msg.frame[0].data[1] = slave_id;
  can_msg.frame[0].data[2] = param_addr & 0xFF;
  can_msg.frame[0].data[3] = (param_addr >> 8) & 0xFF;
  can_msg.frame[0].data[4] = 0;
  can_msg.frame[0].data[5] = 0;
  can_msg.frame[0].data[6] = param_val & 0xFF;
  can_msg.frame[0].data[7] = (param_val >> 8) & 0xFF;

  if (write(socket_, &can_msg, sizeof(can_msg)) < 0) {
    perror(": write: TX_SEND");
    return errno;
  }
}

int SocketCANSender::SendCyclic(const int msgid, const int ival, const std::vector<json> msgs) {
  struct CanMsg {
    struct bcm_msg_head msg_head;
    struct can_frame frame[4];
  } can_msg;

  int ival_sec = ival / 1000000;
  int ival_usec = ival % 1000000;
  can_msg.msg_head.opcode = TX_SETUP;
  can_msg.msg_head.can_id = 0;
  can_msg.msg_head.flags = SETTIMER | STARTTIMER;
  can_msg.msg_head.count = 0;
  can_msg.msg_head.ival2.tv_sec = ival_sec;
  can_msg.msg_head.ival2.tv_usec = ival_usec;

  int num_frames = 0;
  for (auto &msg : msgs) {
    int slave_id = msg.at("slave_id").get<int>();
    int param_addr = msg.at("param_addr").get<int>();
    int operation = msg.at("operation").get<int>();
    int param_val = msg.at("param_val").get<int>();
    can_msg.frame[num_frames].can_dlc = 8;
    can_msg.frame[num_frames].can_id = msgid;
    can_msg.frame[num_frames].data[0] = operation;
    can_msg.frame[num_frames].data[1] = slave_id;
    can_msg.frame[num_frames].data[2] = param_addr & 0xFF;
    can_msg.frame[num_frames].data[3] = (param_addr >> 8) & 0xFF;
    can_msg.frame[num_frames].data[4] = 0;
    can_msg.frame[num_frames].data[5] = 0;
    can_msg.frame[num_frames].data[6] = param_val & 0xFF;
    can_msg.frame[num_frames].data[7] = (param_val >> 8) & 0xFF;
    num_frames++;
  }
  can_msg.msg_head.nframes = num_frames;

  if (write(socket_, &can_msg, sizeof(can_msg)) < 0) {
    perror(": write: TX_SETUP");
    return errno;
  }
}
