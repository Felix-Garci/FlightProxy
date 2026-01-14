#pragma once
#include "FlightProxy/AppLogic/Control/IControl.h"
#include "FlightProxy/Core/OSAL/ITask.h"
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace FlightProxy {
namespace AppLogic {
namespace Control {
class ControlManager : public std::enable_shared_from_this<ControlManager> {
public:
  ControlManager();

  ~ControlManager();

  void init(std::function<std::string(void)> activeControlGetter,
            std::function<uint64_t(void)> samplingPeriodMsGetter);

  void addControl(std::string name, std::unique_ptr<IControl> control);
  void start();
  void stop();

private:
  std::function<std::string(void)> activeControlGetter_;
  std::function<uint64_t(void)> samplingPeriodMsGetter_;
  std::unique_ptr<Core::OSAL::ITask> task_;
  std::map<std::string, std::unique_ptr<IControl>> controls_;
  std::atomic<bool> isRunning_{false};

  void eventLoop();
};
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
