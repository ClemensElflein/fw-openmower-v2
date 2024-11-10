//
// Created by clemens on 10.11.24.
//

#include "debug_udp_interface.hpp"

#include <cstddef>

#include "lwip/sockets.h"

DebugUDPInterface::DebugUDPInterface(uint16_t listen_port, DebuggableDriver *driver) {
  chDbgAssert(listen_port > 0, "port invalid");
  chDbgAssert(driver != nullptr, "invalid driver");
  listen_port_ = listen_port;
  driver_ = driver;
}
void DebugUDPInterface::Start() {
  driver_->SetRawDataCallback(
      etl::delegate<void(const uint8_t *, size_t)>::create<DebugUDPInterface, &DebugUDPInterface::OnRawDriverData>(
          *this));
  chThdCreateStatic(waThread, sizeof(waThread), NORMALPRIO, &ThreadFuncHelper, this);
}
void DebugUDPInterface::ThreadFunc() {
  chRegSetThreadName("DebugUDPInterface");

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  int sockfd2 = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0 || sockfd2 < 0) {
    return;
  }

  // if (fcntl(sockfd2, F_SETFL, O_NONBLOCK) < 0) {
    // return;
  // }

  // Bind the socket to a port
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(listen_port_);

  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    lwip_close(sockfd);
    return;
  }

  current_client_socket_ = sockfd2;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  while (1) {
    int received =
        recvfrom(sockfd, input_buffer_, sizeof(input_buffer_) - 1, 0, (struct sockaddr *)&client_addr, &client_len);
    if (received > 0) {
      // Enable raw mode for the driver
      driver_->SetRawAccessMode(true);
      chMtxLock(&socket_mutex_);
      target_ip_ = client_addr.sin_addr.s_addr;
      target_port_ = client_addr.sin_port;
      chMtxUnlock(&socket_mutex_);
      driver_->RawDataInput(input_buffer_, received);
    }
  }
}
void DebugUDPInterface::OnRawDriverData(const uint8_t *data, size_t size) {
  chMtxLock(&socket_mutex_);
  struct sockaddr_in multicast_addr;
  memset(&multicast_addr, 0, sizeof(multicast_addr));
  multicast_addr.sin_family = AF_INET;
  multicast_addr.sin_addr.s_addr = (target_ip_);
  multicast_addr.sin_port = (target_port_);
  if (current_client_socket_ >= 0 && target_ip_ > 0 && target_port_ > 0) {
    sendto(current_client_socket_, data, size, 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr));
  }
  chMtxUnlock(&socket_mutex_);
}
void DebugUDPInterface::ThreadFuncHelper(void *instance) {
  static_cast<DebugUDPInterface *>(instance)->ThreadFunc();
}