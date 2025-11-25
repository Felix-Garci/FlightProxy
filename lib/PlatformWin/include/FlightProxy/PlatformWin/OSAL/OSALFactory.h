#pragma once

#include "WinTask.h"
#include "WinQueue.h"
#include "WinMutex.h"
#include <memory>

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace OSAL
        {
            struct OSALFactory
            {

                // Factoría de Tareas
                static std::unique_ptr<Core::OSAL::ITask> createTask(Core::OSAL::ITask::TaskFunction func, const Core::OSAL::TaskConfig &config)
                {
                    return std::make_unique<OSAL::WinTask>(func, config);
                }

                // Factoría de Colas
                template <typename T>
                static std::unique_ptr<Core::OSAL::IQueue<T>> createQueue(uint32_t queueLength)
                {
                    return std::make_unique<OSAL::WinQueue<T>>(queueLength);
                }

                // Factoría de Mutex
                static std::unique_ptr<Core::OSAL::IMutex> createMutex()
                {
                    return std::make_unique<OSAL::WinMutex>();
                }

                // sleep
                static void sleep(uint32_t ms)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                }

                // Get current ms time
                static uint64_t getSystemTimeMs()
                {
                    auto now = std::chrono::steady_clock::now();
                    auto duration = now.time_since_epoch();
                    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                }
            };
        }
    }
}