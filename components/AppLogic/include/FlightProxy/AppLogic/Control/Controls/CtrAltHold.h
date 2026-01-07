#pragma once

#include "FlightProxy/AppLogic/Control/IControl.h"
#include "FlightProxy/Core/FlightProxyTypes.h"

#include <functional>

namespace FlightProxy {
namespace AppLogic {
namespace Control {
namespace Controls {

class CtrAltHold : public IControl {
private:
  std::function<FlightProxy::Core::RCData(void)> RCInputGetter_;
  std::function<FlightProxy::Core::BaroData(void)> BaroDataGetter_;
  std::function<void(FlightProxy::Core::RCData)> RCOutputSetter_;
  std::function<void(FlightProxy::Core::ControlPIDVals)> PIDValsSetter_;
  std::function<FlightProxy::Core::ControlPIDCts(void)> PIDCtsGetter_;

public:
  CtrAltHold(
      std::function<FlightProxy::Core::RCData(void)> RCInputGetter,
      std::function<FlightProxy::Core::BaroData(void)> BaroDataGetter,
      std::function<void(FlightProxy::Core::RCData)> RCOutputSetter,
      std::function<void(FlightProxy::Core::ControlPIDVals)> PIDValsSetter,
      std::function<FlightProxy::Core::ControlPIDCts(void)> PIDCtsGetter);

  ~CtrAltHold();

  void init() override;
  void step() override;
};
} // namespace Controls
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
