import socket
import threading


class Connection:
    def __init__(self, ip, port):
        self.target_ip = ip
        self.target_port = port
        self.on_disconnect = []

        self.client = None
        self.connected = False
        self.timeout = 3.0

        self._lock = threading.Lock()

    def connect(self):
        """Intenta conectar y actualiza el estado."""
        try:
            # Si ya había un socket viejo, lo cerramos
            self.close()

            self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client.settimeout(self.timeout)
            self.client.connect((self.target_ip, self.target_port))

            self.connected = True
            return True

        except Exception as e:
            print(f"Error en connection connect: {e}")
            self.connected = False
            return False

    def add_callback_ondisconect(self, callback):
        self.on_disconnect.append(callback)

    def handle_failure(self):
        self.connected = False
        if self.client:
            self.client.close()
        self.client = None

        for cbk in self.on_disconnect:
            cbk()

    def close(self):
        """Cierra la conexión limpiamente."""
        self.connected = False
        if self.client:
            self.client.close()
            self.client = None

    def transact(self, payload):
        """
        Envía un comando y espera respuesta.
        Si falla, avisa al Frontend inmediatamente.
        """
        with self._lock:
            if not self.connected:
                return None

            try:
                self.client.sendall(payload)
                response = self.client.recv(4096)

                if not response:
                    raise ConnectionError("Servidor desconectado")

                return response

            except (socket.error, ConnectionError):
                self.handle_failure()
                return None

    def __del__(self):
        self.close()
