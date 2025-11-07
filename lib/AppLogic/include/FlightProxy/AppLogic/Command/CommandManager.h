#pragma once

#include "FlightProxy/Core/FlightProxyTypes.h"
#include "FlightProxy/AppLogic/Command/ICommand.h"
#include "FlightProxy/Core/Utils/MutexGuard.h"

#include "freertos/task.h"

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
                CommandManager(size_t queueSize = 5) : running_(false)
                {
                    packetQueue_ = xQueueCreate(queueSize, sizeof(Core::PacketEnvelope<PacketT>));
                }

                ~CommandManager()
                {
                    running_ = false;
                    vQueueDelete(packetQueue_);
                    // Recuerda limpiar las tareas adecuadamente en un sistema real
                }

                using SenderFunc = std::function<bool(uint32_t, const PacketT &)>;
                SenderFunc responsehandler;

                // Método público para recibir paquetes desde CUALQUIER sitio (Agregador incluido)
                bool enqueuePacket(const Core::PacketEnvelope<PacketT> &packet)
                {
                    // Usamos wait time 0 para no bloquear a quien envía si la cola está llena
                    return (xQueueSendToBack(packetQueue_, &packet, (TickType_t)0) == pdPASS);
                }

                void registerCommand(std::shared_ptr<ICommand<PacketT>> command)
                {
                    commandMap_[command->getID()] = command;
                }

                void start()
                {
                    if (!running_)
                    {
                        running_ = true;
                        xTaskCreate(eventTaskAdapter, "CmdMgr", 4096, this, 2, &eventTaskHandle_);
                    }
                }

            private:
                QueueHandle_t packetQueue_;
                TaskHandle_t eventTaskHandle_;
                bool running_;

                std::map<int, std::shared_ptr<ICommand<PacketT>>> commandMap_;

                static void eventTaskAdapter(void *arg)
                {
                    static_cast<CommandManager *>(arg)->eventTask();
                }

                void eventTask()
                {
                    Core::PacketEnvelope<PacketT> packet;
                    while (running_)
                    {
                        // Espera indefinida hasta que llegue un paquete
                        if (xQueueReceive(packetQueue_, &packet, portMAX_DELAY) == pdPASS)
                        {
                            FP_LOG_W("CommandManager", "Command manager recived paket %d", packet.packet.command);
                            processContext(packet);
                        }
                    }
                    vTaskDelete(NULL);
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