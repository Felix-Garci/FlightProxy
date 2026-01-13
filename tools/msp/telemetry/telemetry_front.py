import dearpygui.dearpygui as dpg
import csv


class TelemetryTab:
    def __init__(self, bus, telemetry_manager, available_vars):
        self.bus = bus
        self.telemetry = telemetry_manager
        self.vars = available_vars
        self.plots_config = {}
        self.plot_count = 0

        # Tags para poder actualizar los elementos desde fuera
        self.btn_run_id = "btn_start_stop_telemetry"
        self.input_time_id = "input_buffer_time"
        self.input_freq_id = "input_buffer_freq"

        self.bus.register_provider("GET_TELEMETRY_DATA", self._provide_data_to_bus)

    def _provide_data_to_bus(self):
        raw_data = self.telemetry.get_data([])
        return {"data": list(raw_data), "columns": ["tiempo"] + self.vars}

    def create(self, tab_bar):
        with dpg.tab(label="Telemetría RT", parent=tab_bar):
            with dpg.group(horizontal=True):
                # 1. Botón Único Start/Stop
                dpg.add_button(
                    label="START",
                    callback=self.cb_toggle_running,
                    tag=self.btn_run_id,
                    width=80,
                )

                dpg.add_spacer(width=20)

                # 2. Configuración del Buffer
                dpg.add_text("Buffer:")
                dpg.add_input_int(
                    label="Segundos",
                    default_value=self.telemetry.timeframeS,
                    width=100,
                    tag=self.input_time_id,
                )
                dpg.add_input_int(
                    label="Hz",
                    default_value=self.telemetry.samplesperS,
                    width=100,
                    tag=self.input_freq_id,
                )
                dpg.add_button(label="Actualizar", callback=self.cb_update_config)

                dpg.add_spacer(width=20)
                dpg.add_button(label="+ Añadir Gráfico", callback=self.cb_add_plot)

            dpg.add_separator()
            dpg.add_group(tag="plots_container")

    def cb_toggle_running(self):
        """Alterna entre Start y Stop"""
        if self.telemetry.running:
            self.telemetry.stop()
        else:
            self.telemetry.start()

    def cb_update_config(self):
        """Recoge los valores de la UI y actualiza el buffer"""
        new_time = dpg.get_value(self.input_time_id)
        new_freq = dpg.get_value(self.input_freq_id)
        self.telemetry.update_buffer(new_time, new_freq)
        print(f"Buffer actualizado: {new_time}s a {new_freq}Hz")

    def update_plots(self):
        # --- SINCRONIZACIÓN AUTOMÁTICA DEL BOTÓN ---
        # Si el manager dice que no corre, pero el botón dice STOP, lo cambiamos.
        # Esto cubre el caso de errores en el thread o cierre inesperado.
        is_running = self.telemetry.running
        current_label = dpg.get_item_label(self.btn_run_id)

        if is_running and current_label == "START":
            dpg.configure_item(self.btn_run_id, label="STOP")
            # Opcional: Cambiar color a rojo
            # dpg.bind_item_theme(self.btn_run_id, theme_rojo)
        elif not is_running and current_label == "STOP":
            dpg.configure_item(self.btn_run_id, label="START")

        if not is_running:
            return

        # --- LÓGICA DE ACTUALIZACIÓN DE GRÁFICOS ---
        data_list = list(self.telemetry.get_data([]))
        if len(data_list) > 0 and len(data_list[0]) > 0:
            times = data_list[0]
            for p_id, active_vars in self.plots_config.items():
                for var in active_vars:
                    var_idx = self.vars.index(var) + 1
                    tag = f"series_{p_id}_{var}"
                    if dpg.does_item_exist(tag):
                        dpg.set_value(tag, [times, data_list[var_idx]])
                dpg.fit_axis_data(f"x_{p_id}")
                dpg.fit_axis_data(f"y_{p_id}")

    # (El resto de métodos cb_add_plot, cb_toggle, cb_download, cb_remove se mantienen igual)
    def cb_add_plot(self):
        self.plot_count += 1
        p_id = f"plot_{self.plot_count}"
        self.plots_config[p_id] = []
        with dpg.group(parent="plots_container", tag=f"group_{p_id}"):
            with dpg.group(horizontal=True):
                dpg.add_text(f"Gráfico {self.plot_count}", color=[255, 200, 0])
                dpg.add_button(
                    label="Descargar", callback=lambda: self.cb_download(p_id)
                )
                dpg.add_button(label="Eliminar", callback=lambda: self.cb_remove(p_id))
            with dpg.group(horizontal=True):
                for var in self.vars:
                    dpg.add_checkbox(
                        label=var, callback=self.cb_toggle, user_data=[p_id, var]
                    )
            with dpg.plot(height=300, width=-1, tag=p_id):
                dpg.add_plot_legend()
                dpg.add_plot_axis(dpg.mvXAxis, label="S", tag=f"x_{p_id}")
                dpg.add_plot_axis(dpg.mvYAxis, label="V", tag=f"y_{p_id}")

    def cb_toggle(self, sender, app_data, user_data):
        p_id, var = user_data
        tag = f"series_{p_id}_{var}"
        if app_data:
            self.plots_config[p_id].append(var)
            dpg.add_line_series([], [], label=var, parent=f"y_{p_id}", tag=tag)
        else:
            self.plots_config[p_id].remove(var)
            dpg.delete_item(tag)

    def cb_download(self, p_id):
        data = self.telemetry.get_data(self.plots_config[p_id])
        with open(f"datos_{p_id}.csv", "w", newline="") as f:
            csv.writer(f).writerows(zip(*data))

    def cb_remove(self, p_id):
        dpg.delete_item(f"group_{p_id}")
        self.plots_config.pop(p_id, None)

