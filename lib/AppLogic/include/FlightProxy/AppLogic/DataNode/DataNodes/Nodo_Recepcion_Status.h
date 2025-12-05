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
                class Nodo_Recepcion_Status : public IDataNodeBase, public std::enable_shared_from_this<Nodo_Recepcion_Status>
                {
                private:
                    std::shared_ptr<Core::Channel::IChannelT<Core::MspPacket>> m_channelStatusData;
                    std::function<void(Core::StatusData)> m_productor;
                    bool m_esperandoRespuesta = false;

                    void onRespuestaRecibida(std::unique_ptr<const Core::MspPacket> pkt)
                    {
                        m_esperandoRespuesta = false;
                        Core::StatusData datos_status;
                        // Asegurarse de que hay suficientes datos
                        // 18 ya que son 9 valores de 2 bytes cada uno
                        // aunque los ultimos 3 no los usamos
                        if (pkt->payload.size() >= 16)
                        {
                            datos_status.cycleTime = static_cast<uint16_t>(pkt->payload[0] | (pkt->payload[1] << 8));
                            datos_status.i2c_errors = static_cast<uint16_t>(pkt->payload[2] | (pkt->payload[3] << 8));
                            datos_status.sensors = static_cast<uint16_t>(pkt->payload[4] | (pkt->payload[5] << 8));
                            datos_status.boxModeFlags = static_cast<uint32_t>(pkt->payload[6] | (pkt->payload[7] << 8) | (pkt->payload[8] << 16) | (pkt->payload[9] << 24));
                            datos_status.currentProfileIndex = pkt->payload[10];
                            datos_status.averageSystemLoadPercent = static_cast<uint16_t>(pkt->payload[11] | (pkt->payload[12] << 8));
                            datos_status.armingFlags = static_cast<uint16_t>(pkt->payload[13] | (pkt->payload[14] << 8));
                            datos_status.accCalibrationAxisFlags = pkt->payload[15];

                            m_productor(datos_status);
                        }
                    }

                    void onCanalCerrado()
                    {
                        m_channelStatusData.reset();
                    }

                public:
                    Nodo_Recepcion_Status(std::shared_ptr<Core::Channel::IChannelT<Core::MspPacket>> virtualChannelStatusData,
                                          std::function<void(Core::StatusData)> productorStatusData)
                        : m_channelStatusData(virtualChannelStatusData), m_productor(productorStatusData)
                    {
                        m_channelStatusData->onPacket = [this](std::unique_ptr<const Core::MspPacket> pkt)
                        {
                            this->onRespuestaRecibida(std::move(pkt));
                        };

                        // También nos suscribimos al cierre
                        m_channelStatusData->onClose = [this]()
                        {
                            this->onCanalCerrado();
                        };
                    }
                    void transact() override
                    {
                        if (!m_esperandoRespuesta)
                        {
                            // Construir y enviar el paquete MSP para solicitar datos IMU
                            std::vector<uint8_t> payload;                                                                             // Vacío para solicitud de datos
                            auto paqueteSolicitud = std::make_unique<Core::MspPacket>('<', Core::Protocol::MSP_STATUS_DATA, payload); // 105 es el comando MSP para Status

                            m_channelStatusData->sendPacket(std::move(paqueteSolicitud));
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
