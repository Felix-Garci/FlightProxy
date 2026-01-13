import dearpygui.dearpygui as dpg
import pandas as pd
from plantiden.plantiden import PlantIden


class IdenTab:
    def __init__(self, bus):
        self.bus = bus
        self.iden = PlantIden()
        self.last_results = None

        self.bus.register_provider("GET_IDENTIFIED_PLANT", self._provide_plant_params)

    def _provide_plant_params(self):
        """Devuelve parámetros extrayéndolos correctamente del sub-diccionario 'params'"""
        if self.last_results:
            # Extraemos el diccionario de parámetros internos
            p = self.last_results.get("params", {})

            # Ahora sí, accedemos a p['K'], p['tau'], etc.
            return {
                "type": str(self.last_results.get("model_type", "FOPDT")),
                "K": float(p.get("K", 0.0)),
                "tau": float(p.get("tau", 0.0)),
                "theta": float(p.get("theta", 0.0)),
                "zeta": float(p.get("zeta", 1.0)),
            }
        return None

    def create(self, tab_bar):
        # File Dialog para carga de archivos CSV
        with dpg.file_dialog(
            directory_selector=False,
            show=False,
            callback=self.cb_file_selected,
            tag="file_dialog_id",
            width=600,
            height=400,
        ):
            dpg.add_file_extension(".csv")

        with dpg.tab(label="Identificación", parent=tab_bar):
            with dpg.group(horizontal=True):
                # --- Panel Izquierdo: Controles ---
                with dpg.child_window(width=300):
                    with dpg.group(horizontal=True):
                        dpg.add_button(
                            label="Cargar CSV",
                            callback=lambda: dpg.show_item("file_dialog_id"),
                        )
                        # BOTÓN NUEVO: Importar desde telemetría
                        dpg.add_button(
                            label="RT Telemetry",
                            callback=self.cb_import_from_telemetry,
                            # color=[0, 120, 200],
                        )

                    dpg.add_separator()
                    dpg.add_text("1. Asignar Variables")
                    dpg.add_combo(
                        label="t", tag="combo_t", callback=self.cb_update_plot
                    )
                    dpg.add_combo(
                        label="u", tag="combo_u", callback=self.cb_update_plot
                    )
                    dpg.add_combo(
                        label="y", tag="combo_y", callback=self.cb_update_plot
                    )

                    dpg.add_spacer(height=10)
                    dpg.add_text("2. Configurar Fit")
                    dpg.add_combo(
                        label="Modelo",
                        tag="combo_model",
                        items=["FOPDT", "SOPDT", "IPDT", "FO_SIMPLE"],
                        default_value="FOPDT",
                    )
                    dpg.add_button(
                        label="CALCULAR FIT",
                        callback=self.cb_run_fit,
                        width=-1,
                        height=40,
                    )

                    dpg.add_separator()
                    dpg.add_text("Resultados:", color=[0, 255, 0])
                    dpg.add_text("", tag="txt_results", wrap=280)

                # --- Panel Derecho: Gráficos ---
                with dpg.group():
                    with dpg.plot(label="Data Preview", height=-1, width=-1):
                        dpg.add_plot_legend()
                        dpg.add_plot_axis(
                            dpg.mvXAxis, label="Tiempo", tag="x_axis_iden"
                        )
                        y_ax = dpg.add_plot_axis(
                            dpg.mvYAxis, label="Amplitud", tag="y_axis_iden"
                        )

                        dpg.add_line_series(
                            [], [], label="u", tag="series_u", parent=y_ax
                        )
                        dpg.add_line_series(
                            [], [], label="y", tag="series_y", parent=y_ax
                        )
                        dpg.add_line_series(
                            [], [], label="Fit", tag="series_fit", parent=y_ax
                        )

                        dpg.add_drag_line(
                            label="Start",
                            color=[255, 0, 0, 255],
                            default_value=0.0,
                            tag="line_start",
                        )
                        dpg.add_drag_line(
                            label="End",
                            color=[255, 0, 0, 255],
                            default_value=1.0,
                            tag="line_end",
                        )

    def _setup_dropdowns(self, cols):
        """Helper para actualizar los combos de variables"""
        for item in ["combo_t", "combo_u", "combo_y"]:
            dpg.configure_item(item, items=cols)
            # Auto-selección básica para ahorrar tiempo
            if "t" in item:
                dpg.set_value(item, cols[0])
            elif "u" in item and len(cols) > 1:
                dpg.set_value(item, cols[1])
            elif "y" in item and len(cols) > 2:
                dpg.set_value(item, cols[2])

        # Refrescar el gráfico con la nueva selección
        self.cb_update_plot()

    def cb_file_selected(self, sender, app_data):
        success, msg = self.iden.load_csv(app_data["file_path_name"])
        if success:
            self._setup_dropdowns(list(self.iden.df.columns))

    def cb_import_from_telemetry(self):
        # Pedimos los datos al bus
        response = self.bus.request("GET_TELEMETRY_DATA")

        if response and len(response["data"]) > 0:
            data_raw = response["data"]
            cols = response["columns"]

            # 1. Convertimos la respuesta en un DataFrame para PlantIden
            # Creamos un diccionario vinculando cada columna con su lista de datos
            df_dict = {cols[i]: data_raw[i] for i in range(len(cols))}
            self.iden.df = pd.DataFrame(df_dict)

            # 2. Actualizamos la interfaz
            self._setup_dropdowns(cols)
        else:
            print("Error: No se han recibido datos o el buffer está vacío.")

    def cb_update_plot(self):
        t_col = dpg.get_value("combo_t")
        u_col = dpg.get_value("combo_u")
        y_col = dpg.get_value("combo_y")

        if all([t_col, u_col, y_col]):
            self.iden.set_columns(t_col, u_col, y_col)
            t, u, y = self.iden.get_full_data()
            dpg.set_value("series_u", [t, u])
            dpg.set_value("series_y", [t, y])
            dpg.fit_axis_data("x_axis_iden")
            dpg.fit_axis_data("y_axis_iden")

    def cb_run_fit(self):
        self.iden.set_region(dpg.get_value("line_start"), dpg.get_value("line_end"))
        model_type = dpg.get_value("combo_model")
        success, res = self.iden.run_identification(dpg.get_value("combo_model"))
        if success:
            self.last_results = res
            self.last_results["model_type"] = model_type

            dpg.set_value("txt_results", self.iden.get_results_summary())
            dpg.set_value("series_fit", [res["t_pred"], res["y_pred"]])
