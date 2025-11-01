#include "FlightProxy/Mocks/MockTcpTransport.h"
#include "FlightProxy/Mocks/HostLogger.h"

namespace FlightProxy
{
    namespace Mocks
    {
        static const char *TAG = "SimpleTCPAsio";

        // Constructor (Modo Servidor)
        SimpleTCPAsio::SimpleTCPAsio(asio::io_context &ioc, asio::ip::tcp::socket socket)
            : socket_(std::move(socket)),
              resolver_(ioc),
              strand_(ioc.get_executor()),
              is_client_mode_(false)
        {
            // FP_LOG_I(TAG, "Canal (servidor) creado.");
        }

        // Constructor (Modo Cliente)
        SimpleTCPAsio::SimpleTCPAsio(asio::io_context &ioc, const std::string &ip, uint16_t port)
            : socket_(ioc),
              resolver_(ioc),
              strand_(ioc.get_executor()),
              client_ip_(ip),
              client_port_(port),
              is_client_mode_(true)
        {
            // FP_LOG_I(TAG, "Canal (cliente) creado para %s:%u", ip.c_str(), port);
        }

        SimpleTCPAsio::~SimpleTCPAsio()
        {
            // FP_LOG_I(TAG, "Canal destruido.");
        }

        void SimpleTCPAsio::open()
        {
            auto self = shared_from_this();
            asio::post(strand_, [this, self]()
                       {
        if (is_client_mode_)
        {
            // Es un cliente, necesita conectarse primero
            do_connect();
        }
        else
        {
            // Es un servidor, el socket ya está conectado.
            // Notificamos onOpen y empezamos a leer.
            if (onOpen)
            {
                onOpen();
            }
            do_read();
        } });
        }

        void SimpleTCPAsio::close()
        {
            // Postea la operación de cierre al strand para evitar
            // competir con operaciones de lectura/escritura.
            auto self = shared_from_this();
            asio::post(strand_, [this, self]()
                       { do_close(); });
        }

        void SimpleTCPAsio::send(const uint8_t *data, size_t len)
        {
            if (data == nullptr || len == 0)
            {
                return;
            }

            // Copiamos los datos para que vivan hasta que se envíen
            std::vector<uint8_t> data_copy(data, data + len);

            auto self = shared_from_this();
            asio::post(strand_, [this, self, data_copy = std::move(data_copy)]() mutable
                       {
        bool was_empty = write_queue_.empty();
        write_queue_.push_back(std::move(data_copy));

        // Si la cola estaba vacía, no hay una operación de escritura
        // en curso, así que debemos iniciar una.
        if (!is_writing_)
        {
            do_write();
        } });
        }

        // --- Métodos Privados (ejecutados en el strand) ---

        void SimpleTCPAsio::do_connect()
        {
            // FP_LOG_I(TAG, "Modo cliente: Intentando conectar a %s:%u...", client_ip_.c_str(), client_port_);

            auto self = shared_from_this();
            resolver_.async_resolve(
                client_ip_,
                std::to_string(client_port_),
                [this, self](const asio::error_code &ec, asio::ip::tcp::resolver::results_type results)
                {
                    if (ec)
                    {
                        // FP_LOG_E(TAG, "Error en resolve: %s", ec.message().c_str());
                        do_close(); // Notificará onClose
                        return;
                    }

                    asio::async_connect(
                        socket_,
                        results,
                        // Usamos bind_executor para asegurar que on_connect se ejecute en el strand
                        asio::bind_executor(strand_,
                                            [this, self](const asio::error_code &ec, const asio::ip::tcp::endpoint &endpoint)
                                            {
                                                on_connect(ec, endpoint);
                                            }));
                });
        }

        void SimpleTCPAsio::on_connect(const asio::error_code &ec, const asio::ip::tcp::endpoint &endpoint)
        {
            if (ec)
            {
                // FP_LOG_E(TAG, "Error en connect: %s", ec.message().c_str());
                do_close(); // Notificará onClose
                return;
            }

            // FP_LOG_I(TAG, "Conectado con éxito!");

            // Conectados. Notificamos y empezamos a leer.
            if (onOpen)
            {
                onOpen();
            }
            do_read();
        }

        void SimpleTCPAsio::do_read()
        {
            auto self = shared_from_this();
            socket_.async_read_some(
                asio::buffer(read_buffer_),
                asio::bind_executor(strand_,
                                    [this, self](const asio::error_code &ec, size_t bytes_transferred)
                                    {
                                        on_read(ec, bytes_transferred);
                                    }));
        }

        void SimpleTCPAsio::on_read(const asio::error_code &ec, size_t bytes_transferred)
        {
            if (!ec)
            {
                // --- Datos recibidos ---
                if (onData)
                {
                    onData(read_buffer_.data(), bytes_transferred);
                }
                // Volvemos a leer
                do_read();
            }
            else
            {
                // --- Error o Desconexión ---
                if (ec == asio::error::eof || ec == asio::error::connection_reset)
                {
                    // FP_LOG_I(TAG, "Cliente desconectado (EOF).");
                }
                else if (ec != asio::error::operation_aborted)
                {
                    // FP_LOG_E(TAG, "Error en recv: %s", ec.message().c_str());
                }
                // En cualquier caso (excepto 'operation_aborted' que viene de un close() nuestro),
                // cerramos la conexión.
                do_close();
            }
        }

        void SimpleTCPAsio::do_write()
        {
            if (write_queue_.empty())
            {
                is_writing_ = false;
                return; // No hay nada que escribir
            }

            is_writing_ = true;
            auto self = shared_from_this();

            // Usamos async_write para enviar todo el buffer (write_queue_.front())
            asio::async_write(
                socket_,
                asio::buffer(write_queue_.front()),
                asio::bind_executor(strand_,
                                    [this, self](const asio::error_code &ec, size_t bytes_transferred)
                                    {
                                        on_write(ec, bytes_transferred);
                                    }));
        }

        void SimpleTCPAsio::on_write(const asio::error_code &ec, size_t bytes_transferred)
        {
            if (!ec)
            {
                // --- Escritura exitosa ---
                write_queue_.pop_front();
                // Continuamos escribiendo el resto de la cola
                do_write();
            }
            else
            {
                // --- Error de escritura ---
                if (ec != asio::error::operation_aborted)
                {
                    // FP_LOG_E(TAG, "Error en send: %s", ec.message().c_str());
                }
                do_close();
            }
        }

        void SimpleTCPAsio::do_close()
        {
            if (!socket_.is_open())
            {
                return; // Ya está cerrado
            }

            // FP_LOG_I(TAG, "Cerrando socket...");

            // Cierre seguro
            asio::error_code ec;
            socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
            socket_.close(ec);

            // Notificar al suscriptor
            if (onClose)
            {
                onClose();
            }
        }
    } // namespace Mocks
} // namespace FlightProxy