import dearpygui.dearpygui as dpg
from commander import init as getc
from telemetryManager import TelemetryManager
from remote import Remote
import csv

"""
c = getc("localhost", 12345, print("error"))
c.client.connect()

c.process(300, [10.0, 19.0, 30.0])
c.process(200, "altHold")
c.process(201, 10)

a = c.process(301)
print(a)
"""


class DroneApp:
    def __init__(self, ip, port):
        # inicalizamos libreria
        dpg.create_context()

        def mostrar_desconcectado():
            dpg.set_value("status_text", "DESCONECTADO")
            dpg.configure_item("status_text", color=[255, 0, 0])

        self.c = getc(ip, port, mostrar_desconcectado)

        # self.avalible_vars = ["reference", "actual", "output", "p", "i", "d"]
        self.avalible_vars = ["throttle", "altura", "velocidad"]
        self.plots_config = {}  # { plot_id: [lista_de_variables_activas] }
        self.plot_count = 0

        self.telemetry = TelemetryManager(
            lambda: self.c.process(250),
            self.avalible_vars,
            timeframeS=60,
            samplesperS=50,
        )

        self.remote = Remote(
            lambda data: self.c.process(270, data),
            periodMS=10,
            default_cn=[1500, 1500, 1000, 1500],
        )

        # configuramos interfaz
        self._setup_ui()

        # configuramos ventana
        dpg.create_viewport(title="Drone Lab v1.0", width=1000, height=800)
        dpg.setup_dearpygui()
        dpg.show_viewport()

    def _setup_ui(self):
        with dpg.window(label="Main Window", tag="PrimaryWindow"):
            with dpg.group(horizontal=True):
                dpg.add_text("Estado:")
                dpg.add_text("DESCONECTADO", tag="status_text")
                dpg.configure_item("status_text", color=[255, 0, 0])
                dpg.add_button(label="Conectar", callback=self.cb_conectar)

            with dpg.tab_bar():
                with dpg.tab(label="Setings"):
                    dpg.add_spacer(height=10)
                    dpg.add_text("SETINGS")

                    dpg.add_spacer(height=10)
                    # SECCIÓN 1: Un campo y un botón (Ej: Setpoint de Altura)
                    dpg.add_text("Seleccionar Control", color=[100, 150, 255])
                    with dpg.group(horizontal=True):
                        dpg.add_input_text(
                            label="Conrol",
                            tag="input_ctrlName",
                            width=100,
                            default_value="altHold",
                        )
                        dpg.add_button(label="Enviar", callback=self.cb_enviar_control)

                    dpg.add_separator()
                    dpg.add_spacer(height=10)

                    # SECCIÓN 2: Tres campos y un botón (Ej: Sintonía PID)
                    dpg.add_text("Ajuste de Constantes PID", color=[100, 255, 150])
                    with dpg.group(horizontal=True):
                        dpg.add_input_text(
                            label="P", tag="input_p", width=60, default_value="1.0"
                        )
                        dpg.add_input_text(
                            label="I", tag="input_i", width=60, default_value="0.1"
                        )
                        dpg.add_input_text(
                            label="D", tag="input_d", width=60, default_value="0.05"
                        )
                        dpg.add_button(label="Cargar PID", callback=self.cb_cargar_pid)

                    dpg.add_separator()
                    dpg.add_spacer(height=10)

                    # SECCIÓN 3: Un campo y un botón (Ej: Comando especial o ID)
                    dpg.add_text(
                        "Seleccionar periodo de muestreo", color=[255, 100, 100]
                    )
                    with dpg.group(horizontal=True):
                        dpg.add_input_text(
                            label="periodo MS",
                            default_value="10",
                            tag="input_periodMS",
                            width=100,
                        )
                        dpg.add_button(
                            label="Ejecutar", callback=self.cb_enviar_periodoMS
                        )

                with dpg.tab(label="Telemetría RT"):
                    with dpg.group(horizontal=True):
                        dpg.add_button(
                            label="START",
                            callback=self.telemetry.start,
                            # color=[0, 150, 0],
                        )
                        dpg.add_button(
                            label="STOP",
                            callback=self.telemetry.stop,
                            # color=[150, 0, 0],
                        )
                        dpg.add_button(
                            label="+ Añadir Gráfico", callback=self.cb_add_plot
                        )

                    dpg.add_separator()
                    # Contenedor donde se meterán los gráficos dinámicos
                    dpg.add_group(tag="plots_container")

                with dpg.tab(label="remote"):
                    with dpg.group(horizontal=True):
                        dpg.add_button(
                            label="START",
                            callback=self.remote.start,
                        )
                        dpg.add_button(
                            label="STOP",
                            callback=self.remote.stop,
                        )
                    dpg.add_separator()

                    with dpg.group(horizontal=True):
                        dpg.add_button(
                            label="ARM",
                            callback=self.remote.arm,
                        )
                        dpg.add_button(
                            label="DISARM",
                            callback=self.remote.unarm,
                        )
                    dpg.add_separator()

                    with dpg.group(horizontal=False):
                        dpg.add_input_int(
                            label="channel",
                            tag="input_channel",
                            width=100,
                            default_value=2,
                        )
                        dpg.add_input_int(
                            label="startVal",
                            tag="input_startVal",
                            width=100,
                            default_value=1000,
                        )
                        dpg.add_input_int(
                            label="endVal",
                            tag="input_endVal",
                            width=100,
                            default_value=2000,
                        )
                        dpg.add_input_int(
                            label="durationS",
                            tag="input_durationS",
                            width=100,
                            default_value=4,
                        )
                        dpg.add_input_int(
                            label="repeats",
                            tag="input_repeats",
                            width=100,
                            default_value=1,
                        )

                        dpg.add_button(
                            label="STEP TEST",
                            callback=self.cb_step_test,
                        )
                    dpg.add_separator()

    def cb_step_test(self):
        idx = dpg.get_value("input_channel")
        s_val = dpg.get_value("input_startVal")
        e_val = dpg.get_value("input_endVal")
        durationS = dpg.get_value("input_durationS")
        rep = dpg.get_value("input_repeats")
        self.remote.play_step(idx, s_val, e_val, durationS, rep)

    def cb_add_plot(self):
        self.plot_count += 1
        plot_id = f"plot_{self.plot_count}"
        self.plots_config[plot_id] = []

        with dpg.group(parent="plots_container", tag=f"group_{plot_id}"):
            with dpg.group(horizontal=True):
                dpg.add_text(f"Grafico {self.plot_count}", color=[255, 200, 0])
                dpg.add_button(
                    label=f"Descargar", callback=lambda: self.cb_dawload_data(plot_id)
                )
                dpg.add_button(
                    label="Eliminar", callback=lambda: self.cb_remove_plot(plot_id)
                )
            with dpg.group(horizontal=True):
                for var in self.avalible_vars:
                    dpg.add_checkbox(
                        label=var,
                        callback=self.cb_toggle_variable,
                        user_data=[plot_id, var],
                    )

            with dpg.plot(label=f"Vista {plot_id}", height=300, width=-1, tag=plot_id):
                dpg.add_plot_legend()
                dpg.add_plot_axis(dpg.mvXAxis, label="Tiempo (S)", tag=f"x_{plot_id}")
                dpg.add_plot_axis(dpg.mvYAxis, label="Valor", tag=f"y_{plot_id}")
            dpg.add_spacer(height=20)

    def cb_dawload_data(self, plot_id):
        variables_activas = self.plots_config[plot_id]
        data = self.telemetry.get_data(variables_activas)
        with open("datos.csv", "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerows(zip(*data))

    def cb_remove_plot(self, plot_id):
        dpg.delete_item(f"group_{plot_id}")
        if plot_id in self.plots_config:
            del self.plots_config[plot_id]

    def cb_toggle_variable(self, sender, app_data, user_data):
        plot_id, var_name = user_data
        is_checked = app_data
        series_tag = f"series_{plot_id}_{var_name}"
        if is_checked:
            if var_name not in self.plots_config[plot_id]:
                self.plots_config[plot_id].append(var_name)
                dpg.add_line_series(
                    [], [], label=var_name, parent=f"y_{plot_id}", tag=series_tag
                )
        else:
            if var_name in self.plots_config[plot_id]:
                self.plots_config[plot_id].remove(var_name)
                dpg.delete_item(series_tag)

    def cb_conectar(self):
        if self.c.client.connect():
            dpg.set_value("status_text", "CONECTADO")
            dpg.configure_item("status_text", color=[0, 255, 0])

    def cb_enviar_control(self):
        valor = dpg.get_value("input_ctrlName")
        self.c.process(200, valor)

    def cb_cargar_pid(self):
        p = float(dpg.get_value("input_p"))
        i = float(dpg.get_value("input_i"))
        d = float(dpg.get_value("input_d"))
        self.c.process(300, [p, i, d])

    def cb_enviar_periodoMS(self):
        periodoMS = int(dpg.get_value("input_periodMS"))
        self.c.process(201, periodoMS)

    def run(self):
        while dpg.is_dearpygui_running():
            if self.telemetry.running:
                data_gen = self.telemetry.get_data([])
                data_list = list(data_gen)

                if len(data_list) > 0 and len(data_list[0]) > 0:
                    times = data_list[0]
                    for plot_id, active_vars in self.plots_config.items():
                        for var_name in active_vars:
                            var_idx = self.avalible_vars.index(var_name) + 1
                            series_tag = f"series_{plot_id}_{var_name}"

                            if dpg.does_item_exist(series_tag):
                                dpg.set_value(series_tag, [times, data_list[var_idx]])

                        dpg.fit_axis_data(f"x_{plot_id}")
                        dpg.fit_axis_data(f"y_{plot_id}")

            dpg.render_dearpygui_frame()
        dpg.destroy_context()


if __name__ == "__main__":
    app = DroneApp("localhost", 12345)
    dpg.set_primary_window("PrimaryWindow", True)
    app.run()
