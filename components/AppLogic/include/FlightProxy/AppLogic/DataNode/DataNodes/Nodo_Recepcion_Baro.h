#pragma once

#include "FlightProxy/AppLogic/DataNode/IDataNodeBase.h"
#include "FlightProxy/Core/Channel/IChannelT.h"
#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/Protocol/MspProtocol.h"

#include <memory>

namespace FlightProxy {
namespace AppLogic {
namespace DataNode {
namespace DataNodes {
class Nodo_Recepcion_Baro
    : public IDataNodeBase,
      public std::enable_shared_from_this<Nodo_Recepcion_Baro> {
private:
  std::shared_ptr<Core::Channel::IChannelT<Core::MspPacket>> m_channelBaroData;
  std::function<void(Core::BaroData)> m_productor;
  bool m_esperandoRespuesta = false;

  void onRespuestaRecibida(std::unique_ptr<const Core::MspPacket> pkt) {
    m_esperandoRespuesta = false;
    Core::BaroData datos_baro;
    int32_t raw_alt =
        (int32_t)((uint32_t)pkt->payload[0] | ((uint32_t)pkt->payload[1] << 8) |
                  ((uint32_t)pkt->payload[2] << 16) |
                  ((uint32_t)pkt->payload[3] << 24));
    datos_baro.altitude = (float)raw_alt / 1000.0f;

    int16_t raw_vel =
        (int16_t)((uint16_t)pkt->payload[4] | ((uint16_t)pkt->payload[5] << 8));
    datos_baro.vertical_vel = (float)raw_vel / 1000.0f;

    m_productor(datos_baro);
  }

  void onCanalCerrado() { m_channelBaroData.reset(); }

public:
  Nodo_Recepcion_Baro(std::shared_ptr<Core::Channel::IChannelT<Core::MspPacket>>
                          virtualChannelBaroData,
                      std::function<void(Core::BaroData)> productorBaroData)
      : m_channelBaroData(virtualChannelBaroData),
        m_productor(productorBaroData) {
    m_channelBaroData->onPacket =
        [this](std::unique_ptr<const Core::MspPacket> pkt) {
          this->onRespuestaRecibida(std::move(pkt));
        };

    // También nos suscribimos al cierre
    m_channelBaroData->onClose = [this]() { this->onCanalCerrado(); };
  }
  void transact() override {
    if (!m_channelBaroData)
      return;
    if (!m_esperandoRespuesta) {
      // Construir y enviar el paquete MSP para solicitar datos Baro
      std::vector<uint8_t> payload; // Vacío para solicitud de datos
      auto paqueteSolicitud = std::make_unique<Core::MspPacket>(
          '<', Core::Protocol::MSP_BARO_DATA, payload);

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
