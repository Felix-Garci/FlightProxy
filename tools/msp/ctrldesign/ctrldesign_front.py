import dearpygui.dearpygui as dpg
from ctrldesign.ctrldesign import ControlDesign


class DesignTab:
    def __init__(self, bus):
        self.bus = bus
        self.ctrl = ControlDesign()
        # Definimos qué parámetros necesita cada modelo para la visibilidad
        self.model_params = {
            "FOPDT": ["in_k", "in_tau", "in_theta"],
            "SOPDT": ["in_k", "in_tau", "in_theta", "in_zeta"],
            "IPDT": ["in_k", "in_theta"],
            "FO_SIMPLE": ["in_k", "in_tau"],
        }

    def create(self, tab_bar):
        with dpg.tab(label="Control Design", parent=tab_bar):
            with dpg.group(horizontal=True):
                # --- Panel Izquierdo: Configuración ---
                with dpg.child_window(width=320):
                    with dpg.group(horizontal=True):
                        dpg.add_text("1. PLANTA", color=[255, 255, 0])
                        # Botón para traer datos de la pestaña de Identificación
                        dpg.add_button(
                            label="Importar ID",
                            callback=self.cb_import_from_iden,
                            small=True,
                        )

                    dpg.add_combo(
                        ["FOPDT", "SOPDT", "IPDT", "FO_SIMPLE"],
                        label="Tipo",
                        tag="sel_model_type",
                        default_value="FOPDT",
                        callback=self._update_visibility,  # Se ejecuta al cambiar manual
                    )

                    # Definición de todos los inputs (usamos tags únicos)
                    dpg.add_input_float(
                        label="K", tag="in_k", default_value=0.01, format="%.4f"
                    )
                    dpg.add_input_float(
                        label="Tau", tag="in_tau", default_value=2.0, format="%.4f"
                    )
                    dpg.add_input_float(
                        label="Zeta", tag="in_zeta", default_value=1.0, format="%.4f"
                    )
                    dpg.add_input_float(
                        label="Theta", tag="in_theta", default_value=0.1, format="%.4f"
                    )

                    dpg.add_spacer(height=10)
                    dpg.add_text("2. HARDWARE (Dron)", color=[0, 255, 255])
                    dpg.add_input_float(
                        label="U Offset", tag="in_uoffset", default_value=1000.0
                    )
                    dpg.add_input_float(
                        label="U Min", tag="in_umin", default_value=1000.0
                    )
                    dpg.add_input_float(
                        label="U Max", tag="in_umax", default_value=2000.0
                    )

                    dpg.add_text("3. OBJETIVO", color=[0, 255, 0])
                    dpg.add_slider_float(
                        label="Agresividad",
                        tag="in_aggr",
                        min_value=0.5,
                        max_value=4.0,
                        default_value=1.0,
                    )
                    dpg.add_input_float(
                        label="Setpoint", tag="in_sp", default_value=10.0
                    )
                    dpg.add_input_float(label="Ts", tag="in_ts", default_value=0.01)
                    dpg.add_input_float(
                        label="Duración", tag="in_dur", default_value=10.0
                    )

                    dpg.add_button(
                        label="SIMULAR",
                        callback=self.cb_run_simulation,
                        width=-1,
                        height=40,
                    )

                    dpg.add_text("Kp: 0.0000", tag="res_kp")
                    dpg.add_text("Ki: 0.0000", tag="res_ki")
                    dpg.add_text("Kd: 0.0000", tag="res_kd")

                # --- Panel Derecho: Gráficos ---
                with dpg.group():
                    with dpg.plot(label="Salida (PV)", height=350, width=-1):
                        dpg.add_plot_legend()
                        ax_pv = dpg.add_plot_axis(dpg.mvXAxis, label="s")
                        ay_pv = dpg.add_plot_axis(dpg.mvYAxis, label="Altura")
                        dpg.add_line_series(
                            [], [], label="PV", tag="plot_pv", parent=ay_pv
                        )
                        dpg.add_line_series(
                            [], [], label="SP", tag="plot_sp", parent=ay_pv
                        )

                    with dpg.plot(label="Control (U)", height=300, width=-1):
                        ax_u = dpg.add_plot_axis(dpg.mvXAxis, label="s")
                        ay_u = dpg.add_plot_axis(dpg.mvYAxis, label="PWM")
                        dpg.add_line_series(
                            [], [], label="U", tag="plot_op", parent=ay_u
                        )

            # Ajustar visibilidad inicial al crear la pestaña
            self._update_visibility()

    def _update_visibility(self):
        """Muestra u oculta campos según el modelo seleccionado"""
        model = dpg.get_value("sel_model_type")
        required = self.model_params.get(model, [])

        all_inputs = ["in_k", "in_tau", "in_zeta", "in_theta"]
        for inp in all_inputs:
            if inp in required:
                dpg.show_item(inp)
            else:
                dpg.hide_item(inp)

    def cb_import_from_iden(self):
        """Solicita datos al bus, actualiza inputs y refresca la visibilidad"""
        plant_data = self.bus.request("GET_IDENTIFIED_PLANT")

        if not plant_data:
            print("Error: No se recibió información de la planta.")
            return

        # 1. Actualizar el Selector de Modelo (Combo)
        model_type = plant_data.get("type", "FOPDT")
        if model_type in ["FOPDT", "SOPDT", "IPDT", "FO_SIMPLE"]:
            dpg.set_value("sel_model_type", model_type)

        # 2. Actualizar cada input individualmente
        # Usamos tags fijos para asegurar que coincidan con los definidos en create()
        dpg.set_value("in_k", plant_data.get("K", 0.0))
        dpg.set_value("in_tau", plant_data.get("tau", 0.0))
        dpg.set_value("in_theta", plant_data.get("theta", 0.0))
        dpg.set_value("in_zeta", plant_data.get("zeta", 1.0))

        # 3. SINCRONIZAR EL BACKEND (Objeto ControlDesign)
        # Esto es vital para que al dar click a 'Simular' ya tenga los datos
        self.ctrl.set_model(
            model_type,
            {
                "K": plant_data.get("K", 0.0),
                "tau": plant_data.get("tau", 0.0),
                "theta": plant_data.get("theta", 0.0),
                "zeta": plant_data.get("zeta", 1.0),
            },
        )

        # 4. REFRESCAR LA GUI
        # Como dpg.set_value no dispara el callback del combo, lo llamamos a mano
        self._update_visibility()

    def cb_run_simulation(self):
        # Sincronizamos hardware
        self.ctrl.set_hardware(
            u_min=dpg.get_value("in_umin"),
            u_max=dpg.get_value("in_umax"),
            ts=dpg.get_value("in_ts"),
            u_offset=dpg.get_value("in_uoffset"),
        )

        # Sincronizamos modelo de planta (pasando zeta también)
        self.ctrl.set_model(
            dpg.get_value("sel_model_type"),
            {
                "K": dpg.get_value("in_k"),
                "tau": dpg.get_value("in_tau"),
                "theta": dpg.get_value("in_theta"),
                "zeta": dpg.get_value("in_zeta"),
            },
        )

        # Tuneo y Simulación
        pid = self.ctrl.tune_simc(dpg.get_value("in_aggr"))
        dpg.set_value("res_kp", f"Kp: {pid['Kp']:.4f}")
        dpg.set_value("res_ki", f"Ki: {pid['Ki']:.4f}")
        dpg.set_value("res_kd", f"Kd: {pid['Kd']:.4f}")

        t, pv, op = self.ctrl.simulate(
            dpg.get_value("in_sp"), duration=dpg.get_value("in_dur")
        )

        # Actualizar Plots
        dpg.set_value("plot_pv", [list(t), list(pv)])
        dpg.set_value("plot_sp", [list(t), [dpg.get_value("in_sp")] * len(t)])
        dpg.set_value("plot_op", [list(t), list(op)])
