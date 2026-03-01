import subprocess
import threading
import json


class GazeboTopicLector:
    def __init__(self, topic, campos):
        self.topic = topic
        self.campos = campos
        # Inicializamos el diccionario con None o 0 para que siempre tenga estructura
        self.datos_actuales = {campo: None for campo in campos}
        self.proceso = None
        self.thread = threading.Thread(target=self._leer_stream, daemon=True)

    def conectar(self):
        # Ejecutamos el comando
        comando = ["gz", "topic", "-e", "-t", self.topic]

        try:
            self.proceso = subprocess.Popen(
                comando,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1,  # Forzamos lectura línea a línea
            )
            self.thread.start()
            print(f"📡 Escuchando {self.topic}...")
        except Exception as e:
            print(f"❌ Error al iniciar el proceso: {e}")

    def _leer_stream(self):
        # Leemos cada línea de forma continua
        for linea in iter(self.proceso.stdout.readline, ""):
            linea = linea.strip()

            # Si la línea tiene el formato "campo: valor"
            if ":" in linea:
                partes = linea.split(":", 1)
                clave = partes[0].strip()

                if clave in self.campos:
                    valor_raw = partes[1].strip()
                    try:
                        # Intentamos convertir a número (soporta notación científica)
                        valor = float(valor_raw)
                        if valor.is_integer():
                            valor = int(valor)
                        # Actualizamos directamente el diccionario global
                        self.datos_actuales[clave] = valor
                    except ValueError:
                        # Si es un string (ej. "warning", "error"), lo guardamos tal cual
                        self.datos_actuales[clave] = valor_raw

    def leer(self):
        """Devuelve una copia de los datos capturados hasta el momento."""
        return self.datos_actuales.copy()


# --- MODO DE PRUEBA ---
if __name__ == "__main__":
    import time

    # Configura aquí tus campos reales
    # CAMPOS_GPS = ["latitude_deg", "longitude_deg", "velocity_east", "velocity_north"]
    # lector = GazeboTopicLector("/gps", CAMPOS_GPS)  # Cambia el topic al tuyo

    # campos = ["pressure"]
    # topic = "/world/empty_betaflight_world/model/iris_with_Betaflight/model/iris_with_standoffs/link/imu_link/sensor/air_pressure_sensor/air_pressure"

    campos = ["x", "y", "z"]
    topic = "/mag"
    lector = GazeboTopicLector(topic, campos)

    lector.conectar()

    print("Esperando datos...")
    try:
        while True:
            datos = lector.leer()
            # Solo printeamos si al menos un valor ha dejado de ser None
            if any(v is not None for v in datos.values()):
                print(f"📊 JSON: {json.dumps(datos)}")
            else:
                print("⏳ Esperando valores válidos del topic...", end="\r")

            time.sleep(0.2)
    except KeyboardInterrupt:
        print("\nDeteniendo lector...")
