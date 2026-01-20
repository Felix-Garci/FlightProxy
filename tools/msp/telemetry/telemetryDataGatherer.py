from commands.cmd import cmd

from collections import deque
import time
import threading


class TelemetryDataGatherer:
    def __init__(self, variables, active_cmds, cmds, timeframeS=10, samplesperS=100):
        self.variables_ = variables
        self.active_cmds_ = active_cmds

        self.cmds_: dict[int, cmd] = cmds

        self.timeframeS = timeframeS
        self.samplesperS = samplesperS
        self.max_samples = None

        self.running = False
        self.thread = None

        self.lock = threading.Lock()

        self.update_buffer(timeframeS, samplesperS)

    def start(self):
        # limpiamos
        self.update_buffer()

        if self.thread is None or not self.thread.is_alive():
            self.running = True
            self.thread = threading.Thread(target=self.update_loop, daemon=True)
            self.thread.start()

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join()

    def update_buffer(self, new_timeframeS=None, new_samplesperS=None):
        self.stop()
        with self.lock:
            if new_timeframeS is not None:
                self.timeframeS = new_timeframeS
            if new_samplesperS is not None:
                self.samplesperS = new_samplesperS

            self.max_samples = self.timeframeS * self.samplesperS

            self.buffer = {
                "timestamp": deque(maxlen=self.max_samples),
            }
            for variable in self.variables_:
                self.buffer[variable] = deque(maxlen=self.max_samples)

    def update_loop(self):
        try:
            start_time = time.time()
            periodo = 1 / self.samplesperS
            while self.running:
                t_inicio_ciclo = time.time()

                now = t_inicio_ciclo - start_time

                data = self.consume_cmds()

                with self.lock:
                    self.buffer["timestamp"].append(now)
                    for var in self.variables_:
                        self.buffer[var].append(data[var])

                sleep_time = periodo - (time.time() - t_inicio_ciclo)
                if sleep_time > 0:
                    time.sleep(sleep_time)
        finally:
            self.running = False

    def consume_cmds(self):
        """
        llamamos a todos los comandos activos
        mapeamos los valores recividos con nuestra
        self.variables_
        """
        data = {name: 0 for name in self.variables_}

        for id_active_cmd in self.active_cmds_:
            active_cmd = self.cmds_[id_active_cmd]
            response = active_cmd.get()
            if response is not None:
                for name in list(response.keys()):
                    index = str(id_active_cmd) + "_" + name
                    if index in list(data.keys()):
                        data[index] = response[name]
            else:
                self.running = False
        return data

    def get_data(self, names):
        """
        names : [ str ] , donde cada str es un key de los nombres
        Timestamp va por defecto y no se espera en names
        Si names va bacio recives todo
        """
        with self.lock:
            # hacemos copia de la lista names
            if not names:
                query_names = list(self.buffer.keys())
            else:
                query_names = list(names)

            # miramos si alguno no existe en nuestro buffer
            for name in query_names:
                if name not in self.buffer:
                    raise KeyError(f"el nombre '{name}' no esta en el buffer")

            # ponemos timestamp siempre al principo
            if "timestamp" in query_names:
                query_names.remove("timestamp")
            query_names.insert(0, "timestamp")

        return (list(self.buffer[i]) for i in query_names)
