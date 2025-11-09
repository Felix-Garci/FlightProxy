#pragma once
#include "FlightProxy/Core/OSAL/OSALFactory.h" // Para el Mutex

#include <mutex>      // Para std::lock_guard
#include <map>        // Para el almacén de "cajones"
#include <functional> // Para std::function (las "manijas")
#include <memory>     // Para std::shared_ptr (clave para la manija)
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

                SlotBase() : slotMutex(Core::OSAL::Factory::createMutex()) {}
                virtual ~SlotBase() = default;

                // Método virtual para consultar el tipo real almacenado sin usar typeid
                virtual const void *getActualTypeID() const = 0;
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
