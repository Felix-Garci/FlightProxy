#pragma once

namespace FlightProxy
{
    namespace Core
    {
        namespace OSAL
        {

            /**
             * @brief Interfaz para cualquier objeto que pueda ser ejecutado
             * en un bucle de tarea.
             */
            class IRunnable
            {
            public:
                virtual ~IRunnable() = default;

                /**
                 * @brief El bucle principal de la tarea.
                 * Este método será llamado desde la nueva tarea.
                 */
                virtual void run() = 0;
            };

        } // namespace OSAL
    } // namespace Core
} // namespace FlightProxy