#pragma once
#include "FlightProxy/Core/OSAL/IQueue.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace OSAL
        {
            template <typename T>
            class WinQueue : public FlightProxy::Core::OSAL::IQueue<T>
            {
            public:
                WinQueue(uint32_t queueLength) : max_size_(queueLength) {}
                virtual ~WinQueue() {}

                bool send(const T &item, uint32_t timeout_ms) override
                {
                    std::unique_lock<std::mutex> lock(mutex_);

                    // Esperar mientras la cola esté llena
                    if (!not_full_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]()
                                            { return queue_.size() < max_size_; }))
                    {
                        return false; // Timeout
                    }

                    queue_.push(item);
                    not_empty_.notify_one(); // Avisar a un posible receptor
                    return true;
                }

                bool receive(T &item, uint32_t timeout_ms) override
                {
                    std::unique_lock<std::mutex> lock(mutex_);

                    // Esperar mientras la cola esté vacía
                    if (!not_empty_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]()
                                             { return !queue_.empty(); }))
                    {
                        return false; // Timeout
                    }

                    item = queue_.front();
                    queue_.pop();
                    not_full_.notify_one(); // Avisar a un posible emisor
                    return true;
                }

            private:
                std::queue<T> queue_;
                const uint32_t max_size_;
                std::mutex mutex_;
                std::condition_variable not_empty_;
                std::condition_variable not_full_;
            };
        }
    }
}