import dearpygui.dearpygui as dpg
import time
import csv


class TelemetryTab:
    def __init__(self, bus, telemetry_manager):
        self.bus = bus
        self.telemetry = telemetry_manager
        self.plots_config = {}  # {plot_id: [lista_variables_pintandose]}
        self.plot_count = 0
        self.frame_update = 0.1
        self.prev_update = 0

        # Guardaremos los IDs de los contenedores de checkboxes para refrescarlos
        self.selector_groups = {}

    def create(self, tab_bar):
        with dpg.tab(label="Telemetría RT", parent=tab_bar):
            # Barra de Herramientas Superior
            with dpg.group(horizontal=True):
                self.btn_start = dpg.add_button(
                    label="START",
                    callback=self.cb_toggle_running,
                    width=80,
                )
                dpg.add_spacer(width=10)
                self.in_time = dpg.add_input_int(
                    label="Segundos",
                    default_value=self.telemetry.get_timeframeS(),
                    width=90,
                )
                self.in_freq = dpg.add_input_int(
                    label="Hz",
                    default_value=self.telemetry.get_samplesperS(),
                    width=90,
                )
                dpg.add_button(label="Aplicar Config", callback=self.cb_update_config)

            dpg.add_separator()

            with dpg.group(horizontal=True):
                # COLUMNA IZQUIERDA: Menú de Variables (Suscripción)
                with dpg.child_window(width=250, height=-1):
                    dpg.add_text("Menú de Variables", color=[0, 255, 255])
                    dpg.add_button(label="Refrescar Menú", callback=self.refresh_menu)
                    self.menu_container = dpg.add_group()

                # COLUMNA DERECHA: Zona de Gráficos
                with dpg.group():
                    dpg.add_button(label="+ Añadir Gráfico", callback=self.cb_add_plot)
                    self.plots_container = dpg.add_group()

        self.refresh_menu()

    def refresh_menu(self):
        dpg.delete_item(self.menu_container, children_only=True)
        menu = self.telemetry.get_menu()
        active_vars = self.telemetry.get_active_vars()

        for cmd_sig, vars_list in menu.items():
            with dpg.tree_node(label=cmd_sig, parent=self.menu_container):
                for var_id in vars_list:
                    dpg.add_checkbox(
                        label=var_id,
                        default_value=var_id in active_vars,
                        callback=self.cb_menu_subscription,
                        user_data=var_id,
                    )

    def cb_menu_subscription(self, sender, app_data, var_id):
        if app_data:
            self.telemetry.add_variables([var_id])
        else:
            self.telemetry.remove_variables([var_id])

        # Como el Manager hace stop(), actualizamos UI
        dpg.set_item_label(self.btn_start, "START")

        # ¡IMPORTANTE! Actualizamos los selectores de todos los gráficos abiertos
        self.refresh_all_plot_selectors()

    def refresh_all_plot_selectors(self):
        """Actualiza los checkboxes de cada gráfico con las variables actualmente activas"""
        active_vars = self.telemetry.get_active_vars()
        for p_id, group_id in self.selector_groups.items():
            dpg.delete_item(group_id, children_only=True)
            for var in active_vars:
                # Mantener el check si ya se estaba pintando
                already_plotting = var in self.plots_config.get(p_id, [])
                dpg.add_checkbox(
                    label=var,
                    parent=group_id,
                    default_value=already_plotting,
                    callback=self.cb_toggle_series,
                    user_data=[p_id, var],
                )

    def cb_toggle_running(self):
        if self.telemetry.is_running():
            self.telemetry.stop()
            dpg.set_item_label(self.btn_start, "START")
        else:
            self.telemetry.start()  # Aquí el Gatherer regenera el buffer
            dpg.set_item_label(self.btn_start, "STOP")

    def cb_update_config(self):
        t = dpg.get_value(self.in_time)
        f = dpg.get_value(self.in_freq)
        self.telemetry.update_buffer_size(t, f)

    def cb_add_plot(self):
        self.plot_count += 1
        p_id = f"plot_{self.plot_count}"
        self.plots_config[p_id] = []

        with dpg.group(parent=self.plots_container) as group_root:
            with dpg.group(horizontal=True):
                dpg.add_text(f"Gráfico {self.plot_count}", color=[255, 200, 0])
                dpg.add_button(
                    label="Exportar CSV",
                    callback=self.cb_export_csv,
                    user_data=p_id,
                    small=True,
                )
                dpg.add_button(
                    label="Eliminar",
                    callback=lambda: self.cb_remove_plot(p_id, group_root),
                )

            # Contenedor dinámico de variables para este gráfico
            self.selector_groups[p_id] = dpg.add_group(horizontal=True)

            with dpg.plot(height=250, width=-1, tag=p_id):
                dpg.add_plot_legend()
                dpg.add_plot_axis(dpg.mvXAxis, label="s", tag=f"x_{p_id}")
                dpg.add_plot_axis(dpg.mvYAxis, label="v", tag=f"y_{p_id}")

        self.refresh_all_plot_selectors()

    def cb_toggle_series(self, sender, app_data, user_data):
        p_id, var = user_data
        tag = f"series_{p_id}_{var}"
        if app_data:
            if var not in self.plots_config[p_id]:
                self.plots_config[p_id].append(var)
            if not dpg.does_item_exist(tag):
                dpg.add_line_series([], [], label=var, parent=f"y_{p_id}", tag=tag)
        else:
            if var in self.plots_config[p_id]:
                self.plots_config[p_id].remove(var)
            if dpg.does_item_exist(tag):
                dpg.delete_item(tag)

    def update_plots(self):
        if not self.telemetry.is_running():
            dpg.set_item_label(self.btn_start, "START")
            return

        if time.time() - self.prev_update < self.frame_update:
            return

        # Pedimos solo las variables que realmente queremos pintar para ahorrar CPU
        # O pedimos todas ([]) y mapeamos con cuidado.
        active_vars = self.telemetry.get_active_vars()
        data_gen = self.telemetry.get_data([])
        data_list = list(data_gen)

        if not data_list or len(data_list[0]) == 0:
            return

        times = data_list[0]
        # Crear un mapa rápido {nombre_variable: lista_de_datos}
        # El buffer del gatherer tiene [timestamp, var1, var2...]
        data_map = {"timestamp": times}
        for i, var_name in enumerate(active_vars):
            data_map[var_name] = data_list[i + 1]

        for p_id, series_list in self.plots_config.items():
            for var in series_list:
                if var in data_map:
                    tag = f"series_{p_id}_{var}"
                    if dpg.does_item_exist(tag):
                        dpg.set_value(tag, [list(times), list(data_map[var])])

            dpg.fit_axis_data(f"x_{p_id}")
            dpg.fit_axis_data(f"y_{p_id}")

        self.prev_update = time.time()

    def cb_export_csv(self, sender, app_data, p_id):
        """Exporta los datos actuales de las variables asignadas a este gráfico"""
        vars_to_export = self.plots_config.get(p_id, [])

        if not vars_to_export:
            print(f"Error: El gráfico {p_id} no tiene variables seleccionadas.")
            return

        # Obtenemos los datos filtrados por las variables del gráfico
        # get_data devuelve un generador: [timestamp, var1, var2...]
        data_gen = self.telemetry.get_data(vars_to_export)
        data_list = list(data_gen)

        if not data_list or len(data_list[0]) == 0:
            print("Error: No hay datos en el buffer para exportar.")
            return

        filename = f"telemetry_export_{p_id}_{int(time.time())}.csv"

        try:
            with open(filename, mode="w", newline="") as file:
                writer = csv.writer(file)

                # Cabecera: ['timestamp', 'var_name1', 'var_name2'...]
                header = ["timestamp"] + vars_to_export
                writer.writerow(header)

                # Transponemos las listas de datos para escribir filas [t, v1, v2]
                # zip(*data_list) agrupa el i-ésimo elemento de cada sublista
                rows = zip(*data_list)
                writer.writerows(rows)

            print(f"Datos exportados exitosamente a {filename}")

        except Exception as e:
            print(f"Error al guardar el CSV: {e}")

    def cb_remove_plot(self, p_id, group_root):
        dpg.delete_item(group_root)
        self.plots_config.pop(p_id, None)
        self.selector_groups.pop(p_id, None)
