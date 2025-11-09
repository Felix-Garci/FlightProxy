#include "FlightProxy/Core/Utils/Logger.h"

namespace FlightProxy
{
    namespace Core
    {
        namespace Utils
        {

            // Inicializa el puntero estático a nulo
            ILogger *Logger::loggerInstance_ = nullptr;

        } // namespace Utils
    } // namespace Core
} // namespace FlightProxy