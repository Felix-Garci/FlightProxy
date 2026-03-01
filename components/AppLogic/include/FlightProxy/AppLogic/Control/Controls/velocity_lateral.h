#pragma once

#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"
#include <cstdint>
#include <functional>

namespace FlightProxy {
namespace AppLogic {
namespace Control {
namespace Controls {

class velocity_lateral {
private:
  std::function<Core::PidCtrlIn(void)> paramGetter_;
  std::function<void(Core::PidCtrlOut)> telSetter_;

  Core::PidCtrlIn ctrlParams_;
  Core::PidCtrlOut ctrlTel_;

  float integral_ = 0;
  float prevError_ = 0;

public:
  velocity_lateral();

  void init(std::function<Core::PidCtrlIn(void)> paramGetter,
            std::function<void(Core::PidCtrlOut)> telSetter);
  void reset();
  float step(float v_ref, float v_real, float dt);
};

} // namespace Controls
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
