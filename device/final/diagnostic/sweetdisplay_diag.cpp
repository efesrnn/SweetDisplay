// SweetDisplay first-boot diagnostic.
//
// This is deliberately independent of Android Java, Binder, EGL, gralloc and
// vendor graphics libraries.  It is a first-boot probe, not the final renderer.
// It owns DRM/KMS dumb buffers (two in V11), reads one touch evdev node, and
// only observes DWC3-related sysfs state.  It never opens a block device.

// Revision inheritance retains historical build behavior byte-for-byte.
#if defined(SWEETDISPLAY_FINAL_USB)
#define SWEETDISPLAY_DIAGNOSTIC_V11 1
#endif
#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
#define SWEETDISPLAY_DIAGNOSTIC_V10 1
#include "flip_events.h"
#endif
#if defined(SWEETDISPLAY_DIAGNOSTIC_V10)
#define SWEETDISPLAY_DIAGNOSTIC_V9 1
#endif

#include <dirent.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/reboot.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

namespace {

constexpr int kAutoRebootSeconds = 180;
constexpr uint32_t kDrmModeConnected = 1;
constexpr uint32_t kBackground = 0x00101724;
constexpr uint32_t kPanel = 0x001a2b3c;
constexpr uint32_t kAccent = 0x0000d9b5;
constexpr uint32_t kText = 0x00f4f7fb;
constexpr uint32_t kWaiting = 0x00ffc857;
constexpr uint32_t kBad = 0x00ff5c77;

#if defined(SWEETDISPLAY_FINAL_USB)
struct UsbUiStatus {
  char usb[40] = "USB: WAITING";
  char ip[40] = "IP: WAITING";
  char tcp[40] = "TCP: WAITING";
  char session[40] = "SESSION: 0 / 2";
  char heartbeat[40] = "HEARTBEATS: 0 / 8";
  char drain[40] = "DRAIN: 0 / 2";
  char result[40] = "FINAL USB: ACTIVE";
};

UsbUiStatus g_usb_status;

bool ReadUsbUiStatus() {
  const int fd = open("/tmp/sweetdisplay-usb.state", O_RDONLY | O_CLOEXEC);
  if (fd < 0) return false;
  char data[320]{};
  const ssize_t count = read(fd, data, sizeof(data) - 1);
  close(fd);
  if (count <= 0) return false;
  UsbUiStatus next{};
  char* lines[] = {next.usb, next.ip, next.tcp, next.session,
                   next.heartbeat, next.drain, next.result};
  size_t capacities[] = {sizeof(next.usb), sizeof(next.ip), sizeof(next.tcp),
                         sizeof(next.session), sizeof(next.heartbeat),
                         sizeof(next.drain), sizeof(next.result)};
  char* cursor = data;
  for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i) {
    char* end = strchr(cursor, '\n');
    if (!end) return false;
    *end = '\0';
    if (*cursor == '\0' || strlen(cursor) >= capacities[i]) return false;
    strcpy(lines[i], cursor);
    cursor = end + 1;
  }
  if (memcmp(&next, &g_usb_status, sizeof(next)) == 0) return false;
  g_usb_status = next;
  return true;
}
#endif

#if defined(SWEETDISPLAY_DIAGNOSTIC_V2) || defined(SWEETDISPLAY_DIAGNOSTIC_V3) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V4) || defined(SWEETDISPLAY_DIAGNOSTIC_V5)
void WriteAll(int fd, const char* data, size_t size) {
  while (size > 0) {
    const ssize_t written = write(fd, data, size);
    if (written > 0) {
      data += written;
      size -= static_cast<size_t>(written);
    } else if (written < 0 && errno == EINTR) {
      continue;
    } else {
      break;
    }
  }
}

void Checkpoint(const char* stage, const char* detail, int error = 0) {
  timespec now{};
  clock_gettime(CLOCK_BOOTTIME, &now);
  char line[256];
  const int count = snprintf(
#if defined(SWEETDISPLAY_DIAGNOSTIC_V5)
      line, sizeof(line), "SDV5 %lld.%03lld %s %s errno=%d\n",
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V4)
      line, sizeof(line), "SDV4 %lld.%03lld %s %s errno=%d\n",
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V3)
      line, sizeof(line), "SDV3 %lld.%03lld %s %s errno=%d\n",
#else
      line, sizeof(line), "SDV2 %lld.%03lld %s %s errno=%d\n",
#endif
      static_cast<long long>(now.tv_sec),
      static_cast<long long>(now.tv_nsec / 1000000), stage, detail, error);
  if (count <= 0) return;
  const size_t length = static_cast<size_t>(count) < sizeof(line)
      ? static_cast<size_t>(count) : sizeof(line) - 1;
  const char* paths[] = {"/dev/pmsg0", "/dev/kmsg"};
  for (const char* path : paths) {
    const int fd = open(path, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd >= 0) {
      WriteAll(fd, line, length);
      close(fd);
    }
  }
  WriteAll(STDERR_FILENO, line, length);
}

bool SetBacklight(int value) {
  const char* paths[] = {
      "/sys/class/backlight/panel0-backlight/brightness",
      "/sys/class/leds/lcd-backlight/brightness",
  };
  char text[32];
  const int count = snprintf(text, sizeof(text), "%d\n", value);
  for (const char* path : paths) {
    const int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) continue;
    WriteAll(fd, text, static_cast<size_t>(count));
    const int saved_errno = errno;
    close(fd);
    errno = saved_errno;
    return true;
  }
  return false;
}

void BlinkCheckpoint(int pulses) {
  bool available = false;
  for (int pulse = 0; pulse < pulses; ++pulse) {
    available = SetBacklight(20) || available;
    usleep(180000);
    available = SetBacklight(200) || available;
    usleep(220000);
  }
  Checkpoint("VISIBLE", available ? "BACKLIGHT_PULSES_OK" :
             "BACKLIGHT_PULSES_UNAVAILABLE", available ? 0 : errno);
}

[[noreturn]] void RequestReboot(const char* reason) {
  Checkpoint("C12", reason, 0);
  syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
          LINUX_REBOOT_CMD_RESTART2, reason);
  Checkpoint("C12", "RESTART2_FAILED", errno);
  reboot(RB_AUTOBOOT);
  Checkpoint("C12", "RB_AUTOBOOT_FAILED", errno);
  for (;;) pause();
}
#elif !defined(SWEETDISPLAY_DIAGNOSTIC_V6) && \
    !defined(SWEETDISPLAY_DIAGNOSTIC_V7) && \
    !defined(SWEETDISPLAY_DIAGNOSTIC_V8) && \
    !defined(SWEETDISPLAY_DIAGNOSTIC_V9)
inline void Checkpoint(const char*, const char*, int = 0) {}
#endif

#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
// V5 proved that recovery-domain writes to kmsg are denied under enforcing
// policy.  V6 deliberately performs one such denied open with a short,
// stage-specific thread name.  The resulting AVC is evidence without granting
// a new permission, creating a file, or producing the denial storm that hid
// ueventd's early coldboot diagnostics in V5.
void EvidenceNamed(const char* marker) {
  prctl(PR_SET_NAME, marker, 0, 0, 0);
  const int fd = open("/dev/kmsg", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
  if (fd >= 0) close(fd);
  prctl(PR_SET_NAME, "recovery", 0, 0, 0);
}

void EvidenceMarker(const char* stage, int error) {
  char marker[16];
#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
  snprintf(marker, sizeof(marker), "V11_%.6s_E%d", stage, error);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V10)
  snprintf(marker, sizeof(marker), "V10_%.6s_E%d", stage, error);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V9)
  snprintf(marker, sizeof(marker), "V9_%.7s_E%d", stage, error);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V8)
  snprintf(marker, sizeof(marker), "V8_%.7s_E%d", stage, error);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V7)
  snprintf(marker, sizeof(marker), "V7_%.7s_E%d", stage, error);
#else
  snprintf(marker, sizeof(marker), "V6_%.7s_E%d", stage, error);
#endif

  EvidenceNamed(marker);
}

void SetV6Failure(const char** stage, int* error, const char* value) {
  *stage = value;
  *error = errno;
}
#endif

#if defined(SWEETDISPLAY_DIAGNOSTIC_V9)
struct V9ResourceObservation {
  uint32_t card = 0;
  uint32_t crtcs = 0;
  uint32_t connectors = 0;
  uint32_t encoders = 0;
};

struct V9ConnectorQueryObservation {
  uint32_t index = 0;
  uint32_t query_level = 0;
  uint32_t connection = 0;
  uint32_t modes = 0;
  uint32_t props = 0;
  uint32_t type = 0;
  int error = 0;
};

void EvidenceV9Resource(const V9ResourceObservation& observation) {
  char marker[16];
#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
  snprintf(marker, sizeof(marker), "V11R%uC%uK%uE%u", observation.card,
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V10)
  snprintf(marker, sizeof(marker), "V10R%uC%uK%uE%u", observation.card,
#else
  snprintf(marker, sizeof(marker), "V9R%uC%uK%uE%u", observation.card,
#endif
           observation.crtcs, observation.connectors, observation.encoders);
  EvidenceNamed(marker);
}

void EvidenceV9Connector(const V9ConnectorQueryObservation& observation) {
  char marker[16];
  if (observation.query_level < 2) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
    snprintf(marker, sizeof(marker), "V11K%uQ%uE%d", observation.index,
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V10)
    snprintf(marker, sizeof(marker), "V10K%uQ%uE%d", observation.index,
#else
    snprintf(marker, sizeof(marker), "V9K%uQ%uE%d", observation.index,
#endif
             observation.query_level, observation.error);
  } else {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
    snprintf(marker, sizeof(marker), "V11K%uS%uM%uP%u", observation.index,
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V10)
    snprintf(marker, sizeof(marker), "V10K%uS%uM%uP%u", observation.index,
#else
    snprintf(marker, sizeof(marker), "V9K%uS%uM%uP%u", observation.index,
#endif
             observation.connection, observation.modes, observation.props);
  }
  EvidenceNamed(marker);
}
#endif

#if defined(SWEETDISPLAY_DIAGNOSTIC_V7)
struct ConnectorObservation {
  uint32_t connection = 0;
  uint32_t modes = 0;
  uint32_t type = 0;
};

void EvidenceConnector(size_t index, const ConnectorObservation& observation) {
  char marker[16];
  snprintf(marker, sizeof(marker), "V7C%zuS%uM%uT%u", index,
           observation.connection, observation.modes, observation.type);
  EvidenceNamed(marker);
}
#endif

#if defined(SWEETDISPLAY_DIAGNOSTIC_V8)
struct ResourceObservation {
  uint32_t card = 0;
  uint32_t crtcs = 0;
  uint32_t connectors = 0;
  uint32_t encoders = 0;
};

struct ConnectorQueryObservation {
  uint32_t index = 0;
  uint32_t query_level = 0;
  uint32_t connection = 0;
  uint32_t modes = 0;
  uint32_t type = 0;
  int error = 0;
};

void EvidenceResource(const ResourceObservation& observation) {
  char marker[16];
  snprintf(marker, sizeof(marker), "V8R%uC%uK%uE%u", observation.card,
           observation.crtcs, observation.connectors, observation.encoders);
  EvidenceNamed(marker);
}

void EvidenceConnectorQuery(const ConnectorQueryObservation& observation) {
  char marker[16];
  if (observation.query_level == 0) {
    snprintf(marker, sizeof(marker), "V8K%uQ0E%d", observation.index,
             observation.error);
  } else {
    snprintf(marker, sizeof(marker), "V8K%uQ%uS%uM%uT%u", observation.index,
             observation.query_level, observation.connection,
             observation.modes, observation.type);
  }
  EvidenceNamed(marker);
}
#endif

#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
struct ScanoutBuffer {
  uint32_t handle = 0;
  uint32_t framebuffer_id = 0;
  uint32_t pitch = 0;
  uint64_t size = 0;
  uint32_t* pixels = nullptr;
};
#endif

struct Display {
  int fd = -1;
  uint32_t connector_id = 0;
  uint32_t crtc_id = 0;
  uint32_t handle = 0;
  uint32_t framebuffer_id = 0;
  uint32_t pitch = 0;
  uint64_t size = 0;
  uint32_t* pixels = nullptr;
  drm_mode_modeinfo mode{};
#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
  // Existing fields describe ONLY the CPU draw buffer after setup.
  ScanoutBuffer front{};
  uint64_t flip_cookie = 0;
  bool flip_pending = false;
#endif
};

#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
ScanoutBuffer DrawBuffer(const Display& display) {
  return {display.handle, display.framebuffer_id, display.pitch,
          display.size, display.pixels};
}
void SetDrawBuffer(Display* display, const ScanoutBuffer& buffer) {
  display->handle = buffer.handle;
  display->framebuffer_id = buffer.framebuffer_id;
  display->pitch = buffer.pitch;
  display->size = buffer.size;
  display->pixels = buffer.pixels;
}

bool CreateBackBuffer(Display* display, const char** stage, int* error) {
  display->front = DrawBuffer(*display);  // Already bound by SETCRTC.
  SetDrawBuffer(display, ScanoutBuffer{});
  drm_mode_create_dumb create{};
  create.width = display->mode.hdisplay;
  create.height = display->mode.vdisplay;
  create.bpp = 32;
  if (ioctl(display->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) != 0) {
    SetV6Failure(stage, error, "B2DUMB"); return false;
  }
  display->handle = create.handle;
  display->pitch = create.pitch;
  display->size = create.size;
  if (create.pitch < uint64_t(create.width) * 4 ||
      create.size < uint64_t(create.pitch) * create.height ||
      display->front.pitch < uint64_t(create.width) * 4 ||
      display->front.size < uint64_t(display->front.pitch) * create.height) {
    errno = EOVERFLOW; SetV6Failure(stage, error, "B2SIZE"); return false;
  }
  drm_mode_fb_cmd fb{};
  fb.width = create.width; fb.height = create.height;
  fb.pitch = create.pitch; fb.bpp = 32; fb.depth = 24; fb.handle = create.handle;
  if (ioctl(display->fd, DRM_IOCTL_MODE_ADDFB, &fb) != 0) {
    SetV6Failure(stage, error, "B2FB"); return false;
  }
  display->framebuffer_id = fb.fb_id;
  drm_mode_map_dumb map{}; map.handle = create.handle;
  if (ioctl(display->fd, DRM_IOCTL_MODE_MAP_DUMB, &map) != 0) {
    SetV6Failure(stage, error, "B2MAP"); return false;
  }
  void* pixels = mmap(nullptr, create.size, PROT_READ | PROT_WRITE, MAP_SHARED,
                      display->fd, static_cast<off_t>(map.offset));
  if (pixels == MAP_FAILED) {
    SetV6Failure(stage, error, "B2MMAP"); return false;
  }
  display->pixels = static_cast<uint32_t*>(pixels);
  const int flags = fcntl(display->fd, F_GETFL);
  if (flags < 0 || fcntl(display->fd, F_SETFL, flags | O_NONBLOCK) != 0) {
    SetV6Failure(stage, error, "DRMNB"); return false;
  }
  return true;
}

int64_t MonotonicMs() {
  timespec now{};
  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return -1;
  return int64_t(now.tv_sec) * 1000 + now.tv_nsec / 1000000;
}

bool PresentCompleteFrame(Display* display) {
  static_assert(sizeof(drm_mode_crtc_page_flip) == 24);
  static_assert(DRM_IOCTL_MODE_PAGE_FLIP == 0xc01864b0UL);
  static_assert(offsetof(drm_mode_crtc_page_flip, user_data) == 16);
  static_assert(sizeof(drm_event_vblank) == 32);
  static_assert(offsetof(drm_event_vblank, user_data) == 8);
  static_assert(DRM_EVENT_FLIP_COMPLETE == 2);
  if (display->flip_pending || !display->front.pixels ||
      display->front.framebuffer_id == display->framebuffer_id) {
    EvidenceMarker("OWN", EBUSY); return false;
  }
  drm_mode_crtc_page_flip flip{};
  flip.crtc_id = display->crtc_id;
  flip.fb_id = display->framebuffer_id;
  flip.flags = DRM_MODE_PAGE_FLIP_EVENT;  // Never ASYNC / never a torn fallback.
  flip.user_data = ++display->flip_cookie;
  const int64_t start = MonotonicMs();
  if (start < 0) { EvidenceMarker("CLOCK", errno); return false; }
  // Finish CPU stores before queuing the complete frame to the driver.
  __sync_synchronize();
  if (ioctl(display->fd, DRM_IOCTL_MODE_PAGE_FLIP, &flip) != 0) {
    EvidenceMarker("FLIP", errno); return false;
  }
  display->flip_pending = true;
  const int64_t deadline = start + 2000;
  for (;;) {
    const int64_t now = MonotonicMs();
    if (now < 0) { EvidenceMarker("CLOCK", errno); return false; }
    if (now >= deadline) { EvidenceMarker("FWAIT", ETIMEDOUT); return false; }
    pollfd event_fd{display->fd, POLLIN, 0};
    const int result = poll(&event_fd, 1, static_cast<int>(deadline - now));
    if (result < 0 && errno == EINTR) continue;
    if (result < 0) { EvidenceMarker("FPOLL", errno); return false; }
    if (result == 0) continue;
    if (event_fd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
      EvidenceMarker("FPOLL", EIO); return false;
    }
    if (!(event_fd.revents & POLLIN)) continue;
    uint8_t events[4096];
    const ssize_t count = read(display->fd, events, sizeof(events));
    if (count < 0 && (errno == EINTR || errno == EAGAIN)) continue;
    if (count <= 0) { EvidenceMarker("FREAD", count < 0 ? errno : EIO); return false; }
    const int completed = sweetdisplay::FindFlipCompletion(
        events, static_cast<size_t>(count), flip.user_data);
    if (completed < 0) { EvidenceMarker("FEVENT", EPROTO); return false; }
    if (completed == 0) continue;
    // Only completion releases the old scanout buffer for subsequent CPU writes.
    const ScanoutBuffer released = display->front;
    display->front = DrawBuffer(*display);
    SetDrawBuffer(display, released);
    display->flip_pending = false;
    return true;
  }
}
#endif

struct Touch {
  int fd = -1;
  char name[128]{};
  input_absinfo x_info{};
  input_absinfo y_info{};
  int x = 0;
  int y = 0;
  bool active = false;
  bool have_x = false;
  bool have_y = false;
};

bool TestBit(const unsigned long* bits, unsigned int bit) {
  constexpr unsigned int kWordBits = sizeof(unsigned long) * 8;
  return (bits[bit / kWordBits] & (1UL << (bit % kWordBits))) != 0;
}

bool ContainsIgnoreCase(const char* haystack, const char* needle) {
  if (!haystack || !needle) return false;
  const size_t needle_len = strlen(needle);
  for (const char* p = haystack; *p; ++p) {
    size_t i = 0;
    while (i < needle_len && p[i] &&
           (p[i] | 0x20) == (needle[i] | 0x20)) {
      ++i;
    }
    if (i == needle_len) return true;
  }
  return false;
}

#if defined(SWEETDISPLAY_DIAGNOSTIC_V10)
static_assert(sizeof(drm_mode_get_connector) == 80);
static_assert(sizeof(drm_mode_modeinfo) == 68);
static_assert(offsetof(drm_mode_get_connector, encoders_ptr) == 0);
static_assert(offsetof(drm_mode_get_connector, modes_ptr) == 8);
static_assert(offsetof(drm_mode_get_connector, props_ptr) == 16);
static_assert(offsetof(drm_mode_get_connector, prop_values_ptr) == 24);
static_assert(offsetof(drm_mode_get_connector, count_modes) == 32);
static_assert(offsetof(drm_mode_get_connector, count_props) == 36);
static_assert(offsetof(drm_mode_get_connector, count_encoders) == 40);
static_assert(DRM_IOCTL_MODE_GETCONNECTOR == 0xc05064a7UL);
constexpr bool QueryCountsFit(uint32_t a, uint32_t b, uint32_t c,
                              uint32_t capacity_a, uint32_t capacity_b,
                              uint32_t capacity_c) {
  return a <= capacity_a && b <= capacity_b && c <= capacity_c;
}
static_assert(QueryCountsFit(0, 0, 0, 0, 0, 0));
static_assert(QueryCountsFit(2, 3, 4, 2, 3, 4));
static_assert(QueryCountsFit(1, 2, 3, 2, 3, 4));
static_assert(!QueryCountsFit(1, 0, 0, 0, 0, 0));
static_assert(!QueryCountsFit(3, 3, 4, 2, 3, 4));
static_assert(!QueryCountsFit(2, 4, 4, 2, 3, 4));
static_assert(!QueryCountsFit(2, 3, 5, 2, 3, 4));
#endif

bool ReadResources(int fd, drm_mode_card_res* resources, uint32_t** crtcs,
                   uint32_t** connectors, uint32_t** encoders) {
  memset(resources, 0, sizeof(*resources));
  if (ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, resources) != 0) return false;
#if defined(SWEETDISPLAY_DIAGNOSTIC_V10)
  const uint32_t crtc_capacity = resources->count_crtcs;
  const uint32_t connector_capacity = resources->count_connectors;
  const uint32_t encoder_capacity = resources->count_encoders;
  // This new fd owns no FBs. Explicitly request no FB IDs in the second pass.
  resources->count_fbs = 0;
#endif
#if defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
  // A zero count is a valid DRM result. Allocate one unused element so the
  // allocation itself cannot turn a valid zero-count query into a false error.
  *crtcs = static_cast<uint32_t*>(
      calloc(resources->count_crtcs ? resources->count_crtcs : 1,
             sizeof(uint32_t)));
  *connectors = static_cast<uint32_t*>(
      calloc(resources->count_connectors ? resources->count_connectors : 1,
             sizeof(uint32_t)));
  *encoders = static_cast<uint32_t*>(
      calloc(resources->count_encoders ? resources->count_encoders : 1,
             sizeof(uint32_t)));
#else
  *crtcs = static_cast<uint32_t*>(calloc(resources->count_crtcs, sizeof(uint32_t)));
  *connectors = static_cast<uint32_t*>(
      calloc(resources->count_connectors, sizeof(uint32_t)));
  *encoders = static_cast<uint32_t*>(
      calloc(resources->count_encoders, sizeof(uint32_t)));
#endif
  if (!*crtcs || !*connectors || !*encoders) return false;
  resources->crtc_id_ptr = reinterpret_cast<uint64_t>(*crtcs);
  resources->connector_id_ptr = reinterpret_cast<uint64_t>(*connectors);
  resources->encoder_id_ptr = reinterpret_cast<uint64_t>(*encoders);
#if defined(SWEETDISPLAY_DIAGNOSTIC_V10)
  if (ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, resources) != 0) return false;
  if (!QueryCountsFit(resources->count_crtcs, resources->count_connectors,
                      resources->count_encoders, crtc_capacity,
                      connector_capacity, encoder_capacity)) {
    errno = EAGAIN;
    return false;  // Existing bounded outer retry obtains fresh capacities.
  }
  return true;
#else
  return ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, resources) == 0;
#endif
}

#if !defined(SWEETDISPLAY_DIAGNOSTIC_V9)
bool ReadConnector(int fd, uint32_t id, drm_mode_get_connector* connector,
                   drm_mode_modeinfo** modes, uint32_t** encoders
#if defined(SWEETDISPLAY_DIAGNOSTIC_V8)
                   , ConnectorQueryObservation* observation
#endif
                   ) {
  memset(connector, 0, sizeof(*connector));
  connector->connector_id = id;
#if defined(SWEETDISPLAY_DIAGNOSTIC_V8)
  observation->query_level = 0;
  observation->error = 0;
#endif
  if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, connector) != 0) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V8)
    observation->error = errno;
#endif
    return false;
  }
#if defined(SWEETDISPLAY_DIAGNOSTIC_V8)
  observation->query_level = 1;
  observation->connection = connector->connection;
  observation->modes = connector->count_modes;
  observation->type = connector->connector_type;
  *modes = static_cast<drm_mode_modeinfo*>(
      calloc(connector->count_modes ? connector->count_modes : 1,
             sizeof(drm_mode_modeinfo)));
  *encoders = static_cast<uint32_t*>(
      calloc(connector->count_encoders ? connector->count_encoders : 1,
             sizeof(uint32_t)));
#else
  *modes = static_cast<drm_mode_modeinfo*>(
      calloc(connector->count_modes, sizeof(drm_mode_modeinfo)));
  *encoders = static_cast<uint32_t*>(
      calloc(connector->count_encoders, sizeof(uint32_t)));
#endif
  if (!*modes || (connector->count_encoders && !*encoders)) return false;
  connector->modes_ptr = reinterpret_cast<uint64_t>(*modes);
  connector->encoders_ptr = reinterpret_cast<uint64_t>(*encoders);
  if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, connector) != 0) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V8)
    observation->error = errno;
#endif
    return false;
  }
#if defined(SWEETDISPLAY_DIAGNOSTIC_V8)
  observation->query_level = 2;
  observation->connection = connector->connection;
  observation->modes = connector->count_modes;
  observation->type = connector->connector_type;
#endif
  return true;
}
#endif

#if defined(SWEETDISPLAY_DIAGNOSTIC_V9)
bool ReadConnectorV9(int fd, uint32_t id, drm_mode_get_connector* connector,
                     drm_mode_modeinfo** modes, uint32_t** encoders,
                     uint32_t** props, uint64_t** prop_values,
                     V9ConnectorQueryObservation* observation) {
  memset(connector, 0, sizeof(*connector));
  connector->connector_id = id;
  observation->query_level = 0;
  observation->error = 0;
  if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, connector) != 0) {
    observation->error = errno;
    return false;
  }

  observation->query_level = 1;
  observation->connection = connector->connection;
  observation->modes = connector->count_modes;
  observation->props = connector->count_props;
  observation->type = connector->connector_type;

#if defined(SWEETDISPLAY_DIAGNOSTIC_V10)
  const uint32_t mode_capacity = connector->count_modes;
  const uint32_t prop_capacity = connector->count_props;
  const uint32_t encoder_capacity = connector->count_encoders;
#endif

  *modes = static_cast<drm_mode_modeinfo*>(
      calloc(connector->count_modes ? connector->count_modes : 1,
             sizeof(drm_mode_modeinfo)));
  *encoders = static_cast<uint32_t*>(
      calloc(connector->count_encoders ? connector->count_encoders : 1,
             sizeof(uint32_t)));
  *props = static_cast<uint32_t*>(
      calloc(connector->count_props ? connector->count_props : 1,
             sizeof(uint32_t)));
  *prop_values = static_cast<uint64_t*>(
      calloc(connector->count_props ? connector->count_props : 1,
             sizeof(uint64_t)));
  if (!*modes || !*encoders || !*props || !*prop_values) {
    observation->error = errno ? errno : ENOMEM;
    return false;
  }

  connector->modes_ptr = reinterpret_cast<uint64_t>(*modes);
  connector->encoders_ptr = reinterpret_cast<uint64_t>(*encoders);
  connector->props_ptr = reinterpret_cast<uint64_t>(*props);
  connector->prop_values_ptr = reinterpret_cast<uint64_t>(*prop_values);
  if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, connector) != 0) {
    observation->error = errno;
    return false;
  }

#if defined(SWEETDISPLAY_DIAGNOSTIC_V10)
  if (!QueryCountsFit(connector->count_modes, connector->count_props,
                      connector->count_encoders, mode_capacity, prop_capacity,
                      encoder_capacity)) {
    observation->error = EAGAIN;
    errno = EAGAIN;
    return false;  // Never consume unfilled modes or a grown count.
  }
#endif

  observation->query_level = 2;
  observation->connection = connector->connection;
  observation->modes = connector->count_modes;
  observation->props = connector->count_props;
  observation->type = connector->connector_type;
  return true;
}
#endif

#if defined(SWEETDISPLAY_DIAGNOSTIC_V9)
bool InitializeDisplay(Display* display, const char** failure_stage,
                       int* failure_error,
                       V9ResourceObservation* resource_observation,
                       V9ConnectorQueryObservation* observations,
                       size_t* observation_count, size_t observation_capacity) {
  *resource_observation = V9ResourceObservation{};
  *observation_count = 0;
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V8)
bool InitializeDisplay(Display* display, const char** failure_stage,
                       int* failure_error,
                       ResourceObservation* resource_observation,
                       ConnectorQueryObservation* observations,
                       size_t* observation_count, size_t observation_capacity) {
  *resource_observation = ResourceObservation{};
  *observation_count = 0;
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V7)
bool InitializeDisplay(Display* display, const char** failure_stage,
                       int* failure_error, ConnectorObservation* observations,
                       size_t* observation_count, size_t observation_capacity) {
  *observation_count = 0;
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V6)
bool InitializeDisplay(Display* display, const char** failure_stage,
                       int* failure_error) {
#else
bool InitializeDisplay(Display* display) {
#endif
#if defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
  int opened_card = -1;
#endif
  for (int card = 0; card < 4; ++card) {
    char path[64];
    snprintf(path, sizeof(path), "/dev/dri/card%d", card);
    display->fd = open(path, O_RDWR | O_CLOEXEC);
    if (display->fd >= 0) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
      opened_card = card;
#endif
#if !defined(SWEETDISPLAY_DIAGNOSTIC_V6) && \
    !defined(SWEETDISPLAY_DIAGNOSTIC_V7) && \
    !defined(SWEETDISPLAY_DIAGNOSTIC_V8) && \
    !defined(SWEETDISPLAY_DIAGNOSTIC_V9)
      Checkpoint("C7", path, 0);
#endif
      break;
    }
  }
  if (display->fd < 0) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    SetV6Failure(failure_stage, failure_error, "NODE");
#else
    Checkpoint("C7", "DRM_NODE_OPEN_FAILED", errno);
#endif
    return false;
  }
  if (ioctl(display->fd, DRM_IOCTL_SET_MASTER, nullptr) != 0 && errno != EINVAL) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    SetV6Failure(failure_stage, failure_error, "MASTER");
#else
    Checkpoint("C7", "DRM_SET_MASTER_FAILED", errno);
#endif
    return false;
  }

  drm_mode_card_res resources{};
  uint32_t* crtcs = nullptr;
  uint32_t* connectors = nullptr;
  uint32_t* encoders = nullptr;
  if (!ReadResources(display->fd, &resources, &crtcs, &connectors, &encoders)) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V10)
    const int query_error = errno;
    free(crtcs);
    free(connectors);
    free(encoders);
    errno = query_error;
#endif
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    SetV6Failure(failure_stage, failure_error, "RES");
#else
    Checkpoint("C7", "DRM_GET_RESOURCES_FAILED", errno);
#endif
    return false;
  }
#if defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
  resource_observation->card = static_cast<uint32_t>(opened_card);
  resource_observation->crtcs = resources.count_crtcs;
  resource_observation->connectors = resources.count_connectors;
  resource_observation->encoders = resources.count_encoders;
#endif

  uint32_t encoder_id = 0;
  for (uint32_t i = 0; i < resources.count_connectors; ++i) {
    drm_mode_get_connector connector{};
    drm_mode_modeinfo* modes = nullptr;
    uint32_t* connector_encoders = nullptr;
#if defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    uint32_t* connector_props = nullptr;
    uint64_t* connector_prop_values = nullptr;
    V9ConnectorQueryObservation local_observation{};
    local_observation.index = i;
    const bool connector_read = ReadConnectorV9(
        display->fd, connectors[i], &connector, &modes, &connector_encoders,
        &connector_props, &connector_prop_values, &local_observation);
    if (*observation_count < observation_capacity) {
      observations[*observation_count] = local_observation;
      ++*observation_count;
    }
    if (!connector_read) {
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V8)
    ConnectorQueryObservation local_observation{};
    local_observation.index = i;
    const bool connector_read = ReadConnector(
        display->fd, connectors[i], &connector, &modes, &connector_encoders,
        &local_observation);
    if (*observation_count < observation_capacity) {
      observations[*observation_count] = local_observation;
      ++*observation_count;
    }
    if (!connector_read) {
#else
    if (!ReadConnector(display->fd, connectors[i], &connector, &modes,
                       &connector_encoders)) {
#endif
      free(modes);
      free(connector_encoders);
#if defined(SWEETDISPLAY_DIAGNOSTIC_V9)
      free(connector_props);
      free(connector_prop_values);
#endif
      continue;
    }
#if defined(SWEETDISPLAY_DIAGNOSTIC_V7)
    if (*observation_count < observation_capacity) {
      observations[*observation_count].connection = connector.connection;
      observations[*observation_count].modes = connector.count_modes;
      observations[*observation_count].type = connector.connector_type;
      ++*observation_count;
    }
#endif
    if (connector.connection == kDrmModeConnected && connector.count_modes) {
      display->connector_id = connector.connector_id;
      display->mode = modes[0];
      encoder_id = connector.encoder_id;
      if (!encoder_id && connector.count_encoders) encoder_id = connector_encoders[0];
      free(modes);
      free(connector_encoders);
#if defined(SWEETDISPLAY_DIAGNOSTIC_V9)
      free(connector_props);
      free(connector_prop_values);
#endif
      break;
    }
    free(modes);
    free(connector_encoders);
#if defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    free(connector_props);
    free(connector_prop_values);
#endif
  }
  if (!display->connector_id) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V10)
    free(crtcs);
    free(connectors);
    free(encoders);
#endif
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
#if defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    errno = 0;
#endif
    SetV6Failure(failure_stage, failure_error, "CONN");
#else
    Checkpoint("C7", "CONNECTED_CONNECTOR_MISSING", errno);
#endif
    return false;
  }

  if (encoder_id) {
    drm_mode_get_encoder encoder{};
    encoder.encoder_id = encoder_id;
    if (ioctl(display->fd, DRM_IOCTL_MODE_GETENCODER, &encoder) == 0) {
      display->crtc_id = encoder.crtc_id;
    }
  }
  if (!display->crtc_id && resources.count_crtcs) display->crtc_id = crtcs[0];
  free(crtcs);
  free(connectors);
  free(encoders);
  if (!display->crtc_id) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
#if defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    errno = 0;
#endif
    SetV6Failure(failure_stage, failure_error, "CRTC");
#else
    Checkpoint("C7", "CRTC_MISSING", errno);
#endif
    return false;
  }

  drm_mode_create_dumb create{};
  create.width = display->mode.hdisplay;
  create.height = display->mode.vdisplay;
  create.bpp = 32;
  if (ioctl(display->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) != 0) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    SetV6Failure(failure_stage, failure_error, "DUMB");
#else
    Checkpoint("C7", "CREATE_DUMB_FAILED", errno);
#endif
    return false;
  }
  display->handle = create.handle;
  display->pitch = create.pitch;
  display->size = create.size;

  drm_mode_fb_cmd framebuffer{};
  framebuffer.width = create.width;
  framebuffer.height = create.height;
  framebuffer.pitch = create.pitch;
  framebuffer.bpp = 32;
  framebuffer.depth = 24;
  framebuffer.handle = create.handle;
  if (ioctl(display->fd, DRM_IOCTL_MODE_ADDFB, &framebuffer) != 0) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    SetV6Failure(failure_stage, failure_error, "ADDFB");
#else
    Checkpoint("C7", "ADDFB_FAILED", errno);
#endif
    return false;
  }
  display->framebuffer_id = framebuffer.fb_id;

  drm_mode_map_dumb map{};
  map.handle = create.handle;
  if (ioctl(display->fd, DRM_IOCTL_MODE_MAP_DUMB, &map) != 0) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    SetV6Failure(failure_stage, failure_error, "MAP");
#else
    Checkpoint("C7", "MAP_DUMB_FAILED", errno);
#endif
    return false;
  }
  display->pixels = static_cast<uint32_t*>(
      mmap(nullptr, display->size, PROT_READ | PROT_WRITE, MAP_SHARED,
           display->fd, static_cast<off_t>(map.offset)));
  if (display->pixels == MAP_FAILED) {
    display->pixels = nullptr;
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    SetV6Failure(failure_stage, failure_error, "MMAP");
#else
    Checkpoint("C7", "MMAP_DUMB_FAILED", errno);
#endif
    return false;
  }

  drm_mode_crtc crtc{};
  crtc.crtc_id = display->crtc_id;
  crtc.fb_id = display->framebuffer_id;
  crtc.set_connectors_ptr = reinterpret_cast<uint64_t>(&display->connector_id);
  crtc.count_connectors = 1;
  crtc.mode = display->mode;
  crtc.mode_valid = 1;
  if (ioctl(display->fd, DRM_IOCTL_MODE_SETCRTC, &crtc) != 0) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    SetV6Failure(failure_stage, failure_error, "SETCRTC");
#else
    Checkpoint("C8", "LEGACY_SETCRTC_FAILED", errno);
#endif
    return false;
  }
#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
  if (!CreateBackBuffer(display, failure_stage, failure_error)) return false;
#endif
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
  *failure_stage = "OK";
  *failure_error = 0;
#else
  Checkpoint("C8", "LEGACY_SETCRTC_OK", 0);
#endif
  return true;
}

void Fill(Display* display, uint32_t color) {
  for (uint32_t y = 0; y < display->mode.vdisplay; ++y) {
    auto* row = reinterpret_cast<uint32_t*>(
        reinterpret_cast<uint8_t*>(display->pixels) + y * display->pitch);
    for (uint32_t x = 0; x < display->mode.hdisplay; ++x) row[x] = color;
  }
}

void Rect(Display* display, int x, int y, int w, int h, uint32_t color) {
  const int width = display->mode.hdisplay;
  const int height = display->mode.vdisplay;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > width) w = width - x;
  if (y + h > height) h = height - y;
  if (w <= 0 || h <= 0) return;
  for (int py = y; py < y + h; ++py) {
    auto* row = reinterpret_cast<uint32_t*>(
        reinterpret_cast<uint8_t*>(display->pixels) + py * display->pitch);
    for (int px = x; px < x + w; ++px) row[px] = color;
  }
}

// Seven rows, five low-order bits per row.  Only the diagnostic alphabet is
// needed, so the table stays auditable and dependency-free.
const uint8_t* Glyph(char c) {
  static const uint8_t blank[7] = {0,0,0,0,0,0,0};
  static const uint8_t letters[36][7] = {
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},
    {31,4,4,4,4,4,31},{7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14},{14,17,17,15,1,1,14}
  };
  static const uint8_t colon[7] = {0,4,4,0,4,4,0};
  static const uint8_t dash[7] = {0,0,0,31,0,0,0};
  static const uint8_t slash[7] = {1,2,2,4,8,8,16};
  if (c >= 'A' && c <= 'Z') return letters[c - 'A'];
  if (c >= '0' && c <= '9') return letters[26 + c - '0'];
  if (c == ':') return colon;
  if (c == '-') return dash;
  if (c == '/') return slash;
  return blank;
}

void Text(Display* display, int x, int y, int scale, const char* text,
          uint32_t color) {
  for (const char* p = text; *p; ++p, x += scale * 6) {
    const uint8_t* glyph = Glyph(*p >= 'a' && *p <= 'z' ? *p - 32 : *p);
    for (int row = 0; row < 7; ++row) {
      for (int col = 0; col < 5; ++col) {
        if (glyph[row] & (1U << (4 - col))) {
          Rect(display, x + col * scale, y + row * scale, scale, scale, color);
        }
      }
    }
  }
}

bool DetectDwc3() {
  const char* paths[] = {
      "/sys/bus/platform/devices/a600000.dwc3",
      "/sys/devices/platform/soc/a600000.ssusb/a600000.dwc3",
      "/sys/class/udc/a600000.dwc3",
  };
  for (const char* path : paths) if (access(path, F_OK) == 0) return true;
  DIR* directory = opendir("/sys/class/udc");
  if (!directory) return false;
  bool found = false;
  while (dirent* entry = readdir(directory)) {
    if (entry->d_name[0] != '.') { found = true; break; }
  }
  closedir(directory);
  return found;
}

int ReadSelinuxEnforcing() {
  int fd = open("/sys/fs/selinux/enforce", O_RDONLY | O_CLOEXEC);
  if (fd < 0) return -1;
  char value = 0;
  const ssize_t count = read(fd, &value, 1);
  close(fd);
  if (count != 1) return -1;
  if (value == '1') return 1;
  if (value == '0') return 0;
  return -1;
}

bool OpenTouch(Touch* touch) {
  unsigned long ev_bits[(EV_MAX + sizeof(unsigned long) * 8) /
                        (sizeof(unsigned long) * 8)]{};
  unsigned long abs_bits[(ABS_MAX + sizeof(unsigned long) * 8) /
                         (sizeof(unsigned long) * 8)]{};
  for (int index = 0; index < 64; ++index) {
    char path[64];
    snprintf(path, sizeof(path), "/dev/input/event%d", index);
    int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) continue;
    char name[sizeof(touch->name)]{};
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    memset(ev_bits, 0, sizeof(ev_bits));
    memset(abs_bits, 0, sizeof(abs_bits));
    ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits);
    ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits);
    const bool multitouch = TestBit(ev_bits, EV_ABS) &&
        TestBit(abs_bits, ABS_MT_POSITION_X) &&
        TestBit(abs_bits, ABS_MT_POSITION_Y);
    if (multitouch && (ContainsIgnoreCase(name, "goodix") ||
                       ContainsIgnoreCase(name, "touch"))) {
      touch->fd = fd;
      strncpy(touch->name, name, sizeof(touch->name) - 1);
      ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &touch->x_info);
      ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &touch->y_info);
      return true;
    }
    close(fd);
  }
  return false;
}

int ScaleCoordinate(int value, const input_absinfo& info, int extent) {
  const int range = info.maximum - info.minimum;
  if (range <= 0 || extent <= 1) return 0;
  int64_t scaled = static_cast<int64_t>(value - info.minimum) * (extent - 1);
  int result = static_cast<int>(scaled / range);
  if (result < 0) result = 0;
  if (result >= extent) result = extent - 1;
  return result;
}

#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
bool
#else
void
#endif
Render(Display* display, const Touch& touch, bool dwc3, int enforcing,
            int remaining) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
  if (display->flip_pending || display->pixels == display->front.pixels) {
    EvidenceMarker("OWN", EBUSY); return false;
  }
#endif
  Fill(display, kBackground);
  const int w = display->mode.hdisplay;
  const int margin = w / 14;
  const int scale = w >= 1000 ? 5 : 3;
  Rect(display, 0, 0, w, 18, kAccent);
  Text(display, margin, margin, scale + 2, "SWEETDISPLAY", kText);
  Text(display, margin, margin + (scale + 2) * 10, scale,
       "CUSTOM ENVIRONMENT", kAccent);
  const int panel_y = margin + (scale + 2) * 22;
  Rect(display, margin, panel_y, w - 2 * margin, scale * 54, kPanel);
  Text(display, margin * 2, panel_y + scale * 5, scale, "DISPLAY: OK", kText);
  if (touch.fd < 0) {
    Text(display, margin * 2, panel_y + scale * 15, scale,
         "TOUCH: UNKNOWN", kBad);
  } else if (!touch.have_x || !touch.have_y) {
    Text(display, margin * 2, panel_y + scale * 15, scale,
         "TOUCH: WAITING", kWaiting);
  } else {
    char line[64];
    snprintf(line, sizeof(line), "TOUCH: %d / %d", touch.x, touch.y);
    Text(display, margin * 2, panel_y + scale * 15, scale, line, kText);
  }
  Text(display, margin * 2, panel_y + scale * 25, scale,
       dwc3 ? "USB CONTROLLER: DETECTED" : "USB CONTROLLER: UNKNOWN",
       dwc3 ? kText : kWaiting);
  Text(display, margin * 2, panel_y + scale * 35, scale,
       enforcing == 1 ? "SELINUX: ENFORCING" :
       (enforcing == 0 ? "SELINUX: NOT ENFORCING" : "SELINUX: UNKNOWN"),
       enforcing == 1 ? kText : kBad);
  Text(display, margin * 2, panel_y + scale * 45, scale,
#if defined(SWEETDISPLAY_FINAL_USB)
#if defined(SWEETDISPLAY_FINAL_USB_CANDIDATE2)
       "BUILD: FINAL-USB-2", kText);
#elif defined(SWEETDISPLAY_FINAL_USB_CANDIDATE3)
       "BUILD: FINAL-USB-3", kText);
#elif defined(SWEETDISPLAY_FINAL_USB_CANDIDATE4)
       "BUILD: FINAL-USB-4", kText);
#else
       "BUILD: FINAL-USB-1", kText);
#endif
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V11)
       "BUILD: FINAL-BOOT-BRINGUP-V11", kText);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V10)
       "BUILD: FINAL-BOOT-BRINGUP-V10", kText);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V9)
       "BUILD: FINAL-BOOT-BRINGUP-V9", kText);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V8)
       "BUILD: FINAL-BOOT-BRINGUP-V8", kText);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V7)
       "BUILD: FINAL-BOOT-BRINGUP-V7", kText);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V6)
       "BUILD: FINAL-BOOT-BRINGUP-V6", kText);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V5)
       "BUILD: FINAL-BOOT-BRINGUP-V5", kText);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V4)
       "BUILD: FINAL-BOOT-BRINGUP-V4", kText);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V3)
       "BUILD: FINAL-BOOT-BRINGUP-V3", kText);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V2)
       "BUILD: FINAL-BOOT-1B-V2", kText);
#else
       "BUILD: FINAL-BOOT-PREP1", kText);
#endif
#if defined(SWEETDISPLAY_FINAL_USB)
  const int usb_y = panel_y + scale * 60;
  Rect(display, margin, usb_y, w - 2 * margin, scale * 76, kPanel);
  Text(display, margin * 2, usb_y + scale * 5, scale, g_usb_status.usb, kText);
  Text(display, margin * 2, usb_y + scale * 15, scale, g_usb_status.ip, kText);
  Text(display, margin * 2, usb_y + scale * 25, scale, g_usb_status.tcp, kText);
  Text(display, margin * 2, usb_y + scale * 35, scale, g_usb_status.session, kText);
  Text(display, margin * 2, usb_y + scale * 45, scale, g_usb_status.heartbeat, kText);
  Text(display, margin * 2, usb_y + scale * 55, scale, g_usb_status.drain, kText);
  Text(display, margin * 2, usb_y + scale * 65, scale, g_usb_status.result,
       strstr(g_usb_status.result, "PASS") ? kAccent :
       (strstr(g_usb_status.result, "ERROR") ? kBad : kWaiting));
#endif
  char timer[64];
  snprintf(timer, sizeof(timer), "AUTO REBOOT: %d", remaining);
  Text(display, margin, display->mode.vdisplay - margin - scale * 7,
       scale, timer, kWaiting);
#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
  return PresentCompleteFrame(display);
#else
  msync(display->pixels, display->size, MS_ASYNC);
#endif
}

void DestroyDisplay(Display* display) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
  if (display->front.pixels) munmap(display->front.pixels, display->front.size);
  if (display->front.framebuffer_id) {
    uint32_t id = display->front.framebuffer_id;
    ioctl(display->fd, DRM_IOCTL_MODE_RMFB, &id);
  }
  if (display->front.handle) {
    drm_mode_destroy_dumb destroy{}; destroy.handle = display->front.handle;
    ioctl(display->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
  }
  display->front = ScanoutBuffer{};
#endif
  if (display->pixels) munmap(display->pixels, display->size);
  if (display->framebuffer_id) {
    uint32_t framebuffer_id = display->framebuffer_id;
    ioctl(display->fd, DRM_IOCTL_MODE_RMFB, &framebuffer_id);
  }
  if (display->handle) {
    drm_mode_destroy_dumb destroy{};
    destroy.handle = display->handle;
    ioctl(display->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
  }
  if (display->fd >= 0) close(display->fd);
}

}  // namespace

int main(int argc, char** argv) {
#if !defined(SWEETDISPLAY_DIAGNOSTIC_V5) && \
    !defined(SWEETDISPLAY_DIAGNOSTIC_V6) && \
    !defined(SWEETDISPLAY_DIAGNOSTIC_V7) && \
    !defined(SWEETDISPLAY_DIAGNOSTIC_V8) && \
    !defined(SWEETDISPLAY_DIAGNOSTIC_V9)
  (void)argc;
  (void)argv;
#endif
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
  if (argc == 2 && strcmp(argv[1], "--early-probe") == 0) {
    return ReadSelinuxEnforcing() == 1 ? 0 : 3;
  }
  if (argc == 2 && strcmp(argv[1], "--coldboot-watchdog") == 0) {
    sleep(240);
    EvidenceMarker("WATCH", 0);
    return 75;
  }
  EvidenceMarker("COLD", 0);
  const int initial_enforcing = ReadSelinuxEnforcing();
  if (initial_enforcing != 1) {
    EvidenceMarker("SEL", initial_enforcing < 0 ? errno : initial_enforcing);
    sleep(2);
    return 3;
  }
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V2) || defined(SWEETDISPLAY_DIAGNOSTIC_V3) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V4) || defined(SWEETDISPLAY_DIAGNOSTIC_V5)
#if defined(SWEETDISPLAY_DIAGNOSTIC_V5)
  if (argc == 2 && strcmp(argv[1], "--early-probe") == 0) {
    Checkpoint("C4", "EARLY_PROBE_EXEC", 0);
    const int enforcing = ReadSelinuxEnforcing();
    Checkpoint("C4", enforcing == 1 ? "SELINUX_ENFORCING" :
               (enforcing == 0 ? "SELINUX_PERMISSIVE" : "SELINUX_UNKNOWN"),
               enforcing < 0 ? errno : 0);
    return enforcing == 1 ? 0 : 3;
  }
#endif
  Checkpoint("C6", "DIAGNOSTIC_EXEC", 0);
  BlinkCheckpoint(2);
  const int initial_enforcing = ReadSelinuxEnforcing();
  Checkpoint("C4-C6", initial_enforcing == 1 ? "SELINUX_ENFORCING" :
             (initial_enforcing == 0 ? "SELINUX_PERMISSIVE" :
                                      "SELINUX_UNKNOWN"), 0);
  if (initial_enforcing != 1) {
    BlinkCheckpoint(7);
    sleep(3);
#if defined(SWEETDISPLAY_DIAGNOSTIC_V5)
    RequestReboot("sweetdisplay-v5-selinux");
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V4)
    RequestReboot("sweetdisplay-v4-selinux");
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V3)
    RequestReboot("sweetdisplay-v3-selinux");
#else
    RequestReboot("sweetdisplay-v2-selinux");
#endif
  }
#endif
  Display display;
  bool display_ready = false;
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
  const char* display_failure_stage = "NONE";
  int display_failure_error = 0;
#endif
#if defined(SWEETDISPLAY_DIAGNOSTIC_V9)
  V9ResourceObservation v9_resource_observation{};
  V9ConnectorQueryObservation v9_connector_observations[8]{};
  size_t v9_connector_observation_count = 0;
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V8)
  ResourceObservation resource_observation{};
  ConnectorQueryObservation connector_query_observations[8]{};
  size_t connector_query_observation_count = 0;
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V7)
  ConnectorObservation connector_observations[8]{};
  size_t connector_observation_count = 0;
#endif
  for (int attempt = 0; attempt < 30; ++attempt) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    if (InitializeDisplay(&display, &display_failure_stage,
                          &display_failure_error, &v9_resource_observation,
                          v9_connector_observations,
                          &v9_connector_observation_count,
                          sizeof(v9_connector_observations) /
                              sizeof(v9_connector_observations[0]))) {
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V8)
    if (InitializeDisplay(&display, &display_failure_stage,
                          &display_failure_error, &resource_observation,
                          connector_query_observations,
                          &connector_query_observation_count,
                          sizeof(connector_query_observations) /
                              sizeof(connector_query_observations[0]))) {
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V7)
    if (InitializeDisplay(&display, &display_failure_stage,
                          &display_failure_error, connector_observations,
                          &connector_observation_count,
                          sizeof(connector_observations) /
                              sizeof(connector_observations[0]))) {
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V6)
    if (InitializeDisplay(&display, &display_failure_stage,
                          &display_failure_error)) {
#else
    if (InitializeDisplay(&display)) {
#endif
      display_ready = true;
      break;
    }
    DestroyDisplay(&display);
    display = Display{};
    sleep(1);
  }
  if (!display_ready) {
#if defined(SWEETDISPLAY_DIAGNOSTIC_V9)
    EvidenceV9Resource(v9_resource_observation);
    for (size_t i = 0; i < v9_connector_observation_count; ++i) {
      EvidenceV9Connector(v9_connector_observations[i]);
    }
    EvidenceMarker(display_failure_stage, display_failure_error);
    sleep(2);
    return 2;
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V8)
    EvidenceResource(resource_observation);
    for (size_t i = 0; i < connector_query_observation_count; ++i) {
      EvidenceConnectorQuery(connector_query_observations[i]);
    }
    EvidenceMarker(display_failure_stage, display_failure_error);
    sleep(2);
    return 2;
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V7)
    for (size_t i = 0; i < connector_observation_count; ++i) {
      EvidenceConnector(i, connector_observations[i]);
    }
    EvidenceMarker(display_failure_stage, display_failure_error);
    sleep(2);
    return 2;
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V6)
    EvidenceMarker(display_failure_stage, display_failure_error);
    sleep(2);
    return 2;
#else
    Checkpoint("C7", "DRM_INIT_TIMEOUT", errno);
    dprintf(STDERR_FILENO, "sweetdisplay-diag: DRM initialization failed: %s\n",
            strerror(errno));
#if defined(SWEETDISPLAY_DIAGNOSTIC_V2) || defined(SWEETDISPLAY_DIAGNOSTIC_V3) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V4) || defined(SWEETDISPLAY_DIAGNOSTIC_V5)
    BlinkCheckpoint(5);
    sleep(3);
#if defined(SWEETDISPLAY_DIAGNOSTIC_V5)
    RequestReboot("sweetdisplay-v5-drm");
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V4)
    RequestReboot("sweetdisplay-v4-drm");
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V3)
    RequestReboot("sweetdisplay-v3-drm");
#else
    RequestReboot("sweetdisplay-v2-drm");
#endif
#else
    sleep(10);
    reboot(RB_AUTOBOOT);
    return 2;
#endif
#endif
  }

  Touch touch;
  OpenTouch(&touch);
  bool dwc3 = DetectDwc3();
  int enforcing = ReadSelinuxEnforcing();
#if defined(SWEETDISPLAY_DIAGNOSTIC_V2) || defined(SWEETDISPLAY_DIAGNOSTIC_V3) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V4) || defined(SWEETDISPLAY_DIAGNOSTIC_V5)
  Checkpoint("C10", touch.fd >= 0 ? "GOODIX_DISCOVERED" :
             "GOODIX_NOT_YET_DISCOVERED", touch.fd >= 0 ? 0 : errno);
  Checkpoint("C11", dwc3 ? "DWC3_DETECTED" : "DWC3_NOT_DETECTED",
             dwc3 ? 0 : errno);
#endif
  const time_t started = time(nullptr);
  int last_remaining = -1;

#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
  if (!Render(&display, touch, dwc3, enforcing, kAutoRebootSeconds)) {
    DestroyDisplay(&display); return 2;
  }
#else
  Render(&display, touch, dwc3, enforcing, kAutoRebootSeconds);
#endif
  EvidenceMarker("PIXELS", 0);
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V2) || defined(SWEETDISPLAY_DIAGNOSTIC_V3) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V4) || defined(SWEETDISPLAY_DIAGNOSTIC_V5)
  Render(&display, touch, dwc3, enforcing, kAutoRebootSeconds);
  Checkpoint("C9", "FIRST_PIXELS_WRITTEN", 0);
#endif

  for (;;) {
    const int elapsed = static_cast<int>(time(nullptr) - started);
    const int remaining = kAutoRebootSeconds - elapsed;
    if (remaining <= 0) break;
    bool changed = remaining != last_remaining;
    last_remaining = remaining;

    if (touch.fd < 0 && (remaining % 2) == 0) {
      changed = OpenTouch(&touch) || changed;
    }
    if (!dwc3 && (remaining % 2) == 0) {
      dwc3 = DetectDwc3();
      changed = dwc3 || changed;
    }
    if (enforcing != 1 && (remaining % 2) == 0) {
      const int current = ReadSelinuxEnforcing();
      changed = current != enforcing || changed;
      enforcing = current;
    }
#if defined(SWEETDISPLAY_FINAL_USB)
    if (ReadUsbUiStatus()) changed = true;
#endif

    pollfd descriptor{touch.fd, POLLIN, 0};
    const int poll_result = touch.fd >= 0 ? poll(&descriptor, 1, 250) :
                                            (usleep(250000), 0);
    if (poll_result > 0 && (descriptor.revents & POLLIN)) {
      input_event events[32];
      const ssize_t bytes = read(touch.fd, events, sizeof(events));
      if (bytes > 0) {
        const size_t count = static_cast<size_t>(bytes) / sizeof(input_event);
        for (size_t i = 0; i < count; ++i) {
          const input_event& event = events[i];
          if (event.type == EV_ABS && event.code == ABS_MT_POSITION_X) {
            touch.x = ScaleCoordinate(event.value, touch.x_info,
                                      display.mode.hdisplay);
            touch.have_x = true;
          } else if (event.type == EV_ABS && event.code == ABS_MT_POSITION_Y) {
            touch.y = ScaleCoordinate(event.value, touch.y_info,
                                      display.mode.vdisplay);
            touch.have_y = true;
          } else if (event.type == EV_ABS && event.code == ABS_MT_TRACKING_ID) {
            touch.active = event.value >= 0;
          } else if (event.type == EV_SYN && event.code == SYN_REPORT) {
            changed = true;
          }
        }
      }
    }
#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
    if (changed && !Render(&display, touch, dwc3, enforcing, remaining)) {
      DestroyDisplay(&display); return 2;
    }
#else
    if (changed) Render(&display, touch, dwc3, enforcing, remaining);
#endif
  }

#if defined(SWEETDISPLAY_DIAGNOSTIC_V11)
  const bool final_present = Render(&display, touch, dwc3, enforcing, 0);
  DestroyDisplay(&display);
  return final_present ? 0 : 2;
#else
  Render(&display, touch, dwc3, enforcing, 0);
#if defined(SWEETDISPLAY_DIAGNOSTIC_V6) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V7) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V8) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V9)
  DestroyDisplay(&display);
  return 0;
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V2) || defined(SWEETDISPLAY_DIAGNOSTIC_V3) || \
    defined(SWEETDISPLAY_DIAGNOSTIC_V4) || defined(SWEETDISPLAY_DIAGNOSTIC_V5)
#if defined(SWEETDISPLAY_DIAGNOSTIC_V5)
  RequestReboot("sweetdisplay-v5-timer");
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V4)
  RequestReboot("sweetdisplay-v4-timer");
#elif defined(SWEETDISPLAY_DIAGNOSTIC_V3)
  RequestReboot("sweetdisplay-v3-timer");
#else
  RequestReboot("sweetdisplay-v2-timer");
#endif
#else
  sync();
  DestroyDisplay(&display);
  reboot(RB_AUTOBOOT);
  return 0;
#endif
#endif
}
