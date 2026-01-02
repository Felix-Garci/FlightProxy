#pragma once
#include "FlightProxy/Core/OSAL/IQueue.h"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace FlightProxy {
namespace PlatformLinux {
namespace OSAL {
template <typename T>
class LinuxQueue : public FlightProxy::Core::OSAL::IQueue<T> {
public:
  LinuxQueue(uint32_t queueLength) : max_size_(queueLength) {}
  virtual ~LinuxQueue() {}

  bool send(const T &item, uint32_t timeout_ms) override {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!not_full_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                            [this]() { return queue_.size() < max_size_; })) {
      return false; // Timeout
    }

    queue_.push(item);
    not_empty_.notify_one();
    return true;
  }

  bool receive(T &item, uint32_t timeout_ms) override {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!not_empty_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                             [this]() { return !queue_.empty(); })) {
      return false; // Timeout
    }

    item = queue_.front();
    queue_.pop();
    not_full_.notify_one();
    return true;
  }

private:
  std::queue<T> queue_;
  const uint32_t max_size_;
  std::mutex mutex_;
  std::condition_variable not_empty_;
  std::condition_variable not_full_;
};
} // namespace OSAL
} // namespace PlatformLinux
} // namespace FlightProxy
