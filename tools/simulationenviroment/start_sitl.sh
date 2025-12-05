#!/bin/bash
mkdir -p /opt/betaflight/data
cd /opt/betaflight/data
PID_BF=0

#nuestro
sleep 10
../obj/main/betaflight_SITL.elf 127.0.0.1 9002 &

while true; do
	sleep 10
done



kill_b() {
  if [ $PID_BF -ne 0 ]; then
    kill -9 $PID_BF
    PID_BF=0
  fi
}
trap kill_bf SIGTERM SIGINT

echo "🚀 Iniciando Gestor SITL Inteligente..."

while true; do
  # ------------------------------------------------------------------
  # PASO 1: ¿ESTÁ GAZEBO LISTO? (Check de Puerto)
  # ------------------------------------------------------------------
  # Buscamos si ALGUIEN escucha en el 9002 UDP.
  # Como compartimos red con Gateway, veremos su puerto aquí.
  GAZEBO_PORT=$(netstat -uln | grep ":9002 ")

  if [ -z "$GAZEBO_PORT" ]; then
    # CASO A: Gazebo apagado o cargando
    if [ $PID_BF -ne 0 ]; then echo "❌ Gazebo cerrado. Deteniendo SITL..."; kill_bf; fi
    echo "⏳ Esperando a que Gazebo abra el puerto 9002..."
    sleep 10
  else
    # ------------------------------------------------------------------
    # PASO 2: GESTIÓN DE BETAFLIGHT
    # ------------------------------------------------------------------
    if [ $PID_BF -eq 0 ]; then
      # CASO B: Gazebo listo, BF apagado -> ENCENDER (Primer arranque o tras crash)
      echo "✅ Puerto 9002 detectado. Lanzando Betaflight..."
      ../obj/main/betaflight_SITL.elf 127.0.0.1 9002 &
      PID_BF=$!
      # Damos 10 segundos de cortesía para que arranquen los datos
      sleep 10
    else
      # CASO C: Ambos corriendo -> VIGILAR TRÁFICO
      # Si no pasan paquetes UDP por el puerto 9002 en 2 seg, asumimos reinicio.
      timeout 22 tcpdump -i lo udp port 9002 -c 1 -n > /dev/null 2>&1
      TRAFFIC=$?
      if [ $TRAFFIC -ne 0 ]; then
        echo "⚠️ SILENCIO DETECTADO (Posible Reinicio de Simulación). Reiniciando BF..."
        kill_bf
      fi
    fi
  fi
  sleep 3
done
