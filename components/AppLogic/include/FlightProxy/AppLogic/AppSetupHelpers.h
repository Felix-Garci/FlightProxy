#pragma once

#include "FlightProxy/AppLogic/AlmacenFlexible.h"
#include "FlightProxy/AppLogic/Command/CommandManager.h"
#include "FlightProxy/AppLogic/DataNode/DataNodesManagerT.h"
#include "FlightProxy/Channel/ChannelDisgregatorT.h"

namespace FlightProxy {
namespace AppLogic {
namespace Setup {

template <typename CommandT, typename DataT, typename PacketT>
void registerProductorCommand(
    std::shared_ptr<Command::CommandManager<PacketT>> cmdMgr,
    std::shared_ptr<AlmacenFlexible> bb, uint32_t id) {
  auto productor = bb->registrarProductor<DataT>(id);
  auto cmd = std::make_shared<CommandT>(productor);
  cmdMgr->registerCommand(cmd);
}

template <typename CommandT, typename DataT, typename PacketT>
void registerConsumerCommand(
    std::shared_ptr<Command::CommandManager<PacketT>> cmdMgr,
    std::shared_ptr<AlmacenFlexible> bb, uint32_t id) {
  auto consumidor = bb->registrarConsumidor<DataT>(id);
  auto cmd = std::make_shared<CommandT>(consumidor);
  cmdMgr->registerCommand(cmd);
}

template <typename NodeT, typename DataT, typename PacketT>
void addReceptionNode(
    std::shared_ptr<DataNode::DataNodesManager> nodeMgr,
    std::shared_ptr<Channel::ChannelDisgregatorT<PacketT>> disgregator,
    uint32_t mspCommandId, std::shared_ptr<AlmacenFlexible> bb, uint32_t id,
    uint32_t periodMs) {
  auto vChannel = disgregator->createVirtualChannel(mspCommandId);
  auto productor = bb->registrarProductor<DataT>(id);
  auto nodo = std::make_shared<NodeT>(vChannel, productor);
  nodeMgr->addDataNode(nodo, periodMs);
}

template <typename NodeT, typename DataT, typename PacketT>
void addEmissionNode(
    std::shared_ptr<DataNode::DataNodesManager> nodeMgr,
    std::shared_ptr<Channel::ChannelDisgregatorT<PacketT>> disgregator,
    uint32_t mspCommandId, std::shared_ptr<AlmacenFlexible> bb, uint32_t id,
    uint32_t periodMs) {
  auto vChannel = disgregator->createVirtualChannel(mspCommandId);
  auto consumidor = bb->registrarConsumidor<DataT>(id);
  auto nodo = std::make_shared<NodeT>(vChannel, consumidor);
  nodeMgr->addDataNode(nodo, periodMs);
}

} // namespace Setup
} // namespace AppLogic
} // namespace FlightProxy
