#pragma once

#include "FlightProxy/Core/Transport/ITcpListener.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace FlightProxy
{
    namespace Transport
    {
        /**
         * @brief Implementación de ITcpListener para ESP-IDF (lwIP + FreeRTOS).
         *
         * Esta clase crea un socket de servidor y ejecuta una tarea de FreeRTOS
         * dedicada a aceptar nuevas conexiones de clientes.
         */
        class ListenerTCP : public Core::Transport::ITcpListener
        {
        public:
            ListenerTCP();
            virtual ~ListenerTCP() override;

            // --- Implementación de la interfaz ITcpListener ---

            bool startListening(uint16_t port) override;
            void stopListening() override;

        private:
            /**
             * @brief Función de entrada (estática) para la tarea de FreeRTOS
             * que ejecuta el bucle "accept".
             */
            static void listener_task_entry(void *arg);

            /**
             * @brief Handle de la tarea del listener.
             */
            TaskHandle_t m_listener_task_handle = NULL;

            /**
             * @brief File descriptor del socket del servidor (el que escucha).
             */
            int m_server_sock = -1;
        };

    } // namespace Transport
} // namespace FlightProxy