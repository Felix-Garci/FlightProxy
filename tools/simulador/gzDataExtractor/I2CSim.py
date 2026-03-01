import socket
import struct
from gztopicextractor import GazeboTopicLector

HOST = "0.0.0.0"
PORT = 5800

ADDR_BARO = 0x77
ADDR_MAG = 0x0D


def empaquetar_24bit_le(valor):
    """Convierte float a int24 Little Endian (Usado por BMP390)"""
    valor_int = max(0, min(16777215, int(valor)))
    return bytes([valor_int & 0xFF, (valor_int >> 8) & 0xFF, (valor_int >> 16) & 0xFF])


def empaquetar_16bit_signed_le(valor):
    """Convierte float a int16 con signo Little Endian (Usado por QMC5883L)"""
    valor_int = max(-32768, min(32767, int(valor)))
    return struct.pack("<h", valor_int)


class SensorI2C:
    """Clase base para todos los sensores I2C"""

    def handle_request(self, reg_addr, is_read, payload_len, data_in):
        raise NotImplementedError("Debe implementarse en la subclase")


class BarometroBMP390(SensorI2C):
    def __init__(self, lector_gz):
        self.gz = lector_gz
        self.temp_real = 25.0

        self.calib_data = bytes(
            [
                0x73,
                0x60,  # T1
                0x46,
                0x66,  # T2
                0xF6,  # T3
                0x7A,
                0x84,  # P1
                0x32,
                0x36,  # P2
                0x12,  # P3
                0x24,  # P4
                0x45,
                0x4B,  # P5
                0x58,
                0x58,  # P6
                0x80,  # P7
                0xF6,  # P8
                0x00,
                0x00,  # P9
                0x00,  # P10
                0x00,  # P11
            ]
        )

    def handle_request(self, reg_addr, is_read, payload_len, data_in):
        payload_out = b""
        if is_read:
            if reg_addr == 0x31:
                payload_out = self.calib_data
            elif reg_addr == 0x04:
                presion_gz = self.gz.leer().get("pressure", 101325.0)

                raw_pres = empaquetar_24bit_le(presion_gz)
                raw_temp = empaquetar_24bit_le(8388608)
                payload_out = raw_pres + raw_temp
            else:
                payload_out = bytes(payload_len)
        else:
            if reg_addr == 0x1B and payload_len > 0 and data_in[0] == 0x33:
                print("  [Baro BMP390] MODO NORMAL activado (CMD 0x33)")
        return payload_out


class MagnetometroQMC5883L(SensorI2C):
    def __init__(self, lector_gz):
        self.gz = lector_gz
        self.escala_mag = 3000.0

    def handle_request(self, reg_addr, is_read, payload_len, data_in):
        payload_out = b""
        if is_read:
            if reg_addr == 0x00:
                datos_gz = self.gz.leer()
                x = empaquetar_16bit_signed_le(datos_gz.get("x", 0) * self.escala_mag)
                y = empaquetar_16bit_signed_le(datos_gz.get("y", 0) * self.escala_mag)
                z = empaquetar_16bit_signed_le(datos_gz.get("z", 0) * self.escala_mag)
                payload_out = x + y + z
            else:
                payload_out = bytes(payload_len)
        else:
            if reg_addr == 0x0B and payload_len > 0 and data_in[0] == 0x01:
                print("  [Mag QMC5883L] SET/RESET activado")
            elif reg_addr == 0x09 and payload_len > 0 and data_in[0] == 0x1D:
                print("  [Mag QMC5883L] CONTROL_1 configurado (0x1D)")
        return payload_out


class BusI2CVirtual:
    def __init__(self):
        self.dispositivos = {}

    def registrar_dispositivo(self, direccion, dispositivo):
        self.dispositivos[direccion] = dispositivo
        print(f"[*] Dispositivo registrado en la dirección I2C: {hex(direccion)}")

    def procesar(self, dev_addr, reg_addr, is_read, payload_len, data_in):
        if dev_addr in self.dispositivos:
            respuesta = self.dispositivos[dev_addr].handle_request(
                reg_addr, is_read, payload_len, data_in
            )

            if len(respuesta) < payload_len:
                respuesta += bytes(payload_len - len(respuesta))
            return respuesta[:payload_len]
        else:
            return bytes(payload_len)


def main():
    topic_baro = "/world/empty_betaflight_world/model/iris_with_Betaflight/model/iris_with_standoffs/link/imu_link/sensor/air_pressure_sensor/air_pressure"
    topic_mag = "/mag"

    gz_baro = GazeboTopicLector(topic_baro, ["pressure"])
    gz_mag = GazeboTopicLector(topic_mag, ["x", "y", "z"])

    gz_baro.conectar()
    gz_mag.conectar()

    bus = BusI2CVirtual()
    bus.registrar_dispositivo(ADDR_BARO, BarometroBMP390(gz_baro))
    bus.registrar_dispositivo(ADDR_MAG, MagnetometroQMC5883L(gz_mag))

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server.bind((HOST, PORT))
        server.listen(1)
        print(f"\n[+] Bus I2C Virtual escuchando en {HOST}:{PORT}")

        while True:
            conn, addr = server.accept()
            print(f"\n[+] FlightProxy conectado: {addr}")

            try:
                while True:
                    header = conn.recv(4)
                    if not header or len(header) < 4:
                        break

                    dev_addr, reg_addr, is_read, payload_len = struct.unpack(
                        "BBBB", header
                    )

                    is_read_bool = bool(is_read)

                    data_in = b""
                    if not is_read_bool and payload_len > 0:
                        data_in = conn.recv(payload_len)

                    payload_respuesta = bus.procesar(
                        dev_addr, reg_addr, is_read_bool, payload_len, data_in
                    )

                    resp_header = struct.pack(
                        "BBBB",
                        dev_addr,
                        reg_addr,
                        int(is_read_bool),
                        len(payload_respuesta),
                    )
                    conn.sendall(resp_header + payload_respuesta)

            except Exception as e:
                print(f"[-] Error en la conexión: {e}")
            finally:
                conn.close()

    except KeyboardInterrupt:
        print("\nApagando simulador...")
    finally:
        server.close()


if __name__ == "__main__":
    main()
