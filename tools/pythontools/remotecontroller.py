import pygame
import socket
import struct
import time

# --- CONFIGURACIÓN ---
UDP_IP = "127.0.0.1"
UDP_PORT = 12346
REFRESH_RATE = 60

# --- INICIALIZACIÓN ---
pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() == 0:
    print("❌ No se detectó mando. Asegúrate de ejecutar esto en WINDOWS si usas WSL.")
    exit()

joystick = pygame.joystick.Joystick(0)
joystick.init()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"✅ Mando: {joystick.get_name()}")
print("📡 Enviando 5 Canales (Roll, Pitch, Yaw, Throttle, AUX1)")

# --- VARIABLES DE ESTADO PARA EL BOTÓN (TOGGLE) ---
aux1_value = 1000  # Valor inicial (desactivado)
prev_button_x = False  # Estado anterior del botón X


def map_standard_axis(value, inverted=False):
    if inverted:
        value = -value
    ibus_val = int(1500 + (value * 500))
    return max(1000, min(2000, ibus_val))


def map_throttle_special(value):
    if value > 0:
        value = 0  # Zona muerta abajo
    ibus_val = int(1000 + (abs(value) * 1000))
    return max(1000, min(2000, ibus_val))


def create_ibus_packet(channels):
    packet = bytearray([0x20, 0x40])
    for i in range(14):
        val = channels[i] if i < len(channels) else 1500
        packet.extend(struct.pack("<H", val))

    checksum = 0xFFFF
    for b in packet:
        checksum -= b
    packet.extend(struct.pack("<H", checksum & 0xFFFF))
    return packet


try:
    while True:
        pygame.event.pump()

        # --- 1. LEER EJES (Canales 1-4) ---
        roll = map_standard_axis(joystick.get_axis(2))
        pitch = map_standard_axis(joystick.get_axis(3), inverted=True)
        yaw = map_standard_axis(joystick.get_axis(0))
        throttle = map_throttle_special(joystick.get_axis(1))

        # --- 2. LEER BOTÓN X (Canal 5 - AUX1) ---
        # El botón X suele ser el ID 2 en mandos Xbox en Windows.
        # IDs comunes: A=0, B=1, X=2, Y=3
        current_button_x = joystick.get_button(2)

        # Lógica de "Toggle" (Solo actuar cuando se PRESIÓNA, no mientras se mantiene)
        if current_button_x and not prev_button_x:
            if aux1_value == 1000:
                aux1_value = 2000
                # print("🔵 AUX1 ACTIVADO (2000)")
            else:
                aux1_value = 1000
                # print("⚪ AUX1 DESACTIVADO (1000)")

        # Actualizar el estado anterior para la siguiente vuelta
        prev_button_x = current_button_x

        # --- 3. EMPAQUETAR Y ENVIAR ---
        # Añadimos aux1_value como quinto elemento
        channels = [roll, pitch, throttle, yaw, aux1_value]

        print(f"R:{roll} P:{pitch} T:{throttle} Y:{yaw} AUX1:{aux1_value} ", end="\r")

        sock.sendto(create_ibus_packet(channels), (UDP_IP, UDP_PORT))

        time.sleep(1 / REFRESH_RATE)

except KeyboardInterrupt:
    sock.close()
    pygame.quit()
