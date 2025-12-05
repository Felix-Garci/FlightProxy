#pragma once

#include "FlightProxy/AppLogic/DataNode/IDataNodeBase.h"
#include "FlightProxy/Core/Channel/IChannelT.h"
#include "FlightProxy/Core/FlightProxyTypes.h"

#include <memory>

namespace FlightProxy
{
    namespace AppLogic
    {
        namespace DataNode
        {
            namespace DataNodes
            {
                class Nodo_Recepcion_IMU : public IDataNodeBase, public std::enable_shared_from_this<Nodo_Recepcion_IMU>
                {
                private:
                    std::shared_ptr<Core::Channel::IChannelT<Core::MspPacket>> m_channelIMUData;
                    std::function<void(Core::IMUData)> m_productor;
                    bool m_esperandoRespuesta = false;

                    void onRespuestaRecibida(std::unique_ptr<const Core::MspPacket> pkt)
                    {
                        m_esperandoRespuesta = false;
                        Core::IMUData datos_imu;
                        // Asegurarse de que hay suficientes datos
                        // 18 ya que son 9 valores de 2 bytes cada uno
                        // aunque los ultimos 3 no los usamos
                        if (pkt->payload.size() >= 18)
                        {
                            datos_imu.accel_x = static_cast<int16_t>(pkt->payload[0] | (pkt->payload[1] << 8));
                            datos_imu.accel_y = static_cast<int16_t>(pkt->payload[2] | (pkt->payload[3] << 8));
                            datos_imu.accel_z = static_cast<int16_t>(pkt->payload[4] | (pkt->payload[5] << 8));

                            datos_imu.gyro_x = static_cast<int16_t>(pkt->payload[6] | (pkt->payload[7] << 8));
                            datos_imu.gyro_y = static_cast<int16_t>(pkt->payload[8] | (pkt->payload[9] << 8));
                            datos_imu.gyro_z = static_cast<int16_t>(pkt->payload[10] | (pkt->payload[11] << 8));

                            m_productor(datos_imu);
                        }
                    }

                    void onCanalCerrado()
                    {
                        m_channelIMUData.reset();
                    }

                public:
                    Nodo_Recepcion_IMU(std::shared_ptr<Core::Channel::IChannelT<Core::MspPacket>> virtualChannelIMUData,
                                       std::function<void(Core::IMUData)> productorIMUData)
                        : m_channelIMUData(virtualChannelIMUData), m_productor(productorIMUData)
                    {
                        m_channelIMUData->onPacket = [this](std::unique_ptr<const Core::MspPacket> pkt)
                        {
                            this->onRespuestaRecibida(std::move(pkt));
                        };

                        // También nos suscribimos al cierre
                        m_channelIMUData->onClose = [this]()
                        {
                            this->onCanalCerrado();
                        };
                    }
                    void transact() override
                    {
                        if (!m_esperandoRespuesta)
                        {
                            // Construir y enviar el paquete MSP para solicitar datos IMU
                            std::vector<uint8_t> payload;                                                                          // Vacío para solicitud de datos
                            auto paqueteSolicitud = std::make_unique<Core::MspPacket>('<', Core::Protocol::MSP_IMU_DATA, payload); // 105 es el comando MSP para IMU

                            m_channelIMUData->sendPacket(std::move(paqueteSolicitud));
                            m_esperandoRespuesta = true;
                        }
                        else
                        {
                            // Todavía estamos esperando una respuesta, no hacemos nada
                            // FP_LOG_W("Nodo_Recepcion_IMU", "Aún esperando respuesta de datos IMU, no se envía nueva solicitud.");
                            m_esperandoRespuesta = false; // Reiniciamos para evitar bloqueo permanente
                        }
                    }
                };
            } // namespace DataNodes
        } // namespace DataNode
    } // namespace AppLogic
} // namespace FlightProxy
