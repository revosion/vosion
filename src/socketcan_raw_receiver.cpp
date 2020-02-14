#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <iomanip>
#include "socketcan_sender.h"

#include <iostream>

#include "socketcan_raw_receiver.h"

struct EngineFrame {
  std::uint16_t rpm;
  // TODO: Some more hypothetical data
};

struct VehicleFrame {
  // TODO: Some hypothetical vehicle status measurements
};

struct BodyControllerFrame {
  // TODO: Some hypothetical vehicle settings flags
};

SocketCANRawReceiver::SocketCANRawReceiver(const char *iface) {
  iface_ = iface;
  Init();
}

SocketCANRawReceiver::~SocketCANRawReceiver() {
  Close();
}

int SocketCANRawReceiver::Init(void) {
  /* Open the CAN interface */
  socket_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
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
  if (bind(socket_, (struct sockaddr*) &addr_, sizeof(addr_)) < 0) {
    perror(": bind");
    return errno;
  }

  /* Set socket to non-blocking */
  int flags = fcntl(socket_, F_GETFL, 0);
  if (flags < 0) {
    perror(": fcntl: F_GETFL");
    return errno;
  }

  if (fcntl(socket_, F_SETFL, flags | O_NONBLOCK) < 0) {
    perror(": fcntl: F_SETFL");
    return errno;
  }

  // Set a receive filter so we only receive select CAN IDs

  struct can_filter filter[3];
  filter[0].can_id = 0x333;
  filter[0].can_mask = CAN_SFF_MASK;
  filter[1].can_id = 0x110;
  filter[1].can_mask = CAN_SFF_MASK;
  filter[2].can_id = 0x320;
  filter[2].can_mask = CAN_SFF_MASK;

  if (setsockopt(socket_, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter))
      < 0) {
    perror("setsockopt filter");
    return errno;
  }

  // Enable reception of CAN FD frames
  int enable = 1;
  if (setsockopt(socket_, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable,
                 sizeof(enable)) < 0) {
    perror("setsockopt CAN FD");
    return errno;
  }
}

int SocketCANRawReceiver::Close(void) {
  if (close(socket_) < 0) {
    perror(": close");
    return errno;
  }
}

void SocketCANRawReceiver::ProcessFrame(const struct canfd_frame &msg) {
  switch (msg.can_id) {
    case 0x0A0: {
      EngineFrame engine;
      engine.rpm = be16toh(*(std::uint16_t*) (msg.data + 0));
      std::cout << "RPM: " << engine.rpm << std::endl;
    }
      break;
    case 0x110: {
      // TODO: Work!
      // VehicleFrame vehicle;
      std::cout << "Got 0x110" << std::endl;  // XXX
    }
      break;
    case 0x320: {
      // TODO: Work!
      // BodyControllerFrame bodyController;
      std::cout << "Got 0x320" << std::endl;  // XXX
    }
      break;
    default:
      // Should never get here if the receive filters were set up correctly
      std::cerr << "Unexpected CAN ID: 0x" << msg.can_id << std::endl;
      break;
  }
}

int SocketCANRawReceiver::Read() {
  ssize_t nbytes;
  struct canfd_frame msg_;
  /* Read from the CAN interface */
  nbytes = read(socket_, &msg_, CANFD_MTU);
  if (nbytes < 0) {
    if (errno != EAGAIN) {
      perror(": read");
    }
  } else {

    printf("RX:  ");
    ProcessFrame(msg_);
    printf("\n");
  }
}
