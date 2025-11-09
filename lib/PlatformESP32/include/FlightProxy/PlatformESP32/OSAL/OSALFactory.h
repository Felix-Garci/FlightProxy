#pragma once

#include "FlightProxy/Core/OSAL/ITask.h"
#include "FlightProxy/Core/OSAL/IQueue.h"
#include "FlightProxy/Core/OSAL/IMutex.h"

#include "FreeRTOSTask.h"
#include "FreeRTOSQueue.h"
#include "FreeRTOSMutex.h"

#include <memory>

namespace FlightProxy
{
    namespace PlatformESP32
    {
        namespace OSAL
        {

            struct OSALFactory
            {
                template <typename T>
                static std::unique_ptr<Core::OSAL::IQueue<T>> createQueue(size_t size)
                {
                    return std::make_unique<FreeRTOSQueue<T>>(size);
                }

                static std::unique_ptr<Core::OSAL::IMutex> createMutex()
                {
                    return std::make_unique<FreeRTOSMutex>();
                }

                static std::unique_ptr<Core::OSAL::ITask> createTask(
                    Core::OSAL::ITask::TaskFunction func,
                    const Core::OSAL::TaskConfig &config = Core::OSAL::TaskConfig())
                {
                    return std::make_unique<FreeRTOSTask>(func, config);
                }

                static void sleep(uint32_t ms)
                {
                    vTaskDelay(pdMS_TO_TICKS(ms));
                }
            };
        }
    }
}