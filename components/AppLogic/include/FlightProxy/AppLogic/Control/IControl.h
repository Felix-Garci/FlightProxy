#pragma once

namespace FlightProxy {
namespace AppLogic {
namespace Control {
class IControl {
public:
  virtual ~IControl();
  virtual void init();
  virtual void step();
};
} // namespace Control
} // namespace AppLogic
} // namespace FlightProxy
