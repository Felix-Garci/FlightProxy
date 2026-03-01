#include "FlightProxy/AppLogic/Control/ControlStateMachin.h"
#include "FlightProxy/Core/Utils/Logger.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {

ControlStateMachin::ControlStateMachin() { this->reset(); }

void ControlStateMachin::reset() { this->state_ = Estado::DESARMADO; }

uint8_t ControlStateMachin::step(Core::RCNORMData rcn) {
  stateMachin_(rcn);
  uint8_t ret = outputDecision_();

  return ret;
}

void ControlStateMachin::stateMachin_(Core::RCNORMData rcn) {
  // Ojo que el rcn que me llega esta normalizado.
  // deveriamos recivir 1500 de throttle en reposo.
  // que normalizado sale a 0;

  switch (this->state_) {
  case Estado::DESARMADO:
    if (rcn.armed && std::abs(rcn.throttle) < 0.05) {

      FP_LOG_D("CTRL_STATE", "Desarmado->ArmadoTierra");
      this->state_ = Estado::ARMADO_TIERRA;
    }
    break;

  case Estado::ARMADO_TIERRA:
    if (!rcn.armed) {
      FP_LOG_D("CTRL_STATE", "ArmadoTierra->Desarmado");
      this->state_ = Estado::DESARMADO;
    } else if (rcn.throttle > 0.1) {
      FP_LOG_D("CTRL_STATE", "ArmadoTierra->ArmadoBuelo");
      this->state_ = Estado::ARMADO_BUELO;
    }
    break;

  case Estado::ARMADO_BUELO:
    if (!rcn.armed) {
      FP_LOG_D("CTRL_STATE", "ArmadoBuelo->Desarmado");
      this->state_ = Estado::DESARMADO;
    } else if (false) {
      FP_LOG_D("CTRL_STATE", "ArmadoBuelo->ArmadoTierra");
      this->state_ = Estado::ARMADO_TIERRA;
    }
    break;
  }
}

uint8_t ControlStateMachin::outputDecision_() {
  uint8_t ret = 0;

  switch (this->state_) {
  case Estado::DESARMADO:
    ret = 0;
    break;

  case Estado::ARMADO_TIERRA:
    ret = 1;
    break;

  case Estado::ARMADO_BUELO:
    ret = 5;
    break;
  }
  return ret;
}

} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
