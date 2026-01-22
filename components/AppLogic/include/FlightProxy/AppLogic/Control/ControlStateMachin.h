#pragma once

#include "FlightProxy/Core/FlightProxyTypes.h"
#include <cstdint>

namespace FlightProxy {
namespace AppLogic {
namespace Control {

class ControlStateMachin {
public:
  ControlStateMachin();
  void reset();
  uint8_t step(Core::RCNORMData rcn);

private:
  void stateMachin_(Core::RCNORMData rcn);
  uint8_t outputDecision_();

  enum class Estado : uint8_t {
    DESARMADO,
    ARMADO_TIERRA,
    ARMADO_BUELO

  };
  Estado state_ = Estado::DESARMADO;
};

} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
