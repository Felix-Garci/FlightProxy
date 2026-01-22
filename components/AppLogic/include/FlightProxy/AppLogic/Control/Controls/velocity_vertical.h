#pragma once

#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"
#include <cstdint>
#include <functional>

namespace FlightProxy {
namespace AppLogic {
namespace Control {
namespace Controls {

class velocity_vertical {
private:
  std::function<Core::PidCtrlVertVelIn(void)> paramGetter_;
  std::function<void(Core::PidCtrlOut)> telSetter_;

  Core::PidCtrlVertVelIn ctrlParams_;
  Core::PidCtrlOut ctrlTel_;

  float integral_ = 0;
  uint64_t prevTime_ = Core::OSAL::OSALFactory::getSystemTimeMs();
  float prevError_ = 0;

public:
  velocity_vertical();

  void init(std::function<Core::PidCtrlVertVelIn(void)> paramGetter,
            std::function<void(Core::PidCtrlOut)> telSetter);
  void reset();
  float step(float v_ref, float v_real);
};

} // namespace Controls
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
