#pragma once
/*
interface IEncoderT<PacketT>{
    +~IEncoderT()
    +encode(packet:const PacketT&): std::vector<uint8_t>
}
*/

namespace FlightProxy
{
    namespace Protocol
    {
        template <typename PacketT>
        class IEncoderT
        {
        public:
            virtual ~IEncoderT() = default;

            virtual std::vector<uint8_t> encode(const PacketT &packet) = 0;
        };
    }
}