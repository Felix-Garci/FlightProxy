#pragma once
#include "ControlRemoteNorm.h"
#include "ControlStateMachin.h"
#include "ControlUniversal.h"

#include "FlightProxy/AppLogic/AlmacenFlexible.h"
#include "FlightProxy/AppLogic/AlmazenFlexibleID.h"

#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/OSAL/ITask.h"
#include <atomic>
#include <memory>

namespace FlightProxy {
namespace AppLogic {
namespace Control {
class ControlMaster : public std::enable_shared_from_this<ControlMaster> {
public:
  ControlMaster();
  ~ControlMaster();

  void init(std::shared_ptr<AlmacenFlexible> bb);

  void start();
  void stop();

private:
  std::unique_ptr<Core::OSAL::ITask> task_;
  std::atomic<bool> isRunning_{false};
  uint32_t periodCicleMS_ = 10;

  std::function<Core::RCData(void)> inRcGetter_;
  std::function<void(Core::RCNORMData)> outRcNormSetter_;
  std::function<void(Core::RCData)> outRcSetter_;

  ControlRemoteNorm normalizer_ = ControlRemoteNorm();
  ControlStateMachin state_ = ControlStateMachin();
  ControlUniversal control_ = ControlUniversal();

  void eventLoop();
};
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
