import dearpygui.dearpygui as dpg


class SettingsTab:
    def __init__(self, commander):
        self.c = commander

    def create(self, tab_bar):
        with dpg.tab(label="Settings", parent=tab_bar):
            dpg.add_spacer(height=10)
            dpg.add_text("SETTINGS")

            # SECCIÓN 1: Control
            dpg.add_text("Seleccionar Control", color=[100, 150, 255])
            with dpg.group(horizontal=True):
                dpg.add_input_text(
                    label="Control",
                    tag="input_ctrlName",
                    width=100,
                    default_value="altHold",
                )
                dpg.add_button(label="Enviar", callback=self.cb_enviar_control)

            dpg.add_separator()
            # SECCIÓN 2: Hover
            dpg.add_text("Seleccionar Hover", color=[100, 255, 150])
            with dpg.group(horizontal=True):
                dpg.add_input_int(
                    label="hover", tag="input_hover", width=100, default_value=1540
                )
                dpg.add_button(label="Cargar Hover", callback=self.cb_cargar_hover)

            dpg.add_separator()

            # SECCIÓN 2: PID
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
            # SECCIÓN 3: Muestreo
            dpg.add_text("Seleccionar periodo de muestreo", color=[255, 100, 100])
            with dpg.group(horizontal=True):
                dpg.add_input_text(
                    label="periodo MS",
                    default_value="10",
                    tag="input_periodMS",
                    width=100,
                )
                dpg.add_button(label="Ejecutar", callback=self.cb_enviar_periodoMS)

    def cb_enviar_control(self):
        self.c.process(200, dpg.get_value("input_ctrlName"))

    def cb_cargar_pid(self):
        p, i, d = (
            float(dpg.get_value("input_p")),
            float(dpg.get_value("input_i")),
            float(dpg.get_value("input_d")),
        )
        self.c.process(300, [p, i, d])

    def cb_enviar_periodoMS(self):
        self.c.process(201, int(dpg.get_value("input_periodMS")))

    def cb_cargar_hover(self):
        self.c.process(302, int(dpg.get_value("input_hover")))
