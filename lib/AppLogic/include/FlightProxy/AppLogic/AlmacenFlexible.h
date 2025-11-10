#pragma once
#include "FlightProxy/Core/OSAL/OSALFactory.h" // Para el Mutex

#include <mutex>      // Para std::lock_guard
#include <map>        // Para el almacén de "cajones"
#include <functional> // Para std::function (las "manijas")
#include <memory>     // Para std::shared_ptr (clave para la manija)
#include <chrono>     // Para freshness
#include <string>
#include <stdexcept> // Para los errores de arranque

namespace FlightProxy
{
    namespace AppLogic
    {

        using DataID = int;

        /**
         * @brief Generador de ID de tipo en tiempo de compilación (sin RTTI).
         * Devuelve una dirección única para cada tipo T.
         */
        template <typename T>
        static const void *getTypeID()
        {
            static char id_dummy;
            return &id_dummy;
        }

        class AlmacenFlexible : public std::enable_shared_from_this<AlmacenFlexible>
        {

        private:
            /**
             * @struct SlotBase
             * @brief Interfaz base NO tipada.
             * Permite guardar todos los cajones en un mismo mapa std::map.
             * Contiene lo común: el mutex y la validación de tipo.
             */
            struct SlotBase
            {
                std::unique_ptr<Core::OSAL::IMutex> slotMutex;

                std::chrono::steady_clock::time_point last_update;
                double frequency_hz = 0.0;

                SlotBase() : slotMutex(Core::OSAL::Factory::createMutex()) {}
                virtual ~SlotBase() = default;

                // Método virtual para consultar el tipo real almacenado sin usar typeid
                virtual const void *getActualTypeID() const = 0;

                // Método auxiliar para actualizar estadísticas (debe llamarse bajo mutex)
                void updateStats()
                {
                    auto now = std::chrono::steady_clock::now();
                    // Si no es la primera actualización (time_since_epoch > 0)
                    if (last_update.time_since_epoch().count() > 0)
                    {
                        std::chrono::duration<double> elapsed = now - last_update;
                        // Evitamos división por cero si las actualizaciones son excesivamente rápidas
                        if (elapsed.count() > 1e-9)
                        {
                            // Frecuencia instantánea = 1 / periodo
                            // frequency_hz = 1.0 / elapsed.count();

                            // Paso vajo filtro exponencial para suavizar la frecuencia
                            frequency_hz = 0.9 * frequency_hz + 0.1 * (1.0 / elapsed.count());
                        }
                    }
                    last_update = now;
                }
            };

            /**
             * @struct TypedSlot
             * @brief Implementación tipada del cajón.
             * Hereda de SlotBase y añade el dato real de tipo T.
             */
            template <typename T>
            struct TypedSlot : SlotBase
            {
                T data;

                TypedSlot(T defaultVal) : data(std::move(defaultVal)) {}

                // Implementación que devuelve el ID único de T
                const void *getActualTypeID() const override
                {
                    return getTypeID<T>();
                }
            };

            // Mapa de IDs a punteros BASE (polimorfismo)
            std::map<DataID, std::shared_ptr<SlotBase>> m_storage;
            std::unique_ptr<Core::OSAL::IMutex> m_mapMutex;

            /**
             * @brief Obtiene o crea un cajón tipado de forma segura.
             */
            template <typename T>
            std::shared_ptr<TypedSlot<T>> getOrCreateSlot(DataID id)
            {
                std::lock_guard<Core::OSAL::IMutex> lock(*m_mapMutex);

                auto it = m_storage.find(id);
                if (it != m_storage.end())
                {
                    // --- EL CAJÓN YA EXISTE ---
                    // Validamos que el tipo solicitado (T) coincida con el tipo creado originalmente.
                    if (it->second->getActualTypeID() != getTypeID<T>())
                    {
                        throw std::runtime_error("Error de tipo (No-RTTI) en DataID: " + std::to_string(id));
                    }

                    // Static cast es seguro aquí porque acabamos de validar el ID del tipo.
                    return std::static_pointer_cast<TypedSlot<T>>(it->second);
                }
                else
                {
                    // --- EL CAJÓN NO EXISTE ---
                    auto newSlot = std::make_shared<TypedSlot<T>>(T());
                    m_storage[id] = newSlot;
                    return newSlot;
                }
            }

        public:
            AlmacenFlexible() : m_mapMutex(Core::OSAL::Factory::createMutex()) {}
            ~AlmacenFlexible() = default;

            // En la sección public de AlmacenFlexible:

            /**
             * @brief Obtiene la frecuencia estimada actual (en Hz) con decaimiento natural.
             * Si el productor se detiene, este valor bajará progresivamente hacia 0.
             */
            double getFrequency(DataID id)
            {
                std::lock_guard<Core::OSAL::IMutex> mapLock(*m_mapMutex);
                auto it = m_storage.find(id);
                if (it == m_storage.end())
                {
                    return 0.0;
                }

                std::lock_guard<Core::OSAL::IMutex> slotLock(*(it->second->slotMutex));

                // 1. Si nunca se ha actualizado o solo una vez (no hay periodo aún), devolvemos 0 o la inicial.
                if (it->second->frequency_hz <= 0.0 || it->second->last_update.time_since_epoch().count() == 0)
                {
                    return it->second->frequency_hz;
                }

                // 2. Calculamos tiempo transcurrido desde la última actualización real
                auto now = std::chrono::steady_clock::now();
                std::chrono::duration<double> elapsed = now - it->second->last_update;
                double elapsed_s = elapsed.count();

                // Evitamos divisiones por cero raras si se llama inmediatamente
                if (elapsed_s < 1e-9)
                {
                    return it->second->frequency_hz;
                }

                // 3. Calculamos la frecuencia "hipotética" si llegara un dato justo ahora
                double estimated_freq_now = 1.0 / elapsed_s;

                // 4. Devolvemos el mínimo.
                // Si estimated_freq_now es MAYOR que la real, significa que aún estamos
                // dentro del periodo esperado, así que mantenemos la frecuencia real.
                // Si es MENOR, significa que estamos "llegando tarde" y la frecuencia efectiva está bajando.
                return std::min(it->second->frequency_hz, estimated_freq_now);
            }

            template <typename T>
            std::function<void(T)> registrarProductor(DataID id)
            {

                // Obtiene o crea el cajón validando el tipo
                std::shared_ptr<TypedSlot<T>> slot = getOrCreateSlot<T>(id);

                return [slot](T newData)
                {
                    // Bloqueo granular usando el mutex del slot.
                    std::lock_guard<Core::OSAL::IMutex> lock(*(slot->slotMutex));
                    slot->data = std::move(newData);
                    slot->updateStats();
                };
            }

            template <typename T>
            std::function<T(void)> registrarConsumidor(DataID id)
            {
                std::shared_ptr<TypedSlot<T>> slot = getOrCreateSlot<T>(id);

                return [slot]() -> T
                {
                    std::lock_guard<Core::OSAL::IMutex> lock(*(slot->slotMutex));
                    return slot->data;
                };
            }
        };
    }
}
