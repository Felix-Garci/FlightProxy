#include "FlightProxy/AppLogic/Control/Controls/velocity_vertical.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {
namespace Controls {

velocity_vertical::velocity_vertical() {}

void velocity_vertical::init(
    std::function<Core::PidCtrlVertVelIn(void)> paramGetter,
    std::function<void(Core::PidCtrlOut)> telSetter) {
  paramGetter_ = paramGetter;
  telSetter_ = telSetter;
}

void velocity_vertical::reset() {
  ctrlParams_ = paramGetter_();
  integral_ = 0;
  prevError_ = 0;
}

float velocity_vertical::step(float v_ref, float v_real) {

  uint64_t now = Core::OSAL::OSALFactory::getSystemTimeMs();
  float dt = (now - prevTime_) / 1000.0f;
  prevTime_ = now;
  if (dt > 0.5f || dt <= 0.0f)
    dt = 0.01f;

  ctrlTel_.ref = v_ref;
  ctrlTel_.real = v_real;

  float error = v_ref - v_real;

  ctrlTel_.p = ctrlParams_.p * error;

  integral_ += error * dt;
  ctrlTel_.i = ctrlParams_.i * integral_;

  float derivative = (error - prevError_) / dt;
  prevError_ = error;
  ctrlTel_.d = ctrlParams_.d * derivative;

  float total = (float)ctrlParams_.hover + ctrlTel_.p + ctrlTel_.i + ctrlTel_.d;

  if (total > 1) {
    total = 1;
    integral_ -= error * dt;
  } else if (total < 0) {
    total = 0;
    integral_ -= error * dt;
  }
  ctrlTel_.output = total;

  telSetter_(ctrlTel_);

  return total;
}

} // namespace Controls
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
