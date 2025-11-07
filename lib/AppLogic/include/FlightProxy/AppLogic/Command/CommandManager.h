#pragma once

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
            class CommandManager : public std::enable_shared_from_this<CommandManager>
            {
            public:
                CommandManager(size_t queueSize = 5) : running_(false)
                {
                    packetQueue_ = xQueueCreate(queueSize, sizeof(PacketT));
                }
                ~CommandManager()
                {
                    running_ = false;
                    vQueueDelete(packetQueue_);
                    // Recuerda limpiar las tareas adecuadamente en un sistema real
                }

                // Método público para recibir paquetes desde CUALQUIER sitio (Agregador incluido)
                bool enqueuePacket(const PacketT &packet)
                {
                    // Usamos wait time 0 para no bloquear a quien envía si la cola está llena
                    return (xQueueSendToBack(packetQueue_, &packet, (TickType_t)0) == pdPASS);
                }

                void registerCommand(std::shared_ptr<ICommand<PacketT>> command)
                {
                    commandMap_[command->getId()] = command;
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
                    PacketT packet;
                    while (running_)
                    {
                        // Espera indefinida hasta que llegue un paquete
                        if (xQueueReceive(packetQueue_, &packet, portMAX_DELAY) == pdPASS)
                        {
                            processPacket(packet);
                        }
                    }
                    vTaskDelete(NULL);
                }

                void processPacket(const PacketT &packet)
                {
                    // 1. Extraer ID del paquete (asumiendo que PacketT tiene este atributo)
                    int id = packet.command;

                    // 2. Buscar el comando correspondiente
                    auto it = commandMap_.find(id);
                    if (it != commandMap_.end())
                    {
                        it->second->execute(packet);
                    }
                    else
                    {
                        // FP_LOG_W("CMD", "Comando desconocido recibido: %d", (int)id);
                    }
                }
            };
        }
    }
}