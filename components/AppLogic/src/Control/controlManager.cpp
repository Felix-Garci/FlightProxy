#include "FlightProxy/AppLogic/Control/ControlManager.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {
ControlManager::ControlManager(
    std::function<std::string(void)> activeControlGetter,
    std::function<uint64_t(void)> samplingPeriodMsGetter) {
  activeControlGetter_ = activeControlGetter;
  samplingPeriodMsGetter_ = samplingPeriodMsGetter;
}

ControlManager::~ControlManager() {}

void ControlManager::addControl(std::string name,
                                std::unique_ptr<IControl> control) {
  controls_[name] = std::move(control);
  controls_[name]->init();
}

void ControlManager::start() {
  Core::OSAL::TaskConfig config;
  config.name = "CtrMgr";
  config.stackSize = 4096;
  config.priority = 2;

  task_ = Core::OSAL::OSALFactory::createTask(
      [this]() { this->eventLoop(); }, // Lambda que llama al bucle
      config);

  if (task_) {
    isRunning_ = true;
    task_->start();
  }
}

void ControlManager::stop() {
  if (isRunning_ && task_) {
    isRunning_ = false;
    task_->stop();
  }
}

void ControlManager::eventLoop() {
  uint64_t samplingPeriodMs;
  std::string activeCOntrol;

  while (isRunning_) {
    samplingPeriodMs = samplingPeriodMsGetter_();
    activeCOntrol = activeControlGetter_();

    Core::OSAL::OSALFactory::sleep(samplingPeriodMs);

    if (controls_.count(activeCOntrol) == 1)
      controls_[activeCOntrol]->step();
  }
}

} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
