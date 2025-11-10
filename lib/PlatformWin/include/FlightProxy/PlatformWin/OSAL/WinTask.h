#pragma once
#include "FlightProxy/Core/OSAL/ITask.h"
#include <thread>
#include <atomic>
#include <windows.h> // Necesario para HANDLE, SetThreadPriority, etc.

namespace FlightProxy
{
    namespace PlatformWin
    {
        namespace OSAL
        {
            class WinTask : public FlightProxy::Core::OSAL::ITask
            {
            public:
                WinTask(TaskFunction func, const FlightProxy::Core::OSAL::TaskConfig &config)
                    : m_userFunc(func), m_config(config), m_isRunning(false)
                {
                }

                virtual ~WinTask()
                {
                    stop();
                    join();
                }

                void start() override
                {
                    if (m_isRunning.load())
                    {
                        return;
                    }

                    m_isRunning.store(true);

                    // Lanzamos el hilo
                    m_thread = std::thread([this]()
                                           {
                        if (m_userFunc)
                        {
                            m_userFunc();
                        }
                        m_isRunning.store(false); });

                    // --- Configuración de Prioridad/Afinidad (Windows nativo) ---
                    // Obtenemos el HANDLE nativo con un cast explícito para evitar el error de compilación
                    HANDLE winHandle = (HANDLE)m_thread.native_handle();

                    int winPriority = THREAD_PRIORITY_NORMAL;
                    if (m_config.priority > 5)
                        winPriority = THREAD_PRIORITY_ABOVE_NORMAL;
                    if (m_config.priority > 8)
                        winPriority = THREAD_PRIORITY_HIGHEST;
                    if (m_config.priority < 5)
                        winPriority = THREAD_PRIORITY_BELOW_NORMAL;

                    // Usamos el handle casteado
                    SetThreadPriority(winHandle, winPriority);

                    if (m_config.coreId != -1)
                    {
                        DWORD_PTR affinityMask = (1ULL << m_config.coreId);
                        SetThreadAffinityMask(winHandle, affinityMask);
                    }
                }

                void stop() override
                {
                    m_isRunning.store(false);
                }

                void join() override
                {
                    if (m_thread.joinable())
                    {
                        m_thread.join();
                    }
                }

                bool isRunning() const override
                {
                    return m_isRunning.load();
                }

            private:
                std::thread m_thread;
                TaskFunction m_userFunc;
                FlightProxy::Core::OSAL::TaskConfig m_config;
                std::atomic<bool> m_isRunning;
            };
        }
    }
}