#pragma once

#include "ITransport.h"
#include <memory>
#include <functional>
#include <cstdint>

namespace FlightProxy
{
    namespace Core
    {
        namespace Transport
        {
            /**
             * @brief Interfaz para una "fábrica" de transportes del lado del servidor.
             *
             * Define un objeto que puede escuchar en un puerto TCP y notificar
             * cuando se aceptan nuevas conexiones de clientes.
             */
            class ITcpListener
            {
            public:
                virtual ~ITcpListener() = default;

                /**
                 * @brief Comienza a escuchar en un puerto específico.
                 * @param port El puerto TCP en el que escuchar.
                 * @return true si la escucha se inició con éxito, false en caso contrario.
                 */
                virtual bool startListening(uint16_t port) = 0;

                /**
                 * @brief Deja de escuchar y cierra el socket del servidor.
                 */
                virtual void stopListening() = 0;

                /**
                 * @brief Callback que se dispara cuando un nuevo cliente es aceptado.
                 * El manager debe suscribirse a este evento.
                 * El ITransport entregado está listo para usarse (pero 'open()'
                 * debe ser llamado para iniciar su tarea de lectura).
                 */
                std::function<void(std::shared_ptr<ITransport>)> onChannelAccepted;
            };

        } // namespace Transport
    } // namespace Core
} // namespace FlightProxy