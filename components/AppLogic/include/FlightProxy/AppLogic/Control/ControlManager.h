#pragma once
#include "FlightProxy/AppLogic/Control/IControl.h"
#include "FlightProxy/Core/FlightProxyTypes.h"
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

  void init(std::function<Core::RCData(void)> inRcGetter,
            std::function<uint16_t(void)> isArmedGetter,
            std::function<uint16_t(void)> selectorModeGetter,
            std::function<std::string(void)> activeControlGetter,
            std::function<uint64_t(void)> samplingPeriodMsGetter,
            std::function<void(Core::RCData)> outRcSetter);

  void addControl(std::string name, std::shared_ptr<IControl> control);
  void start();
  void stop();

private:
  std::function<Core::RCData(void)> inRcGetter_;
  std::function<uint16_t(void)> isArmedGetter_;
  std::function<uint16_t(void)> selectorModeGetter_;

  std::function<std::string(void)> activeControlGetter_;
  std::function<uint64_t(void)> samplingPeriodMsGetter_;

  std::function<void(Core::RCData)> outRcSetter_;

  std::unique_ptr<Core::OSAL::ITask> task_;
  std::map<std::string, std::shared_ptr<IControl>> controls_;
  std::atomic<bool> isRunning_{false};

  void eventLoop();
};
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
