#pragma once

#include "FlightProxy/AppLogic/DataNode/IDataNodeBase.h"
#include "FlightProxy/Core/Channel/IChannelT.h"
#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"

#include <cmath>
#include <memory>
#include <vector>

namespace FlightProxy {
namespace AppLogic {
namespace DataNode {
namespace DataNodes {

class Nodo_Recepcion_Mag
    : public IDataNodeBase,
      public std::enable_shared_from_this<Nodo_Recepcion_Mag> {
public:
  static constexpr uint8_t QMC5883L_I2C_ADDR = 0x0D;

private:
  static constexpr uint8_t REG_DATA_START = 0x00;
  static constexpr uint8_t REG_CONTROL_1 = 0x09;
  static constexpr uint8_t REG_SET_RESET = 0x0B;

  static constexpr uint8_t CMD_CONTROL_1 = 0x1D;
  static constexpr uint8_t CMD_SET_RESET = 0x01;

  enum class Estado : uint8_t {
    INIT,
    CONF,
    NORMAL,
  };

  std::shared_ptr<Core::Channel::IChannelT<Core::I2CPacket>> m_channelMagData;
  std::function<void(Core::MagData)> m_productor;

  Estado state_ = Estado::INIT;
  bool m_esperandoRespuesta = false;

  void transactStateMachin() {
    auto pkt = std::make_unique<Core::I2CPacket>();
    pkt->device_addr = QMC5883L_I2C_ADDR;

    switch (state_) {
    case Estado::INIT:
      pkt->reg_addr = REG_SET_RESET;
      pkt->payload_len = 1;
      pkt->is_read = false;
      pkt->payload.push_back(CMD_SET_RESET);
      break;

    case Estado::CONF:
      pkt->reg_addr = REG_CONTROL_1;
      pkt->payload_len = 1;
      pkt->is_read = false;
      pkt->payload.push_back(CMD_CONTROL_1);
      break;

    case Estado::NORMAL:
      pkt->reg_addr = REG_DATA_START;
      pkt->payload_len = 6;
      pkt->is_read = true;
      break;
    }

    m_channelMagData->sendPacket(std::move(pkt));
  }

  void onRespRecibStateMachin(std::unique_ptr<const Core::I2CPacket> pkt) {
    switch (state_) {
    case Estado::INIT:
      state_ = Estado::CONF;
      break;

    case Estado::CONF:
      state_ = Estado::NORMAL;
      break;

    case Estado::NORMAL:
      if (pkt->payload.size() >= 6) {
        int16_t x_raw = (int16_t)((pkt->payload[1] << 8) | pkt->payload[0]);
        int16_t y_raw = (int16_t)((pkt->payload[3] << 8) | pkt->payload[2]);
        int16_t z_raw = (int16_t)((pkt->payload[5] << 8) | pkt->payload[4]);

        m_productor(processRawData(x_raw, y_raw, z_raw));
      }
      break;
    }
  }

  void onRespuestaRecibida(std::unique_ptr<const Core::I2CPacket> pkt) {
    onRespRecibStateMachin(std::move(pkt));
    m_esperandoRespuesta = false;
  }

  Core::MagData processRawData(int16_t x_raw, int16_t y_raw, int16_t z_raw) {
    Core::MagData datos_mag;

    datos_mag.mag_x = (float)x_raw / 3000.0f;
    datos_mag.mag_y = (float)y_raw / 3000.0f;
    datos_mag.mag_z = (float)z_raw / 3000.0f;

    return datos_mag;
  }

  void onCanalCerrado() { m_channelMagData.reset(); }

public:
  Nodo_Recepcion_Mag(std::shared_ptr<Core::Channel::IChannelT<Core::I2CPacket>>
                         virtualChannelMagData,
                     std::function<void(Core::MagData)> productorMagData)
      : m_channelMagData(virtualChannelMagData), m_productor(productorMagData) {
    m_channelMagData->onPacket =
        [this](std::unique_ptr<const Core::I2CPacket> pkt) {
          this->onRespuestaRecibida(std::move(pkt));
        };

    m_channelMagData->onClose = [this]() { this->onCanalCerrado(); };
  }

  void transact() override {
    if (!m_channelMagData)
      return;
    if (!m_esperandoRespuesta) {
      transactStateMachin();
      m_esperandoRespuesta = true;
    } else {
      // Step de error/timeout
      m_esperandoRespuesta = false;
    }
  }
};

} // namespace DataNodes
} // namespace DataNode
} // namespace AppLogic
} // namespace FlightProxy
