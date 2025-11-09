#pragma once
#include <cstdint>
#include <memory>
#include <functional>

namespace FlightProxy
{
    namespace Core
    {
        namespace OSAL
        {

            class IMutex
            {
            public:
                virtual ~IMutex() = default;

                // --- Métodos de Instancia (lo que ya tenías) ---
                virtual void lock() = 0;
                virtual void unlock() = 0;
                virtual bool tryLock(uint32_t timeout_ms) = 0;

                // --- NUEVO: Parte Estática (Factoría integrada) ---
                using MutexFactory = std::function<std::unique_ptr<IMutex>()>;

                // 1. Método para registrar QUIÉN crea los mutex (se llama al inicio del programa)
                static void setFactory(MutexFactory factory)
                {
                    getFactoryInstance() = factory;
                }

                // 2. Método para crear un mutex (lo usan tus clases como ChannelAgregator)
                static std::unique_ptr<IMutex> create()
                {
                    auto &factory = getFactoryInstance();
                    if (factory)
                    {
                        return factory();
                    }
                    // Retornar nullptr o lanzar excepción si no se configuró
                    return nullptr;
                }

            private:
                // Singleton interno para guardar la factoría sin necesitar un .cpp separado
                static MutexFactory &getFactoryInstance()
                {
                    static MutexFactory factory;
                    return factory;
                }
            };

        }
    }
}