import dearpygui.dearpygui as dpg


class RemoteTab:
    def __init__(self, remote_manager):
        self.remote = remote_manager
        self.btn_run_tag = "btn_start_stop"
        self.btn_arm_tag = "btn_arm_disarm"
        self.btn_step_tag = "btn_step_test"
        self.btn_ramp_tag = "btn_ramp_test"  # Nuevo Tag para el test de escalera

    def create(self, tab_bar):
        with dpg.tab(label="Remote", parent=tab_bar):
            dpg.add_button(
                label="START SYSTEM",
                tag=self.btn_run_tag,
                callback=self.toggle_running,
                width=200,
            )

            dpg.add_button(
                label="ARM", tag=self.btn_arm_tag, callback=self.toggle_arm, width=200
            )

            with dpg.group(horizontal=True):
                with dpg.group():
                    dpg.add_slider_float(
                        tag="slider_1",
                        callback=self.update_remote_values,
                        min_value=1000,
                        max_value=2000,
                        default_value=1500,
                        vertical=True,
                        height=200,
                        width=50,
                    )
                    dpg.add_text("roll")

                with dpg.group():
                    dpg.add_slider_float(
                        tag="slider_2",
                        callback=self.update_remote_values,
                        min_value=1000,
                        max_value=2000,
                        default_value=1500,
                        vertical=True,
                        height=200,
                        width=50,
                    )
                    dpg.add_text("pitch")

                with dpg.group():
                    dpg.add_slider_float(
                        tag="slider_3",
                        callback=self.update_remote_values,
                        min_value=1000,
                        max_value=2000,
                        default_value=1500,
                        vertical=True,
                        height=200,
                        width=50,
                    )
                    dpg.add_text("throttle")
                with dpg.group():
                    dpg.add_slider_float(
                        tag="slider_4",
                        callback=self.update_remote_values,
                        min_value=1000,
                        max_value=2000,
                        default_value=1500,
                        vertical=True,
                        height=200,
                        width=50,
                    )
                    dpg.add_text("yaw")

            # --- SECCIÓN: STEP TEST ---
            dpg.add_spacer(height=10)
            dpg.add_text("STEP TEST CONFIG", color=(100, 200, 255))
            dpg.add_separator()
            dpg.add_input_int(
                label="Channel##Step", tag="input_channel", default_value=2
            )
            dpg.add_input_int(
                label="StartVal##Step", tag="input_startVal", default_value=1400
            )
            dpg.add_input_int(
                label="Duration 1 (s)", tag="input_durationS_1", default_value=2
            )
            dpg.add_input_int(
                label="EndVal##Step", tag="input_endVal", default_value=2000
            )
            dpg.add_input_int(
                label="Duration 2 (s)", tag="input_durationS_2", default_value=2
            )
            dpg.add_input_int(label="Repeats", tag="input_repeats", default_value=1)

            dpg.add_button(
                label="START STEP TEST",
                tag=self.btn_step_tag,
                callback=self.toggle_step_test,
                width=200,
            )

            # --- SECCIÓN NUEVA: STAIRCASE / RAMP TEST ---
            dpg.add_spacer(height=20)
            dpg.add_text("STAIRCASE TEST (HOVER FINDER)", color=(100, 255, 150))
            dpg.add_separator()
            dpg.add_input_int(
                label="Channel##Ramp", tag="input_ramp_channel", default_value=2
            )
            dpg.add_input_int(
                label="Min Val", tag="input_ramp_start", default_value=1300
            )
            dpg.add_input_int(label="Max Val", tag="input_ramp_end", default_value=1700)
            dpg.add_input_int(
                label="Step Increment", tag="input_ramp_step", default_value=20
            )
            dpg.add_input_float(
                label="Secs per Step", tag="input_ramp_duration", default_value=2.0
            )

            dpg.add_button(
                label="START RAMP TEST",
                tag=self.btn_ramp_tag,
                callback=self.toggle_ramp_test,
                width=200,
            )

    def update_ui_state(self):
        """Sincroniza visualmente ambos botones de test con el estado global de secuencia"""
        # sliders.
        mis_sliders = ["slider_1", "slider_2", "slider_3", "slider_4"]
        ref_values = [1500, 1500, 1500, 1500]

        for i, slider_id in enumerate(mis_sliders):
            if not dpg.is_item_active(slider_id):
                if dpg.get_value(slider_id) != ref_values[i]:
                    dpg.set_value(slider_id, ref_values[i])
                    self.update_remote_values(slider_id, ref_values[i])

        # 1. Sistema corriendo (Start/Stop)
        if self.remote.running:
            dpg.configure_item(self.btn_run_tag, label="STOP SYSTEM")
            dpg.bind_item_theme(self.btn_run_tag, self._get_theme((200, 50, 50)))
        else:
            dpg.configure_item(self.btn_run_tag, label="START SYSTEM")
            dpg.bind_item_theme(self.btn_run_tag, 0)

        # 2. Sistema armado (Arm/Disarm)
        if self.remote.armed:
            dpg.configure_item(self.btn_arm_tag, label="DISARM")
            dpg.bind_item_theme(self.btn_arm_tag, self._get_theme((255, 100, 0)))
        else:
            dpg.configure_item(self.btn_arm_tag, label="ARM")
            dpg.bind_item_theme(self.btn_arm_tag, 0)

        # 3. Lógica Sincronizada de Tests
        # Si CUALQUIER secuencia está activa, ambos botones se ponen en modo STOP
        if self.remote.sequence_running:
            theme_stop = self._get_theme((200, 50, 50))

            dpg.configure_item(self.btn_step_tag, label="STOP STEP TEST")
            dpg.bind_item_theme(self.btn_step_tag, theme_stop)

            dpg.configure_item(self.btn_ramp_tag, label="STOP RAMP TEST")
            dpg.bind_item_theme(self.btn_ramp_tag, theme_stop)
        else:
            # Si no hay nada corriendo, ambos vuelven a su estado original
            dpg.configure_item(self.btn_step_tag, label="START STEP TEST")
            dpg.bind_item_theme(self.btn_step_tag, 0)

            dpg.configure_item(self.btn_ramp_tag, label="START RAMP TEST")
            dpg.bind_item_theme(self.btn_ramp_tag, 0)

    def toggle_running(self):
        if not self.remote.running:
            self.remote.start()
        else:
            self.remote.stop()
        self.update_ui_state()

    def toggle_arm(self):
        if not self.remote.running:
            return
        if not self.remote.armed:
            self.remote.arm()
        else:
            self.remote.unarm()
        self.update_ui_state()

    def update_remote_values(self, sender, app_data):
        channel = int(sender.split("_")[1]) - 1
        value = int(app_data)
        self.remote.update_channel(channel, value)

    def toggle_step_test(self):
        if not self.remote.armed:
            return
        if not self.remote.sequence_running:
            self.remote.play_step(
                dpg.get_value("input_channel"),
                dpg.get_value("input_startVal"),
                dpg.get_value("input_durationS_1"),
                dpg.get_value("input_endVal"),
                dpg.get_value("input_durationS_2"),
                dpg.get_value("input_repeats"),
                on_finished=self.update_ui_state,
            )
        else:
            self.remote.stop_sequence()
        self.update_ui_state()

    def toggle_ramp_test(self):
        """Lógica para iniciar/detener el test de escalera"""
        if not self.remote.armed:
            return

        if not self.remote.sequence_running:
            self.remote.play_staircase(
                channel_idx=dpg.get_value("input_ramp_channel"),
                start_val=dpg.get_value("input_ramp_start"),
                end_val=dpg.get_value("input_ramp_end"),
                step_increment=dpg.get_value("input_ramp_step"),
                duration_per_step=dpg.get_value("input_ramp_duration"),
                on_finished=self.update_ui_state,
            )
        else:
            self.remote.stop_sequence()

        self.update_ui_state()

    def _get_theme(self, color):
        with dpg.theme() as theme:
            with dpg.theme_component(dpg.mvButton):
                dpg.add_theme_color(dpg.mvThemeCol_Button, color)
        return theme
