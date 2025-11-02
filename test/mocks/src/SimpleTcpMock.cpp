#include "FlightProxy/Mocks/SimpleTcpMock.h"

#include "FlightProxy/Core/Utils/Logger.h"
#include "FlightProxy/Mocks/HostLogger.h"

namespace FlightProxy
{
    namespace Mocks
    {
        static const char *TAG = "SimpleTcpMock";
        // --- Inicializador Estático de Winsock (RAII) ---

        // Inicializa Winsock cuando se carga el programa
        SimpleTcpMock::WinsockInitializer::WinsockInitializer()
        {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            {
                FP_LOG_I(TAG, "Fallo al inicializar WSAStartup.");
            }
        }

        // Limpia Winsock cuando se cierra el programa
        SimpleTcpMock::WinsockInitializer::~WinsockInitializer()
        {
            WSACleanup();
        }

        // Definición de la instancia estática
        SimpleTcpMock::WinsockInitializer SimpleTcpMock::_wsInitializer;

        // --- Implementación de SimpleTcpMock ---

        SimpleTcpMock::SimpleTcpMock(const std::string &ip, int port)
            : _ip(ip),
              _port(port),
              _socket(INVALID_SOCKET),
              _isConnected(false),
              _shouldRun(false)
        {
        }

        SimpleTcpMock::~SimpleTcpMock()
        {
            // Asegurarse de que todo esté cerrado al destruir
            close();
        }

        void SimpleTcpMock::open()
        {
            std::lock_guard<std::mutex> lock(_closeMutex);

            if (_isConnected)
            {
                // Ya está abierto
                return;
            }

            // 1. Crear el socket
            _socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (_socket == INVALID_SOCKET)
            {
                FP_LOG_I(TAG, ("Fallo al crear el socket: " + std::to_string(WSAGetLastError())).c_str());
            }

            // 2. Configurar la dirección del servidor (sockaddr_in)
            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(_port);

            // Convertir IP string a formato de red
            if (inet_pton(AF_INET, _ip.c_str(), &serverAddr.sin_addr) <= 0)
            {
                closesocket(_socket);
                _socket = INVALID_SOCKET;
                FP_LOG_I(TAG, ("Dirección IP inválida: " + _ip).c_str());
            }

            // 3. Conectar al servidor
            if (connect(_socket, (SOCKADDR *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
            {
                closesocket(_socket);
                _socket = INVALID_SOCKET;
                FP_LOG_I(TAG, ("Fallo al conectar: " + std::to_string(WSAGetLastError())).c_str());
            }

            // 4. Conexión exitosa. Actualizar estados e iniciar hilo.
            _isConnected = true;
            _shouldRun = true;

            _receiveThread = std::thread(&SimpleTcpMock::receiveLoop, this);
            _receiveThreadId = _receiveThread.get_id();

            // 5. Notificar al cliente (si proveyó un callback)
            if (onOpen)
            {
                onOpen();
            }
        }

        void SimpleTcpMock::close()
        {
            // Usamos _shouldRun como el guardián principal para evitar múltiples cierres.
            // exchange() establece atómicamente el valor a false y devuelve el valor *anterior*.
            // Si ya era false, significa que close() ya se está ejecutando o se completó.
            if (!_shouldRun.exchange(false))
            {
                return;
            }

            // Bloqueamos para asegurar que el proceso de cierre sea atómico
            std::lock_guard<std::mutex> lock(_closeMutex);

            _isConnected = false;

            // 1. Cerrar el socket. Esto hará que recv() en el otro hilo falle.
            if (_socket != INVALID_SOCKET)
            {
                shutdown(_socket, SD_BOTH); // Notifica al otro lado
                closesocket(_socket);
                _socket = INVALID_SOCKET;
            }

            // 2. Esperar a que el hilo de recepción termine.
            // No podemos hacer 'join' si el hilo que llama a close()
            // es el mismo hilo de recepción (lo que pasaría si recv() falla).
            if (_receiveThread.joinable())
            {
                if (std::this_thread::get_id() == _receiveThreadId)
                {
                    _receiveThread.detach(); // El hilo se cierra solo, debe soltarse
                }
                else
                {
                    _receiveThread.join(); // Otro hilo está cerrando, esperamos
                }
            }

            // 3. Notificar al cliente (si proveyó un callback)
            if (onClose)
            {
                onClose();
            }
        }

        void SimpleTcpMock::send(const uint8_t *data, size_t len)
        {
            if (!_isConnected || len == 0)
            {
                return;
            }

            // Prevenir envíos concurrentes o un envío durante un cierre
            std::lock_guard<std::mutex> lock(_sendMutex);

            // Nos aseguramos de que la conexión no se haya cerrado mientras esperábamos el mutex
            if (!_isConnected)
            {
                return;
            }

            size_t totalSent = 0;
            while (totalSent < len)
            {
                // Winsock usa 'char*', no 'uint8_t*'
                const char *bufferStart = reinterpret_cast<const char *>(data + totalSent);
                int bytesToSend = static_cast<int>(len - totalSent);

                int sent = ::send(_socket, bufferStart, bytesToSend, 0);

                if (sent == SOCKET_ERROR)
                {
                    // Ocurrió un error. La conexión probablemente esté muerta.
                    // Cerramos desde aquí.
                    this->close();
                    FP_LOG_I(TAG, ("Error en send(): " + std::to_string(WSAGetLastError())).c_str());
                }

                totalSent += sent;
            }
        }

        void SimpleTcpMock::receiveLoop()
        {
            // Buffer de recepción. 4KB es un tamaño común.
            std::vector<uint8_t> buffer(4096);

            while (_shouldRun)
            {
                // 1. Esperar a recibir datos (bloqueante)
                int bytesReceived = ::recv(
                    _socket,
                    reinterpret_cast<char *>(buffer.data()),
                    static_cast<int>(buffer.size()),
                    0);

                // 2. Analizar el resultado
                if (bytesReceived > 0)
                {
                    // Datos recibidos. Notificar al cliente.
                    if (onData && _shouldRun) // Comprobar _shouldRun de nuevo
                    {
                        onData(buffer.data(), static_cast<size_t>(bytesReceived));
                    }
                }
                else if (bytesReceived == 0)
                {
                    // Conexión cerrada limpiamente por el servidor
                    break; // Salir del bucle
                }
                else
                {
                    // SOCKET_ERROR
                    // Si _shouldRun sigue en true, fue un error inesperado.
                    // Si _shouldRun es false, fue porque close() llamó a shutdown().
                    if (_shouldRun)
                    {
                        // Error inesperado
                    }
                    break; // Salir del bucle
                }
            }

            // 3. El bucle ha terminado (por error, cierre remoto o cierre local).
            // Llamamos a close() para limpiar y disparar el callback onClose().
            // La lógica interna de close() evitará ejecuciones duplicadas.
            this->close();
        }

    }
}