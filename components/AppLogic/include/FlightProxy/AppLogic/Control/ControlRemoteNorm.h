#pragma once

#include "FlightProxy/Core/FlightProxyTypes.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {

class ControlRemoteNorm {
public:
  ControlRemoteNorm();
  Core::RCNORMData norm(Core::RCData rcin);
  Core::RCData de_norm(Core::RCNORMData rcin);

private:
};

} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
