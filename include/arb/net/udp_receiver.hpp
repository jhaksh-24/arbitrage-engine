#pragma once

#include <cstdint>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace arb::net {

class UdpReceiver {
public:
  // We pass the Multicast IP and Port we want to listen to
  UdpReceiver(const std::string &multicast_ip, uint16_t port);

  ~UdpReceiver();

  // Disable copying cuz this manages a raw socket resource
  UdpReceiver(const UdpReceiver &) = delete;

  UdpReceiver &operator=(const UdpReceiver &) = delete;

  // This is the hot path function
  // It attempts to read data into the provided buffer
  // Returns the number of bytes read, or 0 if no data available
  // 'max_bytes' is how big our buffer is
  std::size_t poll(void *buffer, std::size_t max_bytes) noexcept;

private:
#ifdef _WIN32
  SOCKET sock_{INVALID_SOCKET};
#else
  int sock_{-1};
#endif
};

} // namespace arb::net