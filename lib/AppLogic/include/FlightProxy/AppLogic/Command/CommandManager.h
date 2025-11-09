#pragma once

#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/AppLogic/Command/ICommand.h"
#include "FlightProxy/Core/OSAL/OSALFactory.h"

#include <map>
#include <memory>

namespace FlightProxy
{
    namespace AppLogic
    {
        namespace Command
        {
            template <typename PacketT>
            class CommandManager : public std::enable_shared_from_this<CommandManager<PacketT>>
            {
            public:
                using SenderFunc = std::function<bool(uint32_t, const PacketT &)>;
                SenderFunc responsehandler;

                CommandManager(size_t queueSize = 5)
                {
                    packetQueue_ = Core::OSAL::Factory::createQueue<Core::PacketEnvelope<PacketT>>(queueSize);
                }

                ~CommandManager()
                {
                    stop();
                }

                // Método público para recibir paquetes desde CUALQUIER sitio (Agregador incluido)
                bool enqueuePacket(const Core::PacketEnvelope<PacketT> &packet)
                {
                    if (!packetQueue_)
                        return false;
                    return packetQueue_->send(packet, 0);
                }

                void registerCommand(std::shared_ptr<ICommand<PacketT>> command)
                {
                    commandMap_[command->getID()] = command;
                }

                void start()
                {
                    if (isRunning_)
                        return;

                    // 2. Configurar y crear la tarea
                    Core::OSAL::TaskConfig config;
                    config.name = "CmdMgr";
                    config.stackSize = 4096;
                    config.priority = 2;

                    eventTask_ = Core::OSAL::Factory::createTask(
                        [this]()
                        { this->eventLoop(); }, // Lambda que llama al bucle
                        config);

                    if (eventTask_)
                    {
                        isRunning_ = true;
                        eventTask_->start();
                        FP_LOG_I("CommandManager", "Tarea iniciada");
                    }
                }

                void stop()
                {
                    if (isRunning_ && eventTask_)
                    {
                        // Señalizamos que pare (necesitarías un mecanismo mejor para salir del bucle de la cola,
                        // como enviar un paquete especial de "STOP" a la cola, o usar un timeout en receive).
                        isRunning_ = false;
                        // Por simplicidad aquí, forzamos stop si la implementación lo soporta
                        eventTask_->stop();
                    }
                }

            private:
                std::unique_ptr<Core::OSAL::IQueue<Core::PacketEnvelope<PacketT>>> packetQueue_;
                std::unique_ptr<Core::OSAL::ITask> eventTask_;
                std::atomic<bool> isRunning_{false};

                std::map<int, std::shared_ptr<ICommand<PacketT>>> commandMap_;

                static void eventTaskAdapter(void *arg)
                {
                    static_cast<CommandManager *>(arg)->eventTask();
                }

                void eventLoop()
                {
                    Core::PacketEnvelope<PacketT> packet;
                    // Mientras deba correr, esperamos paquetes
                    // Usamos un timeout razonable (ej. 1s) para poder comprobar isRunning_ periódicamente
                    while (isRunning_)
                    {
                        if (packetQueue_->receive(packet, 1000))
                        {
                            FP_LOG_I("CommandManager", "Recibido comando %d", packet.packet.command);
                            processContext(packet);
                        }
                    }
                    FP_LOG_I("CommandManager", "Tarea finalizada");
                }

                void processContext(const Core::PacketEnvelope<PacketT> &ctx)
                {
                    auto it = commandMap_.find(ctx.packet.command);

                    if (it != commandMap_.end())
                    {
                        // creamos la lambda de respuesta "al vuelo"
                        // Capturamos el 'sender_' y el 'channelId' específico de este paquete.
                        ReplyFunc<PacketT> replyCallback = [this, ctx](const PacketT &response)
                        {
                            if (this->responsehandler)
                            {
                                FP_LOG_W("skdhfj", "usando callbak de respuesta");
                                this->responsehandler(ctx.channelId, response);
                            }
                        };

                        FP_LOG_W("skdhfj", "comando encontrado, ejecutamos comando");

                        // Ejecutamos el comando pasándole la forma fácil de responder
                        it->second->execute(ctx.packet, replyCallback);
                    }
                    else
                    {
                        FP_LOG_W("skdhfj", "comando no encontrado recivimos msg de %d", ctx.packet.command);
                    }
                }
            };
        }
    }
}