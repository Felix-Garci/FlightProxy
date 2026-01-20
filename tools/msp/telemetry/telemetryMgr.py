from commands.cmd import cmd
from telemetry.telemetryDataGatherer import TelemetryDataGatherer

import copy
import threading


class TelemetryMgr:
    def __init__(self):
        self.cmds: dict[int, cmd] = {}

        # active_var : str = id+_+var_name
        self.active_vars: dict[str, int] = {}

        # id de los comandos activos
        self.active_cmds: list[int] = []

        self.dataGathrer = TelemetryDataGatherer(
            self.active_vars, self.active_cmds, self.cmds
        )

    def add_cmd(self, comando: cmd):
        self.cmds[comando.id_] = comando

    def get_menu(self):
        menu = {}

        if not len(self.cmds.keys()) or not list(self.cmds.values())[0].cmd_on:
            return menu

        for comando in list(self.cmds.values()):
            cmd_key = f"[{comando.id_}] {comando.get_signature()}"
            menu[cmd_key] = [
                str(comando.id_) + "_" + var_name for var_name in comando.get_names()
            ]

        return menu

    def add_variables(self, variables: list[str]):
        self.stop()
        active_vars_copy = copy.deepcopy(self.active_vars)
        active_cmds_copy = copy.deepcopy(self.active_cmds)
        for var in variables:
            if var in list(active_vars_copy.keys()):
                active_vars_copy[var] += 1
            else:
                active_vars_copy[var] = 1
                command_id = int(var.split("_")[0])
                if command_id not in self.active_cmds:
                    active_cmds_copy.append(command_id)

        with self.dataGathrer.lock:
            self.active_vars.clear()
            self.active_vars.update(active_vars_copy)

            self.active_cmds.clear()
            self.active_cmds.extend(active_cmds_copy)

    def remove_variables(self, variables: list[str]):
        self.stop()
        active_vars_copy = copy.deepcopy(self.active_vars)
        active_cmds_copy = copy.deepcopy(self.active_cmds)
        for var in variables:
            if var in list(active_vars_copy.keys()):
                if active_vars_copy[var] > 1:
                    active_vars_copy[var] -= 1
                else:
                    active_vars_copy.pop(var)
                    command_id = int(var.split("_")[0])
                    if (
                        command_id
                        not in [
                            var_.split("_")[0] for var_ in list(active_vars_copy.keys())
                        ]
                        and command_id in active_cmds_copy
                    ):
                        active_cmds_copy.remove(command_id)

        with self.dataGathrer.lock:
            self.active_vars.clear()
            self.active_vars.update(active_vars_copy)

            self.active_cmds.clear()
            self.active_cmds.extend(active_cmds_copy)

    def get_data(self, variables: list[str]):
        return self.dataGathrer.get_data(variables)

    def is_running(self):
        return self.dataGathrer.running

    def start(self):
        return self.dataGathrer.start()

    def stop(self):
        return self.dataGathrer.stop()

    def update_buffer_size(self, timeframeS: int, samplesperS: int):
        return self.dataGathrer.update_buffer(timeframeS, samplesperS)

    def get_timeframeS(self):
        return self.dataGathrer.timeframeS

    def get_samplesperS(self):
        return self.dataGathrer.samplesperS

    def get_active_vars(self) -> list[str]:
        return list(self.active_vars.keys())
