#!/bin/bash

# --- CONFIGURACIÓN ---
SSID="ESP32_Dev_Net"
PASS="esp32pass"
INTERFACE="wlan0"
ESP32_IP="10.42.0.145"  

# PUERTOS
PORT_ESP_SERVER=12345        # TCP: PC -> ESP
PORT_ESP_UDP=12346           # UDP: PC -> ESP
PORT_PC_REAL=5762            # TCP: El puerto de tu App en el PC
PORT_PC_PUENTE=15762         # TCP: El puerto que usará el ESP32 para no chocar

# Colores
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

if [[ $EUID -ne 0 ]]; then
   echo "Ejecuta con sudo."
   exit 1
fi

cleanup() {
    echo -e "\n${BLUE}[!] Limpiando procesos y red...${NC}"
    pkill -9 -f "socat.*$PORT_ESP_UDP|socat.*$PORT_ESP_SERVER|socat.*$PORT_PC_PUENTE"
    nmcli con down "$SSID" 2>/dev/null
    nmcli con delete "$SSID" 2>/dev/null
    exit
}

trap cleanup SIGINT SIGTERM

# Limpieza preventiva
pkill -9 -f "socat" 2>/dev/null

echo -e "${BLUE}[*] Levantando Hotspot: $SSID...${NC}"
nmcli device wifi hotspot ssid "$SSID" password "$PASS" ifname "$INTERFACE" con-name "$SSID"
sleep 2

GW_IP=$(ip addr show "$INTERFACE" | grep "inet " | awk '{print $2}' | cut -d/ -f1)

echo -e "--------------------------------------------------------"
echo -e "Gateway (PC): ${GREEN}$GW_IP${NC}"
echo -e "ESP32 IP:     ${GREEN}$ESP32_IP${NC}"
echo -e "PUENTE TCP:   ESP32:${PORT_PC_PUENTE} -> PC:${PORT_PC_REAL}"
echo -e "--------------------------------------------------------"

# PUENTES SOCAT
# 1. UDP: PC Local -> ESP32
socat UDP4-LISTEN:$PORT_ESP_UDP,bind=127.0.0.1,fork,reuseaddr UDP4-DATAGRAM:$ESP32_IP:$PORT_ESP_UDP &

# 2. TCP: PC Local -> ESP32 Server
socat TCP4-LISTEN:$PORT_ESP_SERVER,bind=127.0.0.1,fork,reuseaddr TCP4:$ESP32_IP:$PORT_ESP_SERVER &

# 3. TCP: ESP32 -> PC (Usando el puerto puente 15762 para evitar el error 'Address in use')
socat -d -d -d TCP4-LISTEN:$PORT_PC_PUENTE,bind=$GW_IP,fork,reuseaddr TCP4:127.0.0.1:$PORT_PC_REAL &

echo -e "${GREEN}[✔] Puentes activos. Configura el ESP32 al puerto $PORT_PC_PUENTE${NC}"
wait
