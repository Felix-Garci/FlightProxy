#pragma once
#include <cstdint>

namespace FlightProxy
{
    namespace Core
    {
        namespace OSAL
        {

            template <typename T>
            class IQueue
            {
            public:
                virtual ~IQueue() = default;

                /**
                 * @brief Envía un ítem a la cola.
                 * @param item El ítem a copiar en la cola.
                 * @param timeout_ms Tiempo de espera en milisegundos.
                 * @return true si se envió correctamente, false si hubo timeout.
                 */
                virtual bool send(const T &item, uint32_t timeout_ms) = 0;

                /**
                 * @brief Recibe un ítem de la cola.
                 * @param item Referencia donde se copiará el ítem recibido.
                 * @param timeout_ms Tiempo de espera en milisegundos.
                 * @return true si se recibió un ítem, false si hubo timeout.
                 */
                virtual bool receive(T &item, uint32_t timeout_ms) = 0;
            };

        } // namespace OSAL
    } // namespace Core
} // namespace FlightProxy