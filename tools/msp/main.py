import dearpygui.dearpygui as dpg
from commander import init as getc
from telemetry.telemetryManager import TelemetryManager
from remote.remote import Remote
from utils.bus import SignalBus

# Imports de Pestañas
from config.config_front import SettingsTab
from telemetry.telemetry_front import TelemetryTab
from remote.remote_front import RemoteTab
from ctrldesign.ctrldesign_front import DesignTab
from plantiden.plantiden_front import IdenTab


class DroneApp:
    def __init__(self, ip, port):
        dpg.create_context()
        self.bus = SignalBus()

        # Managers
        self.c = getc(ip, port, self.mostrar_desconectado)

        # self.avalible_vars = ["throttle", "altura", "velocidad"]
        # self.telemetry = TelemetryManager(
        #    lambda: self.c.process(250), self.avalible_vars
        # )
        self.avalible_vars = ["reference", "actual", "throttle", "p", "i", "d"]
        self.telemetry = TelemetryManager(
            lambda: self.c.process(301), self.avalible_vars
        )

        self.remote = Remote(lambda data: self.c.process(270, data))

        # Instanciar Pestañas
        self.tab_settings = SettingsTab(self.c)
        self.tab_telemetry = TelemetryTab(self.bus, self.telemetry, self.avalible_vars)
        self.tab_remote = RemoteTab(self.remote)
        self.tab_design = DesignTab(self.bus)
        self.tab_iden = IdenTab(self.bus)

        self._setup_ui()

        dpg.create_viewport(title="Drone Lab Suite v1.2", width=1200, height=900)
        dpg.setup_dearpygui()
        dpg.show_viewport()

    def mostrar_desconectado(self):
        dpg.set_value("status_text", "DESCONECTADO")
        dpg.configure_item("status_text", color=[255, 0, 0])

    def cb_conectar(self):
        if self.c.client.connect():
            dpg.set_value("status_text", "CONECTADO")
            dpg.configure_item("status_text", color=[0, 255, 0])

    def _setup_ui(self):
        with dpg.window(label="Main Window", tag="PrimaryWindow"):
            with dpg.group(horizontal=True):
                dpg.add_text("Estado:")
                dpg.add_text("DESCONECTADO", tag="status_text", color=[255, 0, 0])
                dpg.add_button(label="Conectar", callback=self.cb_conectar)

            with dpg.tab_bar() as main_bar:
                self.tab_settings.create(main_bar)
                self.tab_remote.create(main_bar)
                self.tab_telemetry.create(main_bar)
                self.tab_iden.create(main_bar)
                self.tab_design.create(main_bar)

    def run(self):
        while dpg.is_dearpygui_running():
            self.tab_telemetry.update_plots()
            self.tab_remote.update_ui_state()
            dpg.render_dearpygui_frame()
        dpg.destroy_context()


if __name__ == "__main__":
    app = DroneApp("localhost", 12345)
    dpg.set_primary_window("PrimaryWindow", True)
    app.run()
