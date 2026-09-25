// SweetDisplay FINAL-USB candidate 1: minimal NCM IPv4/TCP control-plane proof.
//
// This static recovery-domain process performs only three volatile operations:
// configure usb0 as 10.77.77.2/30, listen on TCP/48231, and answer the existing
// SWDP HELLO/CAPS/HEARTBEAT/DRAIN probe.  It does not mount storage, use ADB,
// configure a gateway/DNS/DHCP/NAT, or persist any state.

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

namespace {

constexpr uint16_t kPort = 48231;
constexpr size_t kHeaderBytes = 48;
constexpr uint16_t kMajor = 1;
constexpr uint16_t kMinor = 0;
constexpr uint16_t kHello = 1;
constexpr uint16_t kCapabilities = 2;
constexpr uint16_t kControl = 5;
constexpr uint16_t kHeartbeat = 7;
constexpr int kIoTimeoutMs = 3000;

struct UiState {
  const char* usb = "USB: NCM READY";
  const char* ip = "IP: WAITING";
  const char* tcp = "TCP: WAITING";
  int session = 0;
  int heartbeats = 0;
  int drains = 0;
  const char* result = "FINAL USB: ACTIVE";
};

void Publish(const UiState& state) {
  char text[320];
  const int count = snprintf(text, sizeof(text),
      "%s\n%s\n%s\nSESSION: %d / 2\nHEARTBEATS: %d / 8\n"
      "DRAIN: %d / 2\n%s\n",
      state.usb, state.ip, state.tcp, state.session, state.heartbeats,
      state.drains, state.result);
  if (count <= 0 || static_cast<size_t>(count) >= sizeof(text)) return;
  const int fd = open("/tmp/sweetdisplay-usb.state",
                      O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0) return;
  size_t offset = 0;
  while (offset < static_cast<size_t>(count)) {
    const ssize_t written = write(fd, text + offset,
                                  static_cast<size_t>(count) - offset);
    if (written > 0) offset += static_cast<size_t>(written);
    else if (written < 0 && errno == EINTR) continue;
    else break;
  }
  close(fd);
}

uint16_t U16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) |
      static_cast<uint16_t>(static_cast<uint16_t>(p[1]) << 8);
}
uint32_t U32(const uint8_t* p) {
  return static_cast<uint32_t>(U16(p)) |
      (static_cast<uint32_t>(U16(p + 2)) << 16);
}
uint64_t U64(const uint8_t* p) {
  return static_cast<uint64_t>(U32(p)) |
      (static_cast<uint64_t>(U32(p + 4)) << 32);
}
void Put16(uint8_t* p, uint16_t value) {
  p[0] = static_cast<uint8_t>(value);
  p[1] = static_cast<uint8_t>(value >> 8);
}
void Put32(uint8_t* p, uint32_t value) {
  Put16(p, static_cast<uint16_t>(value));
  Put16(p + 2, static_cast<uint16_t>(value >> 16));
}
void Put64(uint8_t* p, uint64_t value) {
  Put32(p, static_cast<uint32_t>(value));
  Put32(p + 4, static_cast<uint32_t>(value >> 32));
}
uint64_t NowNs() {
  timespec now{};
  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return 1;
  return static_cast<uint64_t>(now.tv_sec) * 1000000000ULL +
      static_cast<uint64_t>(now.tv_nsec);
}

bool WaitFd(int fd, short events, int timeout_ms) {
  pollfd item{fd, events, 0};
  for (;;) {
    const int result = poll(&item, 1, timeout_ms);
    if (result > 0) return (item.revents & events) != 0;
    if (result < 0 && errno == EINTR) continue;
    return false;
  }
}

bool ReadExact(int fd, void* output, size_t size) {
  auto* bytes = static_cast<uint8_t*>(output);
  size_t offset = 0;
  while (offset < size) {
    if (!WaitFd(fd, POLLIN, kIoTimeoutMs)) return false;
    const ssize_t count = recv(fd, bytes + offset, size - offset, 0);
    if (count > 0) offset += static_cast<size_t>(count);
    else if (count < 0 && errno == EINTR) continue;
    else return false;
  }
  return true;
}

bool WriteExact(int fd, const void* input, size_t size) {
  const auto* bytes = static_cast<const uint8_t*>(input);
  size_t offset = 0;
  while (offset < size) {
    if (!WaitFd(fd, POLLOUT, kIoTimeoutMs)) return false;
    const ssize_t count = send(fd, bytes + offset, size - offset, MSG_NOSIGNAL);
    if (count > 0) offset += static_cast<size_t>(count);
    else if (count < 0 && errno == EINTR) continue;
    else return false;
  }
  return true;
}

struct Message {
  uint16_t type = 0;
  uint32_t size = 0;
  uint64_t session = 0;
  uint64_t sequence = 0;
  uint8_t payload[32]{};
};

bool ReceiveMessage(int fd, Message* message) {
  uint8_t header[kHeaderBytes]{};
  if (!ReadExact(fd, header, sizeof(header))) return false;
  if (memcmp(header, "SWDP", 4) != 0 || U16(header + 4) != kMajor ||
      U16(header + 6) != kMinor || U16(header + 10) != kHeaderBytes ||
      U32(header + 16) != 0 || U32(header + 20) != 0) return false;
  message->type = U16(header + 8);
  message->size = U32(header + 12);
  message->session = U64(header + 24);
  message->sequence = U64(header + 32);
  if (message->session == 0 || message->sequence == 0 || U64(header + 40) == 0 ||
      message->size > sizeof(message->payload)) return false;
  memset(message->payload, 0, sizeof(message->payload));
  return ReadExact(fd, message->payload, message->size);
}

bool SendMessage(int fd, uint16_t type, uint64_t session, uint64_t sequence,
                 const uint8_t* payload, uint32_t size) {
  uint8_t wire[kHeaderBytes + 32]{};
  memcpy(wire, "SWDP", 4);
  Put16(wire + 4, kMajor);
  Put16(wire + 6, kMinor);
  Put16(wire + 8, type);
  Put16(wire + 10, kHeaderBytes);
  Put32(wire + 12, size);
  Put64(wire + 24, session);
  Put64(wire + 32, sequence);
  Put64(wire + 40, NowNs());
  memcpy(wire + kHeaderBytes, payload, size);
  return WriteExact(fd, wire, kHeaderBytes + size);
}

bool ConfigureUsb0() {
#if defined(SWEETDISPLAY_FINAL_USB_CANDIDATE2)
  prctl(PR_SET_NAME, "FU2_SOCKET", 0, 0, 0);
#elif defined(SWEETDISPLAY_FINAL_USB_CANDIDATE3)
  prctl(PR_SET_NAME, "FU3_SOCKET", 0, 0, 0);
#elif defined(SWEETDISPLAY_FINAL_USB_CANDIDATE4)
  prctl(PR_SET_NAME, "FU4_SOCKET", 0, 0, 0);
#endif
#if defined(SWEETDISPLAY_FINAL_USB_CANDIDATE3) || \
    defined(SWEETDISPLAY_FINAL_USB_CANDIDATE4)
  const int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
#else
  const int fd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
#endif
  if (fd < 0) return false;
  ifreq request{};
  strncpy(request.ifr_name, "usb0", IFNAMSIZ - 1);
  sockaddr_in address{};
  address.sin_family = AF_INET;
  if (inet_pton(AF_INET, "10.77.77.2", &address.sin_addr) != 1) {
    close(fd); return false;
  }
  memcpy(&request.ifr_addr, &address, sizeof(address));
#if defined(SWEETDISPLAY_FINAL_USB_CANDIDATE2)
  prctl(PR_SET_NAME, "FU2_ADDR", 0, 0, 0);
#elif defined(SWEETDISPLAY_FINAL_USB_CANDIDATE3)
  prctl(PR_SET_NAME, "FU3_ADDR", 0, 0, 0);
#elif defined(SWEETDISPLAY_FINAL_USB_CANDIDATE4)
  prctl(PR_SET_NAME, "FU4_ADDR", 0, 0, 0);
#endif
  if (ioctl(fd, SIOCSIFADDR, &request) != 0) { close(fd); return false; }
  sockaddr_in netmask{};
  netmask.sin_family = AF_INET;
  if (inet_pton(AF_INET, "255.255.255.252", &netmask.sin_addr) != 1) {
    close(fd); return false;
  }
  memcpy(&request.ifr_netmask, &netmask, sizeof(netmask));
#if defined(SWEETDISPLAY_FINAL_USB_CANDIDATE2)
  prctl(PR_SET_NAME, "FU2_MASK", 0, 0, 0);
  if (ioctl(fd, SIOCSIFNETMASK, &request) != 0) { close(fd); return false; }
  prctl(PR_SET_NAME, "FU2_GETFLAGS", 0, 0, 0);
  if (ioctl(fd, SIOCGIFFLAGS, &request) != 0) { close(fd); return false; }
#elif defined(SWEETDISPLAY_FINAL_USB_CANDIDATE3)
  prctl(PR_SET_NAME, "FU3_MASK", 0, 0, 0);
  if (ioctl(fd, SIOCSIFNETMASK, &request) != 0) { close(fd); return false; }
  prctl(PR_SET_NAME, "FU3_GETFLAGS", 0, 0, 0);
  if (ioctl(fd, SIOCGIFFLAGS, &request) != 0) { close(fd); return false; }
#elif defined(SWEETDISPLAY_FINAL_USB_CANDIDATE4)
  prctl(PR_SET_NAME, "FU4_MASK", 0, 0, 0);
  if (ioctl(fd, SIOCSIFNETMASK, &request) != 0) { close(fd); return false; }
  prctl(PR_SET_NAME, "FU4_GETFLAGS", 0, 0, 0);
  if (ioctl(fd, SIOCGIFFLAGS, &request) != 0) { close(fd); return false; }
#else
  if (ioctl(fd, SIOCSIFNETMASK, &request) != 0 ||
      ioctl(fd, SIOCGIFFLAGS, &request) != 0) { close(fd); return false; }
#endif
  request.ifr_flags = static_cast<short>(request.ifr_flags | IFF_UP);
#if defined(SWEETDISPLAY_FINAL_USB_CANDIDATE2)
  prctl(PR_SET_NAME, "FU2_UP", 0, 0, 0);
#elif defined(SWEETDISPLAY_FINAL_USB_CANDIDATE3)
  prctl(PR_SET_NAME, "FU3_UP", 0, 0, 0);
#elif defined(SWEETDISPLAY_FINAL_USB_CANDIDATE4)
  prctl(PR_SET_NAME, "FU4_UP", 0, 0, 0);
#endif
  const bool ok = ioctl(fd, SIOCSIFFLAGS, &request) == 0;
  close(fd);
#if defined(SWEETDISPLAY_FINAL_USB_CANDIDATE2)
  prctl(PR_SET_NAME, "sweetdisplay-us", 0, 0, 0);
#elif defined(SWEETDISPLAY_FINAL_USB_CANDIDATE3)
  prctl(PR_SET_NAME, "sweetdisplay-us", 0, 0, 0);
#elif defined(SWEETDISPLAY_FINAL_USB_CANDIDATE4)
  prctl(PR_SET_NAME, "sweetdisplay-us", 0, 0, 0);
#endif
  return ok;
}

int CreateListener() {
  const int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0) return -1;
  int enabled = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(kPort);
  if (inet_pton(AF_INET, "10.77.77.2", &address.sin_addr) != 1 ||
      bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
      listen(fd, 2) != 0) {
    close(fd); return -1;
  }
  return fd;
}

bool RunSession(int client, int ordinal, UiState* state) {
  Message message{};
  if (!ReceiveMessage(client, &message) || message.type != kHello ||
      message.size != 24 || message.sequence != 1 || U32(message.payload) != 1 ||
      U16(message.payload + 4) > U16(message.payload + 6) ||
      U16(message.payload + 4) > 0 || U32(message.payload + 8) > 1 ||
      U32(message.payload + 12) != 0 || U64(message.payload + 16) != 1) return false;
  const uint64_t session = message.session;
  uint8_t hello[24]{};
  Put32(hello, 2);
  Put32(hello + 8, 1);
  Put64(hello + 16, 1);
  if (!SendMessage(client, kHello, session, 1, hello, sizeof(hello))) return false;
  uint8_t caps[32]{};
  Put32(caps, 1);
  Put32(caps + 4, 4U * 1024U * 1024U);
  Put32(caps + 8, 4096);
  Put32(caps + 12, 2160);
  Put32(caps + 16, 1);
  Put32(caps + 20, 1);
  if (!SendMessage(client, kCapabilities, session, 2, caps, sizeof(caps))) return false;
  if (!ReceiveMessage(client, &message) || message.type != kCapabilities ||
      message.size != 32 || message.session != session || message.sequence != 2 ||
      (U32(message.payload) & 1) == 0 || U32(message.payload + 4) == 0 ||
      U32(message.payload + 8) == 0 || U32(message.payload + 12) == 0 ||
      U32(message.payload + 16) != 1 || U32(message.payload + 20) != 1 ||
      U64(message.payload + 24) != 0) return false;
  state->usb = "USB: HOST ENUMERATED";
  state->tcp = "TCP: CONNECTED";
  state->result = "SWDP: ACTIVE";
  state->session = ordinal;
  state->heartbeats = 0;
  Publish(*state);
  for (uint64_t index = 1; index <= 8; ++index) {
    if (!ReceiveMessage(client, &message) || message.type != kHeartbeat ||
        message.size != 8 || message.session != session ||
        message.sequence != index + 2) return false;
    if (!SendMessage(client, kHeartbeat, session, index + 2,
                     message.payload, message.size)) return false;
    state->heartbeats = static_cast<int>(index);
    Publish(*state);
  }
  if (!ReceiveMessage(client, &message) || message.type != kControl ||
      message.size != 8 || message.session != session || message.sequence != 11 ||
      U32(message.payload) != 1 || U32(message.payload + 4) != 0) return false;
  uint8_t acknowledgement[8]{};
  Put32(acknowledgement, 2);
  if (!SendMessage(client, kControl, session, 11, acknowledgement,
                   sizeof(acknowledgement))) return false;
  ++state->drains;
  Publish(*state);
  return true;
}

}  // namespace

int main() {
  UiState state;
  Publish(state);
#if defined(SWEETDISPLAY_FINAL_USB_CANDIDATE2) || \
    defined(SWEETDISPLAY_FINAL_USB_CANDIDATE3) || \
    defined(SWEETDISPLAY_FINAL_USB_CANDIDATE4)
  bool interface_configured = false;
  for (int attempt = 0; attempt < 300; ++attempt) {
    interface_configured = ConfigureUsb0();
    if (interface_configured) break;
    usleep(100000);
  }
  if (!interface_configured) {
    state.usb = "USB: NCM READY";
    state.ip = "IP: ERROR";
    state.result = "FINAL USB: ERROR";
    Publish(state);
    sleep(120);
    return 3;
  }
#else
  bool interface_seen = false;
  bool interface_configured = false;
  for (int attempt = 0; attempt < 300; ++attempt) {
    if (access("/sys/class/net/usb0", F_OK) == 0) {
      interface_seen = true;
      interface_configured = ConfigureUsb0();
      if (interface_configured) break;
    }
    usleep(100000);
  }
  if (!interface_seen || !interface_configured) {
    state.usb = interface_seen ? "USB: NCM READY" : "USB: ERROR";
    state.ip = "IP: ERROR";
    state.result = "FINAL USB: ERROR";
    Publish(state);
    sleep(120);
    return 3;
  }
#endif
  state.ip = "IP: 10 77 77 2 / 30";
  Publish(state);
  const int listener = CreateListener();
  if (listener < 0) {
    state.tcp = "TCP: ERROR";
    state.result = "FINAL USB: ERROR";
    Publish(state);
    sleep(120);
    return 4;
  }
  state.tcp = "TCP: LISTENING";
  Publish(state);
  int completed = 0;
  int attempts = 0;
  while (completed < 2 && attempts < 4) {
    if (!WaitFd(listener, POLLIN, 140000)) break;
    const int client = accept4(listener, nullptr, nullptr, SOCK_CLOEXEC);
    if (client < 0) continue;
    ++attempts;
    const bool ok = RunSession(client, completed + 1, &state);
    shutdown(client, SHUT_RDWR);
    close(client);
    if (ok) ++completed;
  }
  close(listener);
  if (completed == 2) {
    state.session = 2;
    state.heartbeats = 8;
    state.drains = 2;
    state.tcp = "TCP: COMPLETE";
    state.result = "FINAL USB: PASS";
    Publish(state);
    sleep(120);
    return 0;
  }
  state.result = "FINAL USB: ERROR";
  Publish(state);
  sleep(120);
  return 5;
}
