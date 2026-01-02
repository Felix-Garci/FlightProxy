#pragma once
#include "FlightProxy/Core/OSAL/IMutex.h"
#include <chrono>
#include <mutex>

namespace FlightProxy {
namespace PlatformLinux {
namespace OSAL {
class LinuxMutex : public FlightProxy::Core::OSAL::IMutex {
public:
  LinuxMutex() = default;
  virtual ~LinuxMutex() = default;

  void lock() override { mutex_.lock(); }

  void unlock() override { mutex_.unlock(); }

  bool tryLock(uint32_t timeout_ms) override {
    if (timeout_ms == 0) {
      return mutex_.try_lock();
    }
    return mutex_.try_lock_for(std::chrono::milliseconds(timeout_ms));
  }

private:
  std::recursive_timed_mutex mutex_;
};
} // namespace OSAL
} // namespace PlatformLinux
} // namespace FlightProxy
