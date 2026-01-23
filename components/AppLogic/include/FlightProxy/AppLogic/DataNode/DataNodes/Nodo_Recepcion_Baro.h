#pragma once

#include "FlightProxy/AppLogic/DataNode/IDataNodeBase.h"
#include "FlightProxy/Core/Channel/IChannelT.h"
#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"

#include <math.h>
#include <memory>

namespace FlightProxy {
namespace AppLogic {
namespace DataNode {
namespace DataNodes {
class Nodo_Recepcion_Baro
    : public IDataNodeBase,
      public std::enable_shared_from_this<Nodo_Recepcion_Baro> {
private:
  std::shared_ptr<Core::Channel::IChannelT<Core::I2CPacket>> m_channelBaroData;
  std::function<void(Core::BaroData)> m_productor;
  bool m_esperandoRespuesta = false;
  uint64_t tiempo_anterior = 0;
  float altitud_anterior = 0;

  void onRespuestaRecibida(std::unique_ptr<const Core::I2CPacket> pkt) {
    m_esperandoRespuesta = false;
    Core::BaroData datos_baro;
    uint32_t presion_raw =
        (pkt->payload[2] << 16) | (pkt->payload[1] << 8) | pkt->payload[0];
    uint32_t temp_raw =
        (pkt->payload[5] << 16) | (pkt->payload[4] << 8) | pkt->payload[3];

    float presion_compensada =
        presion_raw; // bmp390_compensar_presion(presion_raw);

    float altitud_m =
        44330.0 * (1.0 - pow(presion_compensada / 101325.0, 0.1903));

    uint64_t now = Core::OSAL::OSALFactory::getSystemTimeMs();
    float dt = (now - tiempo_anterior) / 1000.0f;
    float velocidad_ms = (altitud_m - altitud_anterior) / dt;

    altitud_anterior = altitud_m;
    tiempo_anterior = now;

    datos_baro.altitude = altitud_m;
    datos_baro.vertical_vel = velocidad_ms;
    m_productor(datos_baro);
  }

  void onCanalCerrado() { m_channelBaroData.reset(); }

public:
  Nodo_Recepcion_Baro(std::shared_ptr<Core::Channel::IChannelT<Core::I2CPacket>>
                          virtualChannelBaroData,
                      std::function<void(Core::BaroData)> productorBaroData)
      : m_channelBaroData(virtualChannelBaroData),
        m_productor(productorBaroData) {
    m_channelBaroData->onPacket =
        [this](std::unique_ptr<const Core::I2CPacket> pkt) {
          this->onRespuestaRecibida(std::move(pkt));
        };

    // También nos suscribimos al cierre
    m_channelBaroData->onClose = [this]() { this->onCanalCerrado(); };
  }
  void transact() override {
    if (!m_channelBaroData)
      return;
    if (!m_esperandoRespuesta) {
      auto paqueteSolicitud = std::make_unique<Core::I2CPacket>();

      paqueteSolicitud->device_addr = 0x77; // Dirección I2C del BMP390
      paqueteSolicitud->reg_addr = 0x04;    // Registro inicial (Presión XLSB)
      paqueteSolicitud->payload_len = 6; // Pedimos 6 bytes (Presión 3 + Temp 3)

      m_channelBaroData->sendPacket(std::move(paqueteSolicitud));
      m_esperandoRespuesta = true;
    } else {
      // Todavía estamos esperando una respuesta, no hacemos nada
      // FP_LOG_W("Nodo_Recepcion_Baro", "Aún esperando respuesta de datos Baro,
      // no se envía nueva solicitud.");
      m_esperandoRespuesta =
          false; // Reiniciamos para evitar bloqueo permanente
    }
  }
};
} // namespace DataNodes
} // namespace DataNode
} // namespace AppLogic
} // namespace FlightProxy
