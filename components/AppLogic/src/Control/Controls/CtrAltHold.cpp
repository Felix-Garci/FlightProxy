#include "FlightProxy/AppLogic/Control/Controls/CtrAltHold.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {
namespace Controls {
CtrAltHold::CtrAltHold(
    std::function<FlightProxy::Core::RCData(void)> RCInputGetter,
    std::function<FlightProxy::Core::BaroData(void)> BaroDataGetter,
    std::function<void(FlightProxy::Core::RCData)> RCOutputSetter,
    std::function<void(FlightProxy::Core::ControlPIDVals)> PIDValsSetter,
    std::function<FlightProxy::Core::ControlPIDCts(void)> PIDCtsGetter) {
  RCInputGetter_ = RCInputGetter;
  BaroDataGetter_ = BaroDataGetter;
  RCOutputSetter_ = RCOutputSetter;
  PIDValsSetter_ = PIDValsSetter;
  PIDCtsGetter_ = PIDCtsGetter;
}
CtrAltHold::~CtrAltHold() {}

void CtrAltHold::init() {}

void CtrAltHold::step() {

  Core::RCData rcData = RCInputGetter_();
  Core::BaroData brData = BaroDataGetter_();
  Core::ControlPIDCts ctsPIDData = PIDCtsGetter_();

  uint16_t throtle = rcData.throttle;
  float velVertical = brData.vertical_vel;

  // Mapeamos el throtle a velocidad vertical deseada.
  // tenemos de -300 a 300 cm/s
  uint16_t velMax = 300;

  //                 [0 2000] -> [0 1] ->[0 600] -> [-300 300]
  // float velRef = (((throtle) / 2000 ) * 600 ) - 300;
  float velRef = (((float)throtle / 2000) * 2 * velMax) - velMax;

  float error = velRef - velVertical;

  RCOutputSetter_(rcData);

  Core::ControlPIDVals valsPIDDATA;
  valsPIDDATA.p = ctsPIDData.p;
  valsPIDDATA.i = ctsPIDData.i;
  valsPIDDATA.d = ctsPIDData.d;

  valsPIDDATA.reference = velRef;
  valsPIDDATA.actual = velVertical;

  PIDValsSetter_(valsPIDDATA);
}

} // namespace Controls
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
