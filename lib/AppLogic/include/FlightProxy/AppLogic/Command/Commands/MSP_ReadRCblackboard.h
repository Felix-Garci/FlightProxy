#pragma once
#include "FlightProxy/AppLogic/Command/ICommand.h"
#include "FlightProxy/Core/Utils/Logger.h"

#include <functional>
#include <memory>
#include <vector>

#include "FlightProxy/Core/FlightProxyTypes.h"

namespace FlightProxy
{
    namespace AppLogic
    {
        namespace Command
        {
            namespace Commands
            {

                template <typename PacketT>
                class MSP_ReadRCblackboard : public ICommand<PacketT>,
                                             public std::enable_shared_from_this<MSP_ReadRCblackboard<PacketT>>
                {
                public:
                    MSP_ReadRCblackboard(std::function<FlightProxy::Core::IBUSPacket::ChannelsT(void)> rcReader) : rcReader_(rcReader) {}
                    int getID() override { return 105; }

                    void execute(std::unique_ptr<const PacketT> packet, ReplyFunc<PacketT> reply) override
                    {
                        FP_LOG_W("Read command", "Estamos a punto de responder dentro del comando");
                        // hacer cosas con la lecutra de consumer
                        auto rcValues = rcReader_();
                        // montar un packet
                        auto replyPacket = std::make_unique<const PacketT>('>', 1, std::vector<uint8_t>{
                                                                                       static_cast<uint8_t>(rcValues[0] & 0xFF),
                                                                                       static_cast<uint8_t>((rcValues[0] >> 8) & 0xFF),
                                                                                       static_cast<uint8_t>(rcValues[1] & 0xFF),
                                                                                       static_cast<uint8_t>((rcValues[1] >> 8) & 0xFF),
                                                                                       static_cast<uint8_t>(rcValues[2] & 0xFF),
                                                                                       static_cast<uint8_t>((rcValues[2] >> 8) & 0xFF),
                                                                                       static_cast<uint8_t>(rcValues[3] & 0xFF),
                                                                                       static_cast<uint8_t>((rcValues[3] >> 8) & 0xFF),
                                                                                       static_cast<uint8_t>(rcValues[4] & 0xFF),
                                                                                       static_cast<uint8_t>((rcValues[4] >> 8) & 0xFF),
                                                                                       static_cast<uint8_t>(rcValues[5] & 0xFF),
                                                                                       static_cast<uint8_t>((rcValues[5] >> 8) & 0xFF),
                                                                                       static_cast<uint8_t>(rcValues[6] & 0xFF),
                                                                                       static_cast<uint8_t>((rcValues[6] >> 8) & 0xFF),
                                                                                       static_cast<uint8_t>(rcValues[7] & 0xFF),
                                                                                       static_cast<uint8_t>((rcValues[7] >> 8) & 0xFF),
                                                                                   });

                        reply(std::move(replyPacket));
                    }

                private:
                    std::function<FlightProxy::Core::IBUSPacket::ChannelsT(void)> rcReader_;
                };
            }
        }
    }
}