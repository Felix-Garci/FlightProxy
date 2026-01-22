#pragma once

#include "FlightProxy/AppLogic/DataNode/IDataNodeBase.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"
#include "FlightProxy/Core/Utils/Logger.h"

#include <atomic>
#include <memory>
#include <vector>

namespace FlightProxy {
namespace AppLogic {
namespace DataNode {
class DataNodesManager : public std::enable_shared_from_this<DataNodesManager> {
private:
  struct Job {
    std::shared_ptr<IDataNodeBase> task;
    uint64_t period_ms;
    uint64_t next_run;
  };
  std::vector<Job> m_Jobs;

  std::atomic<bool> isRunning_{false};

  std::unique_ptr<Core::OSAL::ITask> eventTask_;

public:
  DataNodesManager() {}

  void addDataNode(std::shared_ptr<IDataNodeBase> dataNode,
                   uint64_t samplingPeriodMs) {
    Job newJob;
    newJob.task = dataNode;
    newJob.period_ms = samplingPeriodMs;
    newJob.next_run =
        Core::OSAL::OSALFactory::getSystemTimeMs() + samplingPeriodMs;
    m_Jobs.push_back(newJob);
  }

  void start() {
    if (isRunning_)
      return;

    // 2. Configurar y crear la tarea
    Core::OSAL::TaskConfig config;
    config.name = "CmdMgr";
    config.stackSize = 4096;
    config.priority = 2;

    eventTask_ = Core::OSAL::OSALFactory::createTask(
        [this]() { this->eventLoop(); }, // Lambda que llama al bucle
        config);

    if (eventTask_) {
      isRunning_ = true;
      eventTask_->start();
      FP_LOG_I("DataNodesManager", "Tarea iniciada");
    }
  }

  void stop() {
    if (isRunning_ && eventTask_) {
      // Señalizamos que pare (necesitarías un mecanismo mejor para salir del
      // bucle de la cola, como enviar un paquete especial de "STOP" a la cola,
      // o usar un timeout en receive).
      isRunning_ = false;
      // Por simplicidad aquí, forzamos stop si la implementación lo soporta
      eventTask_->stop();
    }
  }

private:
  void eventLoop() {
    while (isRunning_) {
      uint64_t now = Core::OSAL::OSALFactory::getSystemTimeMs();
      uint64_t timeToNextJob = 100; // Default por si no hay trabajos (100ms)

      for (auto &job : m_Jobs) {
        if (now >= job.next_run) {
          job.task->transact();
          // Calcular siguiente ejecución evitando drift
          job.next_run = (job.next_run + job.period_ms <= now)
                             ? now + job.period_ms
                             : (job.next_run + job.period_ms);
        }

        // Calculamos cuánto falta para que le toque a ESTA tarea
        uint64_t diff = (job.next_run > now) ? (job.next_run - now) : 0;

        // Nos quedamos con el tiempo más corto de espera
        if (diff < timeToNextJob) {
          timeToNextJob = diff;
        }
      }

      // Si timeToNextJob es 0, significa que vamos retrasados o toca ya, no
      // dormimos. Si es mayor a 0, dormimos lo justo y necesario.
      if (timeToNextJob > 0) {
        Core::OSAL::OSALFactory::sleep(timeToNextJob);
      }
    }
    FP_LOG_I("DataNodesManager", "Tarea finalizada");
  }
};
} // namespace DataNode
} // namespace AppLogic
} // namespace FlightProxy
