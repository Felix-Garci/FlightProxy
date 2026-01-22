import dearpygui.dearpygui as dpg


class SettingsTab:
    def __init__(self, commander):
        self.set_ctrllvl = commander.get_cmd(19)
        # todos tienen [p i d]
        self.map_pids = {}
        self.pid_vertvel = 24
        for pids in [20, 22, 24, 26]:
            self.map_pids[pids] = commander.get_cmd(pids)

    def create(self, tab_bar):
        with dpg.tab(label="Settings", parent=tab_bar):
            dpg.add_spacer(height=10)
            dpg.add_text("SETTINGS")

            # SECCIÓN 1: Control
            dpg.add_text("Seleccionar Nivel de Control", color=[100, 150, 255])
            with dpg.group(horizontal=True):
                dpg.add_input_int(
                    label="Control",
                    tag="input_ctrlName",
                    width=100,
                    default_value=2,
                )
                dpg.add_button(label="Enviar", callback=self.cb_enviar_control)

            dpg.add_separator()

            # SECCIÓN 2: PID
            for pids in list(self.map_pids.keys()):
                dpg.add_text(
                    f"Ajuste de Constantes PID [{pids}]", color=[100, 255, 150]
                )
                with dpg.group(horizontal=True):
                    dpg.add_input_text(
                        label="P", tag=f"input_p{pids}", width=60, default_value="1.0"
                    )
                    dpg.add_input_text(
                        label="I", tag=f"input_i{pids}", width=60, default_value="0.1"
                    )
                    dpg.add_input_text(
                        label="D", tag=f"input_d{pids}", width=60, default_value="0.05"
                    )
                    if pids == self.pid_vertvel:
                        dpg.add_input_text(
                            label="Throttle",
                            tag="input_throttle",
                            width=60,
                            default_value="1540",
                        )

                    dpg.add_button(
                        label="Cargar PID", callback=self.cb_cargar_pid, user_data=pids
                    )

                dpg.add_separator()

    def cb_enviar_control(self):
        selected_ctrl = dpg.get_value("input_ctrlName")
        self.set_ctrllvl.set(selected_ctrl)

    def cb_cargar_pid(self, sender, app_data, user_data):
        p, i, d = (
            float(dpg.get_value(f"input_p{user_data}")),
            float(dpg.get_value(f"input_i{user_data}")),
            float(dpg.get_value(f"input_d{user_data}")),
        )
        ret = [p, i, d]
        if user_data == self.pid_vertvel:
            throttle = float(dpg.get_value("input_throttle"))
            ret.append(throttle)

        self.map_pids[user_data].set(ret)
