#pragma once

#include "FlightProxy/Core/Transport/ITransport.h"

#include "driver/i2c.h"
#include <cstring>

namespace FlightProxy {
namespace PlatformESP32 {
namespace Transport {

class SimpleI2C : public FlightProxy::Core::Transport::ITransport {
public:
  SimpleI2C(i2c_port_t port, uint32_t timeoutMs = 50);

  ~SimpleI2C() override;

  void open() override;
  void close() override;
  void send(const uint8_t *data, size_t len) override;

private:
  i2c_port_t port_;
  uint32_t timeoutMs_;
};

} // namespace Transport
} // namespace PlatformESP32
} // namespace FlightProxy
