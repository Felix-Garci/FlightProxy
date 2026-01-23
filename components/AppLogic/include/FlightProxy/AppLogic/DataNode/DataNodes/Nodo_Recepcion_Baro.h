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
  static constexpr uint8_t BMP390_I2C_ADDR = 0x77;
  static constexpr uint8_t REG_CALIB = 0x31;
  static constexpr uint8_t REG_PWR_CTRL = 0x1B;
  static constexpr uint8_t REG_DATA = 0x04;
  static constexpr uint8_t CMD_NORMAL_MODE = 0x33;
  static constexpr float PRESION_NIVEL_MAR = 101325.0f;

  struct BMP3_Calib_Data {
    float T1;
    float T2;
    float T3;
    float P1;
    float P2;
    float P3;
    float P4;
    float P5;
    float P6;
    float P7;
    float P8;
    float P9;
    float P10;
    float P11;
  };

  enum class Estado : uint8_t {
    INIT,
    CONF,
    NORMAL,
  };

  std::shared_ptr<Core::Channel::IChannelT<Core::I2CPacket>> m_channelBaroData;
  std::function<void(Core::BaroData)> m_productor;

  Estado state_ = Nodo_Recepcion_Baro::Estado::INIT;
  BMP3_Calib_Data calib;

  bool m_esperandoRespuesta = false;
  uint64_t tiempo_anterior = 0;
  float altitud_anterior = 0;

  void transactStateMachin() {
    auto pkt = std::make_unique<Core::I2CPacket>();
    pkt->device_addr = BMP390_I2C_ADDR;

    switch (state_) {
    case Estado::INIT:
      pkt->reg_addr = REG_CALIB;
      pkt->payload_len = 21;
      pkt->is_read = true;
      break;

    case Estado::CONF:
      pkt->reg_addr = REG_PWR_CTRL;
      pkt->payload_len = 1;
      pkt->is_read = false;
      pkt->payload.push_back(CMD_NORMAL_MODE);
      break;

    case Estado::NORMAL:
      pkt->reg_addr = REG_DATA;
      pkt->payload_len = 6;
      pkt->is_read = true;
      break;
    }

    m_channelBaroData->sendPacket(std::move(pkt));
  }

  void onRespRecibStateMachin(std::unique_ptr<const Core::I2CPacket> pkt) {
    switch (state_) {
    case Estado::INIT:

      unpackCalib(pkt->payload);
      state_ = Estado::CONF;

      break;
    case Estado::CONF:
      // Aqui podemos confirmar que el paquete de conf se recive de buelta.
      state_ = Estado::NORMAL;
      break;
    case Estado::NORMAL:
      uint32_t presion_raw =
          (pkt->payload[2] << 16) | (pkt->payload[1] << 8) | pkt->payload[0];
      uint32_t temp_raw =
          (pkt->payload[5] << 16) | (pkt->payload[4] << 8) | pkt->payload[3];

      m_productor(processRawData(presion_raw, temp_raw));

      break;
    }
  }

  void onRespuestaRecibida(std::unique_ptr<const Core::I2CPacket> pkt) {
    onRespRecibStateMachin(std::move(pkt));
    m_esperandoRespuesta = false;
  }

  void unpackCalib(std::vector<uint8_t> payload) {
    calib.T1 = (float)((uint16_t)((payload[1] << 8) | payload[0])) * 256.0f;
    calib.T2 =
        (float)((uint16_t)((payload[3] << 8) | payload[2])) / 1073741824.0f;
    calib.T3 = (float)((int8_t)payload[4]) / 281474976710656.0f;
    calib.P1 =
        (float)((int16_t)((payload[6] << 8) | payload[5]) - 16384) / 1048576.0f;
    calib.P2 = (float)((int16_t)((payload[8] << 8) | payload[7]) - 16384) /
               536870912.0f;
    calib.P3 = (float)((int8_t)payload[9]) / 4294967296.0f;
    calib.P4 = (float)((int8_t)payload[10]) / 137438953472.0f;
    calib.P5 = (float)((uint16_t)((payload[12] << 8) | payload[11])) * 8.0f;
    calib.P6 = (float)((uint16_t)((payload[14] << 8) | payload[13])) / 64.0f;
    calib.P7 = (float)((int8_t)payload[15]) / 256.0f;
    calib.P8 = (float)((int8_t)payload[16]) / 32768.0f;
    calib.P9 = (float)((int16_t)((payload[18] << 8) | payload[17])) /
               281474976710656.0f;
    calib.P10 = (float)((int8_t)payload[19]) / 281474976710656.0f;
    calib.P11 = (float)((int8_t)payload[20]) / 36893488147419103232.0f;
  }

  Core::BaroData processRawData(uint32_t presion_raw, uint32_t temp_raw) {
    Core::BaroData datos_baro;

    float presion_compensada = bmp390_compensar_presion(presion_raw, temp_raw);

    float altitud_m =
        44330.0 * (1.0 - pow(presion_compensada / PRESION_NIVEL_MAR, 0.1903));

    uint64_t now = Core::OSAL::OSALFactory::getSystemTimeMs();
    float dt = (now - tiempo_anterior) / 1000.0f;
    float velocidad_ms = (altitud_m - altitud_anterior) / dt;

    altitud_anterior = altitud_m;
    tiempo_anterior = now;

    datos_baro.altitude = altitud_m;
    datos_baro.vertical_vel = velocidad_ms;
    return datos_baro;
  }

  float bmp390_compensar_presion(uint32_t temp_raw, uint32_t presion_raw) {

    // --- 1. COMPENSACIÓN DE TEMPERATURA ---
    float pd1 = (float)temp_raw - calib.T1;
    float pd2 = pd1 * calib.T2;
    float temperatura = pd2 + (pd1 * pd1 * calib.T3);

    // --- 2. COMPENSACIÓN DE PRESIÓN ---
    float temp_2 = temperatura * temperatura;
    float temp_3 = temp_2 * temperatura;

    float raw_p_float = (float)presion_raw;
    float raw_p_2 = raw_p_float * raw_p_float;
    float raw_p_3 = raw_p_2 * raw_p_float;

    float offset_pd1 = calib.P6 * temperatura;
    float offset_pd2 = calib.P7 * temp_2;
    float offset_pd3 = calib.P8 * temp_3;
    float offset = calib.P5 + offset_pd1 + offset_pd2 + offset_pd3;

    float sens_pd1 = calib.P2 * temperatura;
    float sens_pd2 = calib.P3 * temp_2;
    float sens_pd3 = calib.P4 * temp_3;
    float sens = calib.P1 + sens_pd1 + sens_pd2 + sens_pd3;

    float presion_pd1 = sens * raw_p_float;
    float presion_pd2 = calib.P9 * sens * raw_p_2;
    float presion_pd3 = calib.P10 * raw_p_3;
    float presion_pd4 = calib.P11 * raw_p_3;

    float presion =
        offset + presion_pd1 + presion_pd2 + presion_pd3 + presion_pd4;

    return presion;
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
      transactStateMachin();
      m_esperandoRespuesta = true;
    } else {
      // Damos un step de error. luego reintentamos
      m_esperandoRespuesta = false;
    }
  }
};
} // namespace DataNodes
} // namespace DataNode
} // namespace AppLogic
} // namespace FlightProxy
