#include "FlightProxy/PlatformESP32/Transport/SimpleI2C.h"
#include "FlightProxy/Core/Utils/Logger.h"

#include <cstring>
#include <vector>

namespace FlightProxy {
namespace PlatformESP32 {
namespace Transport {

SimpleI2C::SimpleI2C(i2c_port_t port, uint32_t timeoutMs)
    : port_(port), timeoutMs_(timeoutMs) {}

SimpleI2C::~SimpleI2C() { close(); }

void SimpleI2C::open() {
  if (onOpen) {
    onOpen();
  }
}

void SimpleI2C::close() {
  if (onClose) {
    onClose();
  }
}

void SimpleI2C::send(const uint8_t *data, size_t len) {
  if (len < 4) {
    FP_LOG_E("I2C_TRANS", "Orden invalida: Se requieren al menos 4 bytes "
                          "[Addr, Reg, isRead, Len]");
    return;
  }

  uint8_t deviceAddr = data[0];             // Byte 0: Dirección I2C
  uint8_t regAddr = data[1];                // Byte 1: Registro
  bool isRead = static_cast<bool>(data[2]); // Byte 2: 1 = Read, 0 = Write
  uint8_t payloadLen = data[3];             // Byte 3: Longitud del payload

  esp_err_t ret = ESP_FAIL;
  std::vector<uint8_t> rxBuffer; // Buffer para lectura

  // 2. Bifurcación: Lectura vs Escritura
  if (isRead) {
    if (payloadLen == 0)
      return;

    rxBuffer.resize(payloadLen);

    // Write(Reg) -> Restart -> Read(Data)
    ret = i2c_master_write_read_device(port_, deviceAddr, &regAddr, 1,
                                       rxBuffer.data(), payloadLen,
                                       timeoutMs_ / portTICK_PERIOD_MS);

  } else {
    if (len < 4 + payloadLen) {
      FP_LOG_E("I2C_TRANS", "Payload incompleto para escritura. Esperados: %d",
               payloadLen);
      return;
    }

    std::vector<uint8_t> txBuffer;
    txBuffer.reserve(1 + payloadLen);
    txBuffer.push_back(regAddr);
    txBuffer.insert(txBuffer.end(), data + 4, data + 4 + payloadLen);

    // Start -> Write(Reg + Data) -> Stop
    ret = i2c_master_write_to_device(port_, deviceAddr, txBuffer.data(),
                                     txBuffer.size(),
                                     timeoutMs_ / portTICK_PERIOD_MS);
  }

  if (ret == ESP_OK) {
    if (onData) {
      std::vector<uint8_t> responseBuffer;
      size_t totalLen = 4 + (isRead ? payloadLen : 0);
      responseBuffer.reserve(totalLen);

      responseBuffer.push_back(deviceAddr);
      responseBuffer.push_back(regAddr);
      responseBuffer.push_back(isRead ? 1 : 0);
      responseBuffer.push_back(payloadLen);

      if (isRead) {
        responseBuffer.insert(responseBuffer.end(), rxBuffer.begin(),
                              rxBuffer.end());
      }

      onData(responseBuffer.data(), responseBuffer.size());
    }
  } else {
    FP_LOG_W("I2C_TRANS", "Error I2C [Dev: 0x%02X Reg: 0x%02X Modo: %s]: %s",
             deviceAddr, regAddr, (isRead ? "RD" : "WR"), esp_err_to_name(ret));
  }
}

} // namespace Transport
} // namespace PlatformESP32
} // namespace FlightProxy
