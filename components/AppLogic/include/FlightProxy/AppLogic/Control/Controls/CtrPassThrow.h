#pragma once

#include "FlightProxy/AppLogic/Control/IControl.h"
#include "FlightProxy/Core/FlightProxyTypes.h"

#include <functional>

namespace FlightProxy {
namespace AppLogic {
namespace Control {
namespace Controls {

class CtrPassThrow : public IControl {
private:
  std::function<FlightProxy::Core::RCData(void)> RCInputGetter_;
  std::function<void(FlightProxy::Core::RCData)> RCOutputSetter_;

public:
  CtrPassThrow(std::function<FlightProxy::Core::RCData(void)> RCInputGetter,
               std::function<void(FlightProxy::Core::RCData)> RCOutputSetter);
  ~CtrPassThrow();

  void init() override;
  void step() override;
};
} // namespace Controls
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
