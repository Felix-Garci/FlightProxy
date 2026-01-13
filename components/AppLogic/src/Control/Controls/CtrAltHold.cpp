#include "FlightProxy/AppLogic/Control/Controls/CtrAltHold.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {
namespace Controls {
CtrAltHold::CtrAltHold(
    std::function<FlightProxy::Core::RCData(void)> RCInputGetter,
    std::function<FlightProxy::Core::BaroData(void)> BaroDataGetter,
    std::function<void(FlightProxy::Core::RCData)> RCOutputSetter,
    std::function<void(FlightProxy::Core::ControlPIDVals)> PIDValsSetter,
    std::function<FlightProxy::Core::ControlPIDCts(void)> PIDCtsGetter,
    std::function<uint16_t(void)> HoverGetter) {
  RCInputGetter_ = RCInputGetter;
  BaroDataGetter_ = BaroDataGetter;
  RCOutputSetter_ = RCOutputSetter;
  PIDValsSetter_ = PIDValsSetter;
  PIDCtsGetter_ = PIDCtsGetter;
  HoverGetter_ = HoverGetter;
}
CtrAltHold::~CtrAltHold() {}

void CtrAltHold::init() {}

void CtrAltHold::reset() {
  integral = 0;
  prev_error = 0;
  prev_time = Core::OSAL::OSALFactory::getSystemTimeMs();
}

void CtrAltHold::step() {
  // 1. Obtener tiempo actual y calcular dt en segundos reales (float)
  uint64_t now = Core::OSAL::OSALFactory::getSystemTimeMs();
  float dt = (now - prev_time) / 1000.0f;

  // IMPORTANTE: Actualizar prev_time para el próximo ciclo
  prev_time = now;

  // Failsafe para evitar dt gigante en el primer ciclo o tras un parón
  if (dt > 0.5f || dt <= 0.0f)
    dt = 0.01f; // Asumimos 10ms si algo falla

  Core::RCData rcData = RCInputGetter_();
  if (rcData.aux1 < 1800) {
    rcData.roll = 1500;
    rcData.pitch = 1500;
    rcData.throttle = 1000;
    rcData.yaw = 1500;
    reset();
    RCOutputSetter_(rcData);
    was_unarmed = true;
    armed_time = now;
    return;
  } else if (was_unarmed) {
    rcData.roll = 1500;
    rcData.pitch = 1500;
    rcData.throttle = 1000;
    rcData.yaw = 1500;
    RCOutputSetter_(rcData);
    if (now - armed_time > 2000) {
      target_altitud = BaroDataGetter_().altitude;
      was_unarmed = false;
    }
    return;
  }

  float stick_offset = (float)rcData.throttle - 1500.0f;
  if (abs(stick_offset) > 50.0f)
    target_altitud += (stick_offset / 500.0f) * 100.0f * dt;

  float error_alt = target_altitud - BaroDataGetter_().altitude;
  float Kp_pos = 0.5f;
  float velRef_calculada = error_alt * Kp_pos;

  if (velRef_calculada > 200.0f)
    velRef_calculada = 200.0f;
  if (velRef_calculada < -200.0f)
    velRef_calculada = -200.0f;

  // 2. Mapeo de referencia (Stick 1500 = 0 cm/s)
  float velVertical = BaroDataGetter_().vertical_vel;
  // float velMax = 500.0f;
  // float velRef = ((float)rcData.throttle - 1500.0f) * (velMax / 500.0f);
  float velRef = velRef_calculada;

  float error = velRef - velVertical;
  Core::ControlPIDCts ctsPIDData = PIDCtsGetter_();

  // 3. PID con dt correcto
  float p = ctsPIDData.p * error;

  // Integral (solo si dt es coherente)
  integral += error * dt;
  float i = ctsPIDData.i * integral;

  // Derivada (evitamos división por cero)
  float derivative = (error - prev_error) / dt;
  float d = ctsPIDData.d * derivative;

  // 4. Salida sumada al Hover
  float total = (float)HoverGetter_() + p + i + d;

  // 5. Saturación y Anti-Windup (Mantenemos tu lógica)
  if (total > 2000) {
    total = 2000;
    integral -= error * dt;
  } else if (total < 1000) {
    total = 1000;
    integral -= error * dt;
  }

  rcData.throttle = (uint16_t)total;
  RCOutputSetter_(rcData);

  prev_error = error;

  Core::ControlPIDVals valsPIDDATA;
  valsPIDDATA.p = p;
  valsPIDDATA.i = i;
  valsPIDDATA.d = d;
  valsPIDDATA.reference = velRef;
  valsPIDDATA.actual = velVertical;
  valsPIDDATA.output = total;

  PIDValsSetter_(valsPIDDATA);
}

} // namespace Controls
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
