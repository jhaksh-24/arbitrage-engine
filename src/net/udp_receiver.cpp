#include "arb/net/udp_receiver.hpp"
#include <stdexcept>
#include <winsock2.h>
#include <ws2ipdef.h>

namespace arb::net {

UdpReceiver::UdpReceiver(const std::string &multicast_ip, uint16_t port) {
#ifdef _WIN32
  // Windows requires to initialize the networking subsystem first
  WSAData wsadata;
  if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
    throw std::runtime_error("WSAStartup failed");
  }
#endif

  // 1. Create a Udp Socket (SOCK_DGRAM meaning UDP)
  sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#ifdef _WIN32
  if (sock_ == INVALID_SOCKET)
#else
  if (sock_ == -1)
#endif
  {
    throw std::runtime_error("Failed to create UDP socket");
  }

  // 2. Allow multiple programs to bind to the same port (for debugging
  // purposes)
  int reuse = 1;
  setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse,
             sizeof(reuse));

  // 3. Bind the socket to the port so it listens for incoming traffic
  sockaddr_in local_addr{};
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(port);
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sock_, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
    throw std::runtime_error("Failed to bind UDP socket");
  }

  // 4. Join the Multicast Group (this tells the router "Send me these packets
  // too")
  struct ip_mreq mreq{};
  mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip.c_str());
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);

  if (setsockopt(sock_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&mreq,
                 sizeof(mreq)) < 0) {
    throw std::runtime_error("Failed t o join multicast group");
  }

  // 5. Set the socket to Non-Blocking mode for our HFT while loop
#ifdef _WIN32
  u_long mode = 1;
  ioctlsocket(sock_, FIONBIO, &mode);
#endif
}

UdpReceiver::~UdpReceiver() {
#ifdef _WIN32
  if (sock_ != INVALID_SOCKET) {
    closesocket(sock_);
    WSACleanup();
  }
#else
  if (sock_ != -1) {
    close(sock_);
  }
#endif
}

std::size_t UdpReceiver::poll(void *buffer, std::size_t max_bytes) noexcept {
// Read data from socket into buffer
#ifdef _WIN32
  int flags =
      0; // Windows non-blocking was set via ioctlsocket in the constructor
#else
  int flags =
      MSG_DONTWAIT; // On Linux, this tells recv not to freeze the thread
#endif

  // recv is the raw C function that reads from the network card buffer
  int bytes_read = recv(sock_, (char *)buffer, max_bytes, flags);

  if (bytes_read > 0) {
    return static_cast<std::size_t>(bytes_read);
  }

  // If we get here, there aint any packet waiting for us
  return 0;
}

} // namespace arb::net