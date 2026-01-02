#pragma once
#include "FlightProxy/Core/OSAL/ITask.h"
#include <atomic>
#include <cstring> // para memset
#include <pthread.h>
#include <sched.h>
#include <thread>

namespace FlightProxy {
namespace PlatformLinux {
namespace OSAL {
class LinuxTask : public FlightProxy::Core::OSAL::ITask {
public:
  LinuxTask(TaskFunction func,
            const FlightProxy::Core::OSAL::TaskConfig &config)
      : m_userFunc(func), m_config(config), m_isRunning(false) {}

  virtual ~LinuxTask() {
    stop();
    join();
  }

  void start() override {
    if (m_isRunning.load()) {
      return;
    }

    m_isRunning.store(true);

    m_thread = std::thread([this]() {
      // Configurar nombre del thread (útil para debug en htop/gdb)
      // pthread_setname_np(pthread_self(), "TaskName");

      if (m_userFunc) {
        m_userFunc();
      }
      m_isRunning.store(false);
    });

    // --- Configuración de Prioridad y Afinidad (Linux nativo) ---
    pthread_t nativeHandle = m_thread.native_handle();

    // 1. Configurar Afinidad (Core ID)
    if (m_config.coreId != -1) {
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      CPU_SET(m_config.coreId, &cpuset);
      // Ignoramos errores por simplicidad, pero se podría loguear
      pthread_setaffinity_np(nativeHandle, sizeof(cpu_set_t), &cpuset);
    }

    // 2. Configurar Prioridad
    // En Linux, para cambiar prioridades efectivamente se suele usar SCHED_FIFO
    // o SCHED_RR. Esto requiere permisos de root o CAP_SYS_NICE. Si falla, el
    // hilo corre como SCHED_OTHER (normal).
    int policy = SCHED_FIFO;
    struct sched_param param;
    memset(&param, 0, sizeof(param));

    // Mapeo simple de prioridad 1-10 a prioridades POSIX (1-99)
    // Ajustar según la lógica de tu proyecto
    int min_prio = sched_get_priority_min(policy);
    int max_prio = sched_get_priority_max(policy);

    // Escalamos la prioridad de 0-10 a min-max del sistema
    int linuxPriority =
        min_prio + (m_config.priority * (max_prio - min_prio) / 10);
    param.sched_priority = linuxPriority;

    pthread_setschedparam(nativeHandle, policy, &param);
  }

  void stop() override { m_isRunning.store(false); }

  void join() override {
    if (m_thread.joinable()) {
      m_thread.join();
    }
  }

  bool isRunning() const override { return m_isRunning.load(); }

private:
  std::thread m_thread;
  TaskFunction m_userFunc;
  FlightProxy::Core::OSAL::TaskConfig m_config;
  std::atomic<bool> m_isRunning;
};
} // namespace OSAL
} // namespace PlatformLinux
} // namespace FlightProxy
