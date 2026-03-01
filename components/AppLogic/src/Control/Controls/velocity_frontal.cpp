#include "FlightProxy/AppLogic/Control/Controls/velocity_frontal.h"
#include "FlightProxy/Core/Utils/Logger.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {
namespace Controls {

velocity_frontal::velocity_frontal() {}

void velocity_frontal::init(std::function<Core::PidCtrlIn(void)> paramGetter,
                            std::function<void(Core::PidCtrlOut)> telSetter) {
  paramGetter_ = paramGetter;
  telSetter_ = telSetter;
}

void velocity_frontal::reset() {
  ctrlParams_ = paramGetter_();
  integral_ = 0;
  prevError_ = 0;
}

float velocity_frontal::step(float v_ref, float v_real, float dt) {

  if (dt > 0.5f || dt <= 0.0f)
    dt = 0.01f;

  float V_MAX = 0.3;

  ctrlTel_.ref = v_ref * V_MAX;
  ctrlTel_.real = v_real;

  float error = v_ref - v_real;

  ctrlTel_.p = ctrlParams_.p * error;

  integral_ += error * dt;
  ctrlTel_.i = ctrlParams_.i * integral_;

  float derivative = (error - prevError_) / dt;
  prevError_ = error;
  ctrlTel_.d = ctrlParams_.d * derivative;

  float total = ctrlTel_.p + ctrlTel_.i + ctrlTel_.d;

  float max_out = 0.4;
  if (total > max_out) {
    total = max_out;
    integral_ -= error * dt;
  } else if (total < -max_out) {
    total = -max_out;
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
