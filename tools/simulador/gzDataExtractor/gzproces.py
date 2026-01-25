import subprocess


def leer_presion_gazebo():
    comando = [
        "gz",
        "topic",
        "-e",
        "-t",
        "/world/empty_betaflight_world/model/iris_with_Betaflight/model/iris_with_standoffs/link/imu_link/sensor/air_pressure_sensor/air_pressure",
        "-n",
        "1",
    ]

    try:
        # Ejecutamos el comando
        salida_cruda = subprocess.check_output(comando, text=True)

        presion = salida_cruda.split(":")[-1].strip().split(".")[0]
        presion = int(presion)
        return presion

    except subprocess.CalledProcessError as e:
        print("Error al ejecutar el comando:", e)
        return None


if __name__ == "__main__":
    print(leer_presion_gazebo())
