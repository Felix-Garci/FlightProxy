#include "FlightProxy/AppLogic/Control/Controls/position_vertical.h"

namespace FlightProxy {
namespace AppLogic {
namespace Control {
namespace Controls {

position_vertical::position_vertical() {}

void position_vertical::init(std::function<Core::PidCtrlIn(void)> paramGetter,
                             std::function<void(Core::PidCtrlOut)> telSetter) {
  paramGetter_ = paramGetter;
  telSetter_ = telSetter;
}

void position_vertical::reset() {
  ctrlParams_ = paramGetter_();
  initialized_ = false;
}

float position_vertical::step(float h_ref, float h_real, float dt) {
  if (!initialized_) {
    targetH_ = h_real;
    initialized_ = true;
  }

  if (dt > 0.5f || dt <= 0.0f)
    dt = 0.01f;

  targetH_ += (h_ref * 1 * dt);

  float error = targetH_ - h_real;

  ctrlTel_.ref = targetH_;
  ctrlTel_.real = h_real;
  ctrlTel_.p = ctrlParams_.p * error;
  ctrlTel_.i = 0;
  ctrlTel_.d = 0;
  ctrlTel_.output = ctrlTel_.p;

  telSetter_(ctrlTel_);

  return ctrlTel_.output;
}

} // namespace Controls
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
