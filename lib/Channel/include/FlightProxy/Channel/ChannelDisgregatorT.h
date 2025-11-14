#pragma once

#include "FlightProxy/Core/Channel/IChannelT.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"
#include "FlightProxy/Core/Utils/Logger.h"

#include <functional> // Para std::function
#include <memory>     // Para std::unique_ptr, shared_ptr, weak_ptr, enable_shared_from_this
#include <map>        // Para la tabla de enrutamiento
#include <vector>     // Para la lista de suscriptores por comando
#include <algorithm>  // Para std::remove_if
#include <mutex>      // Para std::lock_guard

namespace FlightProxy
{
    namespace Channel
    {
        // Pre-declaración de la clase interna del canal virtual
        template <typename PacketT>
        class VirtualChannelT;

        // Pre-declaración de la fábrica
        template <typename PacketT>
        class ChannelDisgregatorT;

        // Definimos un tipo genérico para el ID del comando.
        using CommandId = uint16_t;

        template <typename PacketT>
        class VirtualChannelT : public Core::Channel::IChannelT<PacketT>,
                                public std::enable_shared_from_this<VirtualChannelT<PacketT>>
        {
        private:
            // Puntero débil a la fábrica que nos creó.
            std::weak_ptr<ChannelDisgregatorT<PacketT>> m_factory;
            CommandId m_commandId; // El ID de respuesta que escucha

        public:
            VirtualChannelT(std::weak_ptr<ChannelDisgregatorT<PacketT>> factory, CommandId commandId)
                : m_factory(factory), m_commandId(commandId)
            {
                FP_LOG_D("VirtualChannelT", "Canal virtual (shared) creado para el comando %u", m_commandId);
            }

            ~VirtualChannelT() override
            {
                FP_LOG_D("VirtualChannelT", "Destruyendo canal virtual (shared) para el comando %u", m_commandId);
            }

            // --- Implementación de IChannelT ---

            void open() override { /* No-op */ }
            void close() override { /* No-op */ }

            void sendPacket(std::unique_ptr<const PacketT> packet) override
            {
                // Intenta obtener un shared_ptr a partir del weak_ptr
                if (auto factory_sptr = m_factory.lock())
                {
                    // Si la fábrica aún existe, le enviamos el paquete
                    factory_sptr->sendPacketFromVirtual(std::move(packet));
                }
                else
                {
                    FP_LOG_W("VirtualChannelT", "Intento de envío en canal virtual huérfano (cmd %u)", m_commandId);
                }
            }

            void dispatch(const PacketT &pkt)
            {
                if (this->onPacket)
                {
                    auto pkt_ptr = std::make_unique<const PacketT>(pkt);
                    this->onPacket(std::move(pkt_ptr));
                }
            }

            void dispatchClose()
            {
                if (this->onClose)
                {
                    this->onClose();
                }
            }
        };

        template <typename PacketT>
        class ChannelDisgregatorT : public std::enable_shared_from_this<ChannelDisgregatorT<PacketT>>
        {
        public:
            using CommandExtractor = std::function<CommandId(const PacketT &)>;

        private:
            std::shared_ptr<Core::Channel::IChannelT<PacketT>> m_realChannel;
            CommandExtractor m_extractor;

            // Tabla de enrutamiento: Key = CommandId, Value = Lista de punteros DÉBILES
            std::map<CommandId, std::vector<std::weak_ptr<VirtualChannelT<PacketT>>>> m_routingTable;

            std::unique_ptr<Core::OSAL::IMutex> m_routingMutex;

            std::atomic<bool> m_isClosed{true};

            void onRealPacketReceived(std::unique_ptr<const PacketT> pkt)
            {
                CommandId cmd = m_extractor(*pkt);
                bool needs_cleanup = false;

                // Lista local de suscriptores VIVOS para evitar
                // mantener el mutex bloqueado durante el dispatch.
                std::vector<std::shared_ptr<VirtualChannelT<PacketT>>> live_subscribers;

                {
                    // --- Sección Crítica ---
                    std::lock_guard<Core::OSAL::IMutex> lock(*m_routingMutex);

                    auto it = m_routingTable.find(cmd);
                    if (it != m_routingTable.end())
                    {
                        auto &subscribers = it->second; // Vector de weak_ptrs

                        // Iteramos sobre los weak_ptr e intentamos "activarlos" (lock)
                        for (const auto &weak_sub : subscribers)
                        {
                            if (auto shared_sub = weak_sub.lock())
                            {
                                // ¡Vivo! Añadir a la lista para despachar
                                live_subscribers.push_back(shared_sub);
                            }
                            else
                            {
                                // ¡Muerto! Marcar que necesitamos limpiar esta lista
                                needs_cleanup = true;
                            }
                        }

                        // Lógica de limpieza (si se marcó)
                        if (needs_cleanup)
                        {
                            // "erase-remove idiom" para weak_ptrs expirados
                            subscribers.erase(
                                std::remove_if(subscribers.begin(), subscribers.end(),
                                               [](const auto &p)
                                               { return p.expired(); }),
                                subscribers.end());

                            FP_LOG_D("DemuxFactory", "Limpieza de suscriptores muertos para cmd %u", cmd);

                            // Opcional: si el vector queda vacío, borramos la entrada
                            if (subscribers.empty())
                            {
                                m_routingTable.erase(it);
                            }
                        }
                    }
                    // --- Fin Sección Crítica ---
                }

                // 4. Despachar el paquete a la lista de VIVOS (fuera del mutex)
                for (const auto &vChannel : live_subscribers)
                {
                    vChannel->dispatch(*pkt);
                }
            }

        public:
            ChannelDisgregatorT(std::shared_ptr<Core::Channel::IChannelT<PacketT>> realChannel,
                                CommandExtractor extractor)
                : m_realChannel(realChannel),
                  m_extractor(extractor),
                  m_routingMutex(Core::OSAL::Factory::createMutex())
            {
                if (!m_realChannel || !m_extractor)
                {
                    FP_LOG_E("DemuxFactory", "¡El canal real o la función extractora no pueden ser nulos!");
                    throw std::runtime_error("ChannelDisgregatorT: Dependencias nulas");
                }

                m_realChannel->onPacket = [this](std::unique_ptr<const PacketT> pkt)
                {
                    this->onRealPacketReceived(std::move(pkt));
                };

                m_isClosed.store(false);

                m_realCHannel->onClose = [this]()
                {
                    FP_LOG_I("DemuxFactory", "Canal real cerrado.");
                    m_isClosed.store(true);

                    for (const auto &vChannel : all_live_subscribers)
                    {
                        vChannel->dispatchClose();
                    }
                };
            }

            ~ChannelDisgregatorT()
            {
                if (m_realChannel)
                {
                    m_realChannel->onPacket = nullptr;
                }
                FP_LOG_I("DemuxFactory", "Destruida.");
            }

            std::shared_ptr<Core::Channel::IChannelT<PacketT>>
            createVirtualChannel(CommandId responseIdToListenFor)
            {
                // 1. Crear el canal con make_shared
                //    Pasamos un weak_ptr de 'this' (fábrica) al constructor del canal.
                //    shared_from_this() viene de enable_shared_from_this.
                auto vChannel = std::make_shared<VirtualChannelT<PacketT>>(
                    std::weak_ptr<ChannelDisgregatorT<PacketT>>(this->shared_from_this()),
                    responseIdToListenFor);

                {
                    // --- Sección Crítica ---
                    std::lock_guard<Core::OSAL::IMutex> lock(*m_routingMutex);

                    // 2. Registrar un weak_ptr en nuestra tabla de enrutamiento
                    m_routingTable[responseIdToListenFor].push_back(
                        std::weak_ptr<VirtualChannelT<PacketT>>(vChannel));
                    // --- Fin Sección Crítica ---
                }

                // 3. Devolver el shared_ptr al llamador (casteado a la interfaz)
                return std::static_pointer_cast<Core::Channel::IChannelT<PacketT>>(vChannel);
            }

            void sendPacketFromVirtual(std::unique_ptr<const PacketT> packet)
            {
                // 1. Comprobamos la bandera atómica PRIMERO.
                if (m_isClosed)
                {
                    // La puerta está cerrada. Descartamos.
                    return;
                }
                m_realChannel->sendPacket(std::move(packet));
            }
        };

    } // namespace Channel
} // namespace FlightProxy