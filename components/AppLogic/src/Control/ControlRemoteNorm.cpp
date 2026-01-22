#include "FlightProxy/AppLogic/Control/ControlRemoteNorm.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {
ControlRemoteNorm::ControlRemoteNorm() {}
Core::RCNORMData ControlRemoteNorm::norm(Core::RCData rcin) {

  Core::RCNORMData rcout;
  rcout.roll = (float)(rcin.roll - 1500) / 500.0f;
  rcout.pitch = (float)(rcin.pitch - 1500) / 500.0f;
  rcout.throttle = (float)(rcin.throttle - 1500) / 500.0f;
  rcout.yaw = (float)(rcin.yaw - 1500) / 500.0f;

  rcout.armed = rcin.aux1 > 1800 ? true : false;

  return rcout;
}
Core::RCData ControlRemoteNorm::de_norm(Core::RCNORMData rcin) {

  Core::RCData rcout;
  rcout.roll = (uint16_t)((rcin.roll * 500.0f) + 1500.0f);
  rcout.pitch = (uint16_t)((rcin.pitch * 500.0f) + 1500.0f);
  rcout.throttle = (uint16_t)((rcin.throttle * 500.0f) + 1500.0f);
  rcout.yaw = (uint16_t)((rcin.yaw * 500.0f) + 1500.0f);

  rcout.aux1 = rcin.armed ? 2000 : 1500;
  return rcout;
}

} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
