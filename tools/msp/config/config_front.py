import dearpygui.dearpygui as dpg


class SettingsTab:
    def __init__(self, commander):
        # nivel de control
        self.set_ctrllvl = commander.get_cmd(19)
        self.commander = commander

        # todos tienen [p i d offset]
        self.map_pids = {}
        # for pids in [20, 22, 24, 26, 40, 42, 44]:
        #    self.map_pids[pids] = commander.get_cmd(pids)

        self.pid_setup(24, [0.4, 0.2, 0, 0.54], "vertical velocity")
        self.pid_setup(42, [0.5, 0, 0, 0], "vertical position")

        self.pid_setup(22, [0, 0, 0, 0], "frontal velocity")

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
            for pid in list(self.map_pids.keys()):
                self.pid_form(
                    pid, self.map_pids[pid]["default"], self.map_pids[pid]["name"]
                )

    def cb_enviar_control(self):
        selected_ctrl = dpg.get_value("input_ctrlName")
        self.set_ctrllvl.set(selected_ctrl)

    def cb_cargar_pid(self, sender, app_data, user_data):
        p, i, d, offset = (
            float(dpg.get_value(f"input_p{user_data}")),
            float(dpg.get_value(f"input_i{user_data}")),
            float(dpg.get_value(f"input_d{user_data}")),
            float(dpg.get_value(f"input_offset{user_data}")),
        )
        ret = [p, i, d, offset]

        self.map_pids[user_data]["commander"].set(ret)

    def pid_setup(self, control_code, default_values, name):
        self.map_pids[control_code] = {
            "commander": self.commander.get_cmd(control_code),
            "default": default_values,
            "name": name,
        }

    def pid_form(self, control_code, default_values: dict, name=""):
        """
        ej:
        control_code = 32
        default_values = [4 , 3 , 1 , 0]
                          p , i , d , offset
        """
        labels = ["p", "i", "d", "offset"]

        if name == "":
            name = str(control_code)

        dpg.add_text(f"Ajuste de Constantes PID [{name}]", color=[100, 255, 150])

        with dpg.group(horizontal=True):
            for i in range(len(default_values)):
                dpg.add_input_text(
                    label=labels[i],
                    tag=f"input_{labels[i]}{control_code}",
                    width=60,
                    default_value=str(default_values[i]),
                )
                dpg.add_button(
                    label="Cargar PID",
                    callback=self.cb_cargar_pid,
                    user_data=control_code,
                )

        dpg.add_separator()
