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
  if (len < 3) {
    FP_LOG_E("I2C_TRANS",
             "Orden invalida: Se requieren al menos 3 bytes [Addr, Reg, Len]");
    return;
  }

  uint8_t deviceAddr = data[0]; // Byte 0: Dirección I2C
  uint8_t regAddr = data[1];    // Byte 1: Registro
  uint8_t readSize = data[2];   // Byte 2: Cantidad a leer

  if (readSize == 0)
    return;

  // 2. Preparamos el buffer de recepción
  // Usamos vector para gestión automática de memoria, aunque sea pequeña
  std::vector<uint8_t> rxBuffer(readSize);

  // 3. Ejecución de la transacción (La traducción eléctrica)
  // Esta función de ESP-IDF hace: Start -> Write(Reg) -> Restart ->
  // Read(Data) -> Stop
  esp_err_t ret = i2c_master_write_read_device(
      port_, deviceAddr, &regAddr,
      1, // Escribimos 1 byte (la dirección del registro)
      rxBuffer.data(), readSize, timeoutMs_ / portTICK_PERIOD_MS);

  // 4. Salida (El Resultado)
  if (ret == ESP_OK) {
    if (onData) {
      std::vector<uint8_t> responseBuffer;
      responseBuffer.reserve(3 + readSize);

      responseBuffer.push_back(deviceAddr);
      responseBuffer.push_back(regAddr);
      responseBuffer.push_back(readSize);

      responseBuffer.insert(responseBuffer.end(), rawI2CData.begin(),
                            rawI2CData.end());

      onData(responseBuffer.data(), responseBuffer.size());
    }
  } else {
    FP_LOG_W("I2C_TRANS", "Error leyendo Dev 0x%02X Reg 0x%02X: %s", deviceAddr,
             regAddr, esp_err_to_name(ret));
    // Opcional: Podrías llamar a onClose() o manejar reconexión si es crítico
  }
}

} // namespace Transport
} // namespace PlatformESP32
} // namespace FlightProxy
