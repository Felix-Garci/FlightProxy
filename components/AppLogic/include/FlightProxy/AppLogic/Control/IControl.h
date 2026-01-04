#pragma once

namespace FlightProxy {
namespace AppLogic {
namespace Control {
class IControl {
public:
  virtual ~IControl() = default;
  virtual void init() = 0;
  virtual void step() = 0;
};
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
