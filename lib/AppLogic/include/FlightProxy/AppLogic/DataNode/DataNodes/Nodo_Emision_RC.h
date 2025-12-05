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
                class Nodo_Emision_RC : public IDataNodeBase, public std::enable_shared_from_this<Nodo_Emision_RC>
                {
                private:
                    std::shared_ptr<Core::Channel::IChannelT<Core::MspPacket>> m_channelRCData;
                    std::function<Core::RCData()> m_consumidor;
                    bool m_esperandoRespuesta = false;

                    void onRespuestaRecibida(std::unique_ptr<const Core::MspPacket> pkt)
                    {
                        m_esperandoRespuesta = false;
                        // No necesitamos procesar la respuesta para este nodo
                    }
                    void onCanalCerrado()
                    {
                        m_channelRCData.reset();
                    }

                public:
                    Nodo_Emision_RC(std::shared_ptr<Core::Channel::IChannelT<Core::MspPacket>> virtualChannelRCData,
                                    std::function<Core::RCData()> consumidorRCData)
                        : m_channelRCData(virtualChannelRCData), m_consumidor(consumidorRCData)
                    {
                        m_channelRCData->onPacket = [this](std::unique_ptr<const Core::MspPacket> pkt)
                        {
                            this->onRespuestaRecibida(std::move(pkt));
                        };

                        // También nos suscribimos al cierre del canal
                        m_channelRCData->onClose = [this]()
                        {
                            this->onCanalCerrado();
                        };
                    }

                    virtual void transact() override
                    {
                        if (m_channelRCData && !m_esperandoRespuesta)
                        {
                            Core::RCData rcData = m_consumidor();

                            std::vector<uint8_t> payload;
                            payload.resize(12); // 6 canales x 2 bytes cada uno

                            payload[0] = rcData.roll & 0xFF;
                            payload[1] = (rcData.roll >> 8) & 0xFF;
                            payload[2] = rcData.pitch & 0xFF;
                            payload[3] = (rcData.pitch >> 8) & 0xFF;
                            payload[4] = rcData.throttle & 0xFF;
                            payload[5] = (rcData.throttle >> 8) & 0xFF;
                            payload[6] = rcData.yaw & 0xFF;
                            payload[7] = (rcData.yaw >> 8) & 0xFF;
                            payload[8] = rcData.aux1 & 0xFF;
                            payload[9] = (rcData.aux1 >> 8) & 0xFF;
                            payload[10] = rcData.aux2 & 0xFF;
                            payload[11] = (rcData.aux2 >> 8) & 0xFF;

                            auto packet = std::make_unique<const Core::MspPacket>('<', Core::Protocol::MSP_RC_DATA, std::move(payload));

                            m_channelRCData->sendPacket(std::move(packet));
                            m_esperandoRespuesta = true;
                        }
                    }
                };
            } // namespace DataNodes
        } // namespace DataNode
    } // namespace AppLogic
} // namespace FlightProxy