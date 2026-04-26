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
class Nodo_Recepcion_Attitude
    : public IDataNodeBase,
      public std::enable_shared_from_this<Nodo_Recepcion_Attitude> {
private:
  std::shared_ptr<Core::Channel::IChannelT<Core::MspPacket>>
      m_channelAttitudData;
  std::function<void(Core::AttitudeData)> m_productor_raw;
  std::function<void(Core::AttitudeData)> m_productor;
  bool m_esperandoRespuesta = false;

  void onRespuestaRecibida(std::unique_ptr<const Core::MspPacket> pkt) {
    m_esperandoRespuesta = false;
    Core::AttitudeData attdata;
    if (pkt->payload.size() >= 6) {
      int16_t roll_raw =
          static_cast<int16_t>(pkt->payload[0] | (pkt->payload[1] << 8));
      int16_t pitch_raw =
          static_cast<int16_t>(pkt->payload[2] | (pkt->payload[3] << 8));
      int16_t yaw_raw =
          static_cast<int16_t>(pkt->payload[4] | (pkt->payload[5] << 8));

      // todos en grados.
      attdata.roll = roll_raw / 10.0f;
      attdata.pitch = pitch_raw / 10.0f;
      attdata.yaw = static_cast<float>(yaw_raw);

      m_productor_raw(attdata);
      m_productor(alineador->alignAttitude(attdata));
    }
  }

  void onCanalCerrado() { m_channelAttitudData.reset(); }

public:
  Nodo_Recepcion_Attitude(
      std::shared_ptr<Core::Channel::IChannelT<Core::MspPacket>>
          virtualChannelAttitudData,
      std::function<void(Core::AttitudeData)> productorAttitudData_raw,
      std::function<void(Core::AttitudeData)> productorAttitudData)
      : m_channelAttitudData(virtualChannelAttitudData),
        m_productor_raw(productorAttitudData_raw),
        m_productor(productorAttitudData) {
    m_channelAttitudData->onPacket =
        [this](std::unique_ptr<const Core::MspPacket> pkt) {
          this->onRespuestaRecibida(std::move(pkt));
        };

    // También nos suscribimos al cierre
    m_channelAttitudData->onClose = [this]() { this->onCanalCerrado(); };
  }
  void transact() override {
    if (!m_channelAttitudData)
      return;
    if (!m_esperandoRespuesta) {
      // Construir y enviar el paquete MSP para solicitar datos IMU
      std::vector<uint8_t> payload; // Vacío para solicitud de datos
      auto paqueteSolicitud = std::make_unique<Core::MspPacket>(
          '<', Core::Protocol::MSP_ATTITUDE_DATA, payload);

      m_channelAttitudData->sendPacket(std::move(paqueteSolicitud));
      m_esperandoRespuesta = true;

    } else {
      m_esperandoRespuesta = false;
    }
  }
};
} // namespace DataNodes
} // namespace DataNode
} // namespace AppLogic
} // namespace FlightProxy
