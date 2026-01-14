#include "FlightProxy/AppLogic/Control/Controls/CtrPassThrow.h"
#include "FlightProxy/Core/Utils/Logger.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {
namespace Controls {
CtrPassThrow::CtrPassThrow(
    std::function<FlightProxy::Core::RCData(void)> RCInputGetter,
    std::function<void(FlightProxy::Core::RCData)> RCOutputSetter) {
  RCInputGetter_ = RCInputGetter;
  RCOutputSetter_ = RCOutputSetter;
}
CtrPassThrow::~CtrPassThrow() {}

void CtrPassThrow::init() {}

void CtrPassThrow::reset() {}

void CtrPassThrow::step() {

  // FP_LOG_D("Passthrow", "Step");

  Core::RCData data = RCInputGetter_();
  // FP_LOG_D("Passthrow", "throtle : %d", data.throttle);
  // FP_LOG_D("Passthrow", "aux1 : %d", data.aux1);
  RCOutputSetter_(data);
}

} // namespace Controls
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
