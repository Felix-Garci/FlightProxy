import socket
import struct
from gzproces import leer_presion_gazebo

# --- CONFIGURACIÓN ---
HOST = "0.0.0.0"
PORT = 5800

temperatura_real = 25.0  # °C
presion_real = 101001.0  # Pa


class dato:
    def __init__(self):
        self.temperatura_real = 25.0
        self.presion_real = 101001.0

    def get_tem(self):
        pass


def empaquetar_24bit(valor):
    """Convierte float a int24 para el protocolo I2C"""
    valor_int = int(valor)
    # Clamp para evitar overflow
    valor_int = max(0, min(16777215, valor_int))

    b0 = valor_int & 0xFF
    b1 = (valor_int >> 8) & 0xFF
    b2 = (valor_int >> 16) & 0xFF
    return bytes([b0, b1, b2])


def manejar_cliente(conn, addr):
    print(f"[+] Cliente conectado: {addr}")
    try:
        while True:
            header = conn.recv(4)
            if not header or len(header) < 4:
                break

            # Desempaquetar: Addr, Reg, IsRead(bool), Len
            dev_addr, reg_addr, is_read, payload_len = struct.unpack("BBBB", header)

            payload_respuesta = b""

            if is_read:
                # --- MODO LECTURA ---
                if reg_addr == 0x00:
                    payload_respuesta = b"\x60"

                elif reg_addr == 0x31:
                    print("  -> Enviando Calibración (Identidad T2=1, P1=1)")
                    payload_respuesta = bytearray(21)  # enviamos ceros

                elif reg_addr == 0x04:
                    raw_pres = empaquetar_24bit(leer_presion_gazebo())
                    raw_temp = empaquetar_24bit(temperatura_real)

                    payload_respuesta = raw_pres + raw_temp

                elif reg_addr == 0x1B:
                    # PWR_CTRL (Asumimos modo normal activo)
                    payload_respuesta = b"\x33"

                else:
                    payload_respuesta = bytes(payload_len)  # Ceros por defecto

                # Rellenar o cortar al tamaño exacto pedido
                if len(payload_respuesta) < payload_len:
                    payload_respuesta += bytes(payload_len - len(payload_respuesta))
                else:
                    payload_respuesta = payload_respuesta[:payload_len]

            else:
                # --- MODO ESCRITURA ---
                if payload_len > 0:
                    datos = conn.recv(payload_len)
                    if reg_addr == 0x1B and datos[0] == 0x33:
                        print("  <- Configuración recibida: MODO NORMAL activado")

                payload_respuesta = b""  # Escritura no devuelve payload

            # 2. Enviar RESPUESTA (Cabecera + Payload)
            # Replicamos la cabecera original, ajustando el len real
            resp_header = struct.pack(
                "BBBB", dev_addr, reg_addr, is_read, len(payload_respuesta)
            )
            conn.sendall(resp_header + payload_respuesta)

    except Exception as e:
        print(f"[-] Error o Desconexión: {e}")
    finally:
        conn.close()


def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server.bind((HOST, PORT))
        server.listen(1)
        print(f"Escuchando en {HOST}:{PORT}")

        while True:
            conn, addr = server.accept()
            manejar_cliente(conn, addr)
    except KeyboardInterrupt:
        print("\nApagando...")
    finally:
        server.close()


if __name__ == "__main__":
    main()
