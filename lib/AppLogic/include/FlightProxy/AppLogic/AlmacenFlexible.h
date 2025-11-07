#pragma once

#include "FlightProxy/Core/Utils/MutexGuard.h"

#include <any>        // Para std::any (el contenedor mágico)
#include <map>        // Para el almacén de "cajones"
#include <functional> // Para std::function (las "manijas")
#include <memory>     // Para std::shared_ptr (clave para la manija)
#include <typeindex>  // Para typeid (el chequeo de tipo)
#include <stdexcept>  // Para los errores de arranque

namespace FlightProxy
{
    namespace AppLogic
    {

        /**
         * @brief Define el tipo de "dirección" o "ID" para cada dato.
         * Puedes usar 'int' o un 'enum' más seguro.
         */
        using DataID = int;

        /**
         * @class AlmacenFlexible
         * @brief Un almacén de datos (Blackboard) con seguridad de tipos
         * y bloqueo granular.
         *
         * Esta clase permite a los "Productores" (sensores) y "Consumidores"
         * (controles) registrarse en tiempo de arranque.
         * * Valida que el tipo de dato para un ID sea consistente y devuelve
         * "manijas" (funciones) que son seguras para usar entre hilos.
         */
        class AlmacenFlexible
        {

        private:
            /**
             * @struct DataSlot
             * @brief Estructura interna que representa un "cajón" de datos.
             * Contiene el dato, su mutex y su información de tipo.
             */
            struct DataSlot
            {
                std::any data;               // El dato en sí
                SemaphoreHandle_t slotMutex; // El mutex granular (¡uno por dato!)
                std::type_index typeInfo;    // El tipo de dato (para validación)

                // Constructor: inicializa con el tipo
                DataSlot(std::type_index info, std::any defaultVal)
                    : typeInfo(info), data(std::move(defaultVal)),
                      slotMutex(xSemaphoreCreateMutex()) {}
                ~DataSlot()
                {
                    vSemaphoreDelete(slotMutex);
                }
            };

            /**
             * @brief El almacén principal.
             * Mapea un ID a un puntero compartido a su "cajón" de datos.
             * Usamos shared_ptr para que las "manijas" (funciones lambda)
             * puedan compartir la propiedad del cajón y su mutex.
             */
            std::map<DataID, std::shared_ptr<DataSlot>> m_storage;

            /**
             * @brief Mutex global.
             * Protege SOLAMENTE el mapa m_storage durante el registro.
             * NO se usa durante las lecturas/escrituras normales.
             */
            SemaphoreHandle_t m_mapMutex;

            /**
             * @brief Función de ayuda interna para obtener o crear un cajón.
             * Esta es la lógica central de validación.
             */
            template <typename T>
            std::shared_ptr<DataSlot> getOrCreateSlot(DataID id)
            {
                Core::Utils::MutexGuard lock(m_mapMutex);

                auto it = m_storage.find(id);
                std::type_index requestedType = std::type_index(typeid(T));

                if (it != m_storage.end())
                {
                    // --- EL CAJÓN YA EXISTE ---
                    std::shared_ptr<DataSlot> slot = it->second;

                    // "se comprueva si esa posicion esta ya esta creada para comparar el typo"
                    if (slot->typeInfo != requestedType)
                    {
                        // ¡ERROR DE ARRANQUE!
                        // Un productor y un consumidor no coinciden.
                        throw std::runtime_error("Error de tipo en DataID: " + std::to_string(id));
                    }
                    // El tipo coincide, devuelve el cajón existente
                    return slot;
                }
                else
                {
                    // --- EL CAJÓN NO EXISTE ---
                    // "le dice direccion X, mete un TIPO"
                    // Creamos un nuevo cajón
                    auto newSlot = std::make_shared<DataSlot>(requestedType, T()); // T() es el valor por defecto

                    // Lo guardamos en el mapa
                    m_storage[id] = newSlot;

                    // Devolvemos el nuevo cajón
                    return newSlot;
                }
            }

        public:
            AlmacenFlexible() : m_mapMutex(xSemaphoreCreateMutex()) {}
            ~AlmacenFlexible()
            {
                vSemaphoreDelete(m_mapMutex);
            }

            /**
             * @brief Lo llama un SENSOR (Productor) para registrarse.
             * @tparam T El tipo de dato que este sensor producirá (ej. GyroData).
             * @param id El ID único (dirección) para este dato (ej. ID_GYRO).
             * @return Una función "setter" que el sensor usará para cargar datos.
             */
            template <typename T>
            std::function<void(T)> registrarProductor(DataID id)
            {

                // Obtiene o crea el cajón validando el tipo
                std::shared_ptr<DataSlot> slot = getOrCreateSlot<T>(id);

                // "el almacen le debuelbe una funcion protegida"
                // Creamos la "manija" (lambda) y capturamos el puntero al cajón
                auto setter = [slot](T newData)
                {
                    // "con el mutex de esa posicion"
                    Core::Utils::MutexGuard lock(slot->slotMutex);
                    slot->data = std::move(newData);
                };

                return setter;
            }

            /**
             * @brief Lo llama un CONTROL (Consumidor) para registrarse.
             * @tparam T El tipo de dato que este control espera (ej. GyroData).
             * @param id El ID único (dirección) de donde leerá (ej. ID_GYRO).
             * @return Una función "getter" que el control usará para leer datos.
             */
            template <typename T>
            std::function<T(void)> registrarConsumidor(DataID id)
            {

                // Obtiene o crea el cajón validando el tipo
                std::shared_ptr<DataSlot> slot = getOrCreateSlot<T>(id);

                // "pedirle una funcion para recivir dato... con su mutex de posicion"
                // Creamos la "manija" (lambda)
                auto getter = [slot]() -> T
                {
                    // "con el mutex de esa posicion"
                    Core::Utils::MutexGuard lock(slot->slotMutex);

                    // Usamos any_cast. Es seguro porque ya lo validamos en getOrCreateSlot
                    return std::any_cast<T>(slot->data);
                };

                return getter;
            }
        };
    }
}
