#pragma once

#include "FlightProxy/Core/Transport/ITransport.h"
#include <asio.hpp>
#include <memory>
#include <string>
#include <deque>
#include <vector>

// Asegúrate de que los namespaces de FlightProxy estén disponibles
using FlightProxy::Core::Transport::ITransport;
namespace FlightProxy
{
    namespace Mocks
    {

        /**
         * @brief Implementación de ITransport usando Asio.
         *
         * Esta clase maneja una única conexión TCP, ya sea como cliente
         * o como un canal aceptado por un servidor.
         *
         * Utiliza operaciones asíncronas y un 'strand' para la sincronización.
         */
        class SimpleTCPAsio : public ITransport, public std::enable_shared_from_this<SimpleTCPAsio>
        {
        public:
            /**
             * @brief Constructor para el modo Servidor (cuando un socket ya ha sido aceptado).
             * @param ioc El io_context en el que se ejecutarán los handlers.
             * @param socket El socket TCP ya aceptado por el Listener.
             */
            SimpleTCPAsio(asio::io_context &ioc, asio::ip::tcp::socket socket);

            /**
             * @brief Constructor para el modo Cliente (crea un socket para conectar).
             * @param ioc El io_context en el que se ejecutarán los handlers.
             * @param ip La IP a la que conectarse.
             * @param port El puerto al que conectarse.
             */
            SimpleTCPAsio(asio::io_context &ioc, const std::string &ip, uint16_t port);

            virtual ~SimpleTCPAsio();

            // --- Implementación de la interfaz ITransport ---

            /**
             * @brief Inicia la conexión (si es cliente) y/o el bucle de lectura.
             */
            void open() override;

            /**
             * @brief Cierra la conexión de forma segura.
             */
            void close() override;

            /**
             * @brief Envía datos de forma asíncrona.
             * Es seguro llamar a esta función desde cualquier hilo.
             */
            void send(const uint8_t *data, size_t len) override;

        private:
            /**
             * @brief Inicia la conexión (para el modo cliente).
             */
            void do_connect();

            /**
             * @brief Inicia el bucle de lectura asíncrono.
             */
            void do_read();

            /**
             * @brief Inicia el bucle de escritura asíncrono (procesa la cola).
             */
            void do_write();

            /**
             * @brief Cierra el socket y notifica. (Debe llamarse desde el strand).
             */
            void do_close();

            // --- Handlers de Callbacks Asíncronos ---

            void on_connect(const asio::error_code &ec, const asio::ip::tcp::endpoint &endpoint);
            void on_read(const asio::error_code &ec, size_t bytes_transferred);
            void on_write(const asio::error_code &ec, size_t bytes_transferred);

        private:
            asio::ip::tcp::socket socket_;
            asio::ip::tcp::resolver resolver_;
            asio::strand<asio::io_context::executor_type> strand_;

            // Búfer de lectura
            std::array<uint8_t, 4096> read_buffer_;

            // Cola de escritura (para la seguridad de hilos en send())
            std::deque<std::vector<uint8_t>> write_queue_;
            bool is_writing_ = false;

            // Datos para el modo cliente
            std::string client_ip_;
            uint16_t client_port_;
            bool is_client_mode_ = false;
        };

    } // namespace Mocks
} // namespace FlightProxy