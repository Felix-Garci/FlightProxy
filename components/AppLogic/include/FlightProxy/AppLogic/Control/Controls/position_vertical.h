#pragma once

#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"
#include <cstdint>
#include <functional>

namespace FlightProxy {
namespace AppLogic {
namespace Control {
namespace Controls {

class position_vertical {
private:
  std::function<Core::PidCtrlIn(void)> paramGetter_;
  std::function<void(Core::PidCtrlOut)> telSetter_;

  Core::PidCtrlIn ctrlParams_;
  Core::PidCtrlOut ctrlTel_;

  float targetH_ = 0.0f;
  bool initialized_ = false;

public:
  position_vertical();

  void init(std::function<Core::PidCtrlIn(void)> paramGetter,
            std::function<void(Core::PidCtrlOut)> telSetter);
  void reset();
  float step(float h_ref, float h_real, float dt);
};

} // namespace Controls
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
