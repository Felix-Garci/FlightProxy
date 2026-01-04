#include "FlightProxy/AppLogic/Control/Controls/CtrPassThrow.h"

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

void CtrPassThrow::step() {

  Core::RCData data = RCInputGetter_();
  RCOutputSetter_(data);
}

} // namespace Controls
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
