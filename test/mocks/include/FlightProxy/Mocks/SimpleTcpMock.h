#pragma once

#include "FlightProxy/Core/Transport/ITransport.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>

namespace FlightProxy
{
    namespace Mocks
    {
        class SimpleTcpMock : public Core::Transport::ITransport
        {
        public:
            /**
             * @brief Constructor que recibe la IP y el puerto para la conexión.
             * @param ip Dirección IP del servidor.
             * @param port Puerto del servidor.
             */
            SimpleTcpMock(const std::string &ip, int port);

            /**
             * @brief Destructor. Llama a close() para limpiar.
             */
            virtual ~SimpleTcpMock() override;

            // --- Implementación de la Interfaz ITransport ---

            /**
             * @brief Inicia la conexión con el servidor.
             * Lanza el hilo de recepción y llama a onOpen si tiene éxito.
             */
            void open() override;

            /**
             * @brief Cierra la conexión y detiene el hilo de recepción.
             * Llama a onClose.
             */
            void close() override;

            /**
             * @brief Envía datos al servidor. Es seguro llamarlo desde múltiples hilos.
             * @param data Puntero a los datos a enviar.
             * @param len Número de bytes a enviar.
             */
            void send(const uint8_t *data, size_t len) override;

        private:
            /**
             * @brief Bucle principal del hilo de recepción.
             * Espera datos (recv) y gestiona los eventos onData y onClose.
             */
            void receiveLoop();

            // --- Miembros Privados ---

            std::string _ip;
            int _port;
            SOCKET _socket;

            std::thread _receiveThread;
            std::thread::id _receiveThreadId; // Para evitar auto-joins

            std::mutex _sendMutex;  // Protege la función send()
            std::mutex _closeMutex; // Protege el proceso de cierre

            std::atomic<bool> _isConnected;
            std::atomic<bool> _shouldRun; // Controla el bucle del hilo

            /**
             * @brief Clase interna estática para inicializar y limpiar Winsock (RAII).
             */
            struct WinsockInitializer
            {
                WinsockInitializer();
                ~WinsockInitializer();
            };

            static WinsockInitializer _wsInitializer; // Instancia estática
        };
    }
}