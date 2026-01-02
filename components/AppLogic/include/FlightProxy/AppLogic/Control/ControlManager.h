#pragma once
#include "FlightProxy/AppLogic/Control/IControl.h"
#include "FlightProxy/Core/OSAL/ITask.h"
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace FlightProxy {
namespace AppLogic {
namespace Control {
class ControlManager {
public:
  ControlManager(std::function<std::string(void)> activeControlGetter);

  ~ControlManager();

  // Inicializa el control y lo anade a su mapa de controles
  void addControl(std::string name, std::unique_ptr<IControl> control);

  // Genera una tarea que cada sampleTime_ revisa el control activo y le da un
  // ciclo
  void start();

  // Paramos la tarea
  void stop();

private:
  std::function<std::string(void)> activeControlGetter_;
  std::unique_ptr<Core::OSAL::ITask> task_;
  std::map<std::string, std::unique_ptr<IControl>> controls_;

  void eventLoop();
};
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
