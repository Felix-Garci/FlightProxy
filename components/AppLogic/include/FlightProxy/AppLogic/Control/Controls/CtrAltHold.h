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
  std::function<uint16_t(void)> HoverGetter_;

  uint64_t prev_time = 0;
  float prev_error = 0;
  float integral = 0;
  bool was_unarmed = false;
  uint64_t armed_time = 0;
  float target_altitud = 0;

public:
  CtrAltHold(
      std::function<FlightProxy::Core::RCData(void)> RCInputGetter,
      std::function<FlightProxy::Core::BaroData(void)> BaroDataGetter,
      std::function<void(FlightProxy::Core::RCData)> RCOutputSetter,
      std::function<void(FlightProxy::Core::ControlPIDVals)> PIDValsSetter,
      std::function<FlightProxy::Core::ControlPIDCts(void)> PIDCtsGetter,
      std::function<uint16_t(void)> HoverGetter);

  ~CtrAltHold();

  void init() override;
  void reset() override;
  void step() override;
};
} // namespace Controls
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
