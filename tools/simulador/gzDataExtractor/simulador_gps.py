import socket
import time
import math
import datetime

from gztopicextractor import GazeboTopicLector

# --- CONFIGURACIÓN ---
TCP_IP = "0.0.0.0"
TCP_PORT = 5801


def generar_rmc(lat_deg, lon_deg, vn, ve):
    """Convierte grados decimales a sentencia NMEA $GPRMC completa"""
    now = datetime.datetime.utcnow()
    timestamp = now.strftime("%H%M%S.%f")[:9]  # HHMMSS.sss
    datestamp = now.strftime("%d%m%y")

    # Conversión Latitud (DDMM.MMMM)
    lat_abs = abs(lat_deg)
    lat_d = int(lat_abs)
    lat_m = (lat_abs - lat_d) * 60
    lat_dir = "N" if lat_deg >= 0 else "S"
    lat_str = f"{lat_d:02d}{lat_m:07.4f}"

    # Conversión Longitud (DDDMM.MMMM)
    lon_abs = abs(lon_deg)
    lon_d = int(lon_abs)
    lon_m = (lon_abs - lon_d) * 60
    lon_dir = "E" if lon_deg >= 0 else "W"
    lon_str = f"{lon_d:03d}{lon_m:07.4f}"

    # Velocidad (Módulo de vectores en nudos)
    speed_ms = math.sqrt(vn**2 + ve**2)
    speed_knots = speed_ms * 1.94384
    course = math.degrees(math.atan2(ve, vn)) % 360

    # Construcción de la trama
    body = f"GPRMC,{timestamp},A,{lat_str},{lat_dir},{lon_str},{lon_dir},{speed_knots:.2f},{course:.2f},{datestamp},,,A"

    # Cálculo de Checksum XOR
    checksum = 0
    for char in body:
        checksum ^= ord(char)

    return f"${body}*{hex(checksum)[2:].upper():02s}\r\n"


def iniciar_servidor(get_data):
    # Crear Socket TCP
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((TCP_IP, TCP_PORT))
    server_socket.listen(1)

    print(f"📡 Servidor GPS NMEA activo en {TCP_IP}:{TCP_PORT}")
    print("Esperando conexión del cliente...")

    while True:
        conn, addr = server_socket.accept()
        print(f"✅ Cliente conectado desde: {addr}")

        try:
            while True:
                # 1. Obtener datos de la simulación
                sim = get_data()

                # ["latitude_deg", "longitude_deg", "velocity_east", "velocity_north"]

                # 2. Formatear a NMEA
                paquete_nmea = generar_rmc(
                    sim["latitude_deg"],
                    sim["longitude_deg"],
                    sim["velocity_north"],
                    sim["velocity_east"],
                )

                # 3. Enviar por TCP
                conn.sendall(paquete_nmea.encode("ascii"))
                print(f"Enviado: {paquete_nmea.strip()}")

                # 4. Esperar 1 segundo
                time.sleep(1)

        except (ConnectionResetError, BrokenPipeError):
            print("❌ Cliente desconectado. Esperando nueva conexión...")
            conn.close()


if __name__ == "__main__":
    CAMPOS_GPS = ["latitude_deg", "longitude_deg", "velocity_east", "velocity_north"]
    lector = GazeboTopicLector("/gps", CAMPOS_GPS)
    lector.conectar()

    iniciar_servidor(lector.leer)
