#include "FlightProxy/AppLogic/Control/ControlManager.h"
#include "FlightProxy/Core/OSAL/ITask.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {
ControlManager::ControlManager(
    std::function<std::string(void)> activeControlGetter) {
  activeControlGetter_ = activeControlGetter;
}

ControlManager::~ControlManager() {}

void ControlManager::addControl(std::string name,
                                std::unique_ptr<IControl> control) {
  controls_[name] = std::move(control);
  control->init();
}

void ControlManager::start() {
  Core::OSAL::TaskConfig config;
  config.name = "CmdMgr";
  config.stackSize = 4096;
  config.priority = 2;

  task_ = Core::OSAL::OSALFactory::createTask(
      [this]() { this->eventLoop(); }, // Lambda que llama al bucle
      config);

  if (task_) {
    // isRunning_ = true;
    task_->start();
  }
}

void ControlManager::stop() {}

void ControlManager::eventLoop() {}

} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
