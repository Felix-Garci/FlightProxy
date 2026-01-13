from collections import deque
import time
import threading


class TelemetryManager:
    def __init__(self, dataGeter, datapts, timeframeS=10, samplesperS=100):
        self.dataGetter = dataGeter
        self.datapts = datapts
        self.timeframeS = timeframeS
        self.samplesperS = samplesperS
        self.max_samples = None

        self.running = False
        self.thread = None

        self.lock = threading.Lock()

        self.update_buffer(timeframeS, samplesperS)

    def start(self):
        # limpiamos
        for elemento in self.buffer:
            self.buffer[elemento].clear()

        if self.thread is None or not self.thread.is_alive():
            self.running = True
            self.thread = threading.Thread(target=self.update_loop, daemon=True)
            self.thread.start()

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join()

    def update_buffer(self, new_timeframeS, new_samplesperS):
        self.stop()
        with self.lock:
            self.timeframeS = new_timeframeS
            self.samplesperS = new_samplesperS

            self.max_samples = self.timeframeS * self.samplesperS

            self.buffer = {
                "timestamp": deque(maxlen=self.max_samples),
            }
            for datapt in self.datapts:
                self.buffer[datapt] = deque(maxlen=self.max_samples)

    def update_loop(self):
        try:
            start_time = time.time()
            while self.running:
                now = time.time() - start_time
                data = self.dataGetter()

                if data is not None and len(data) == len(self.datapts):
                    with self.lock:
                        self.buffer["timestamp"].append(now)
                        for i, datapt in enumerate(self.datapts):
                            self.buffer[datapt].append(data[i])
                else:
                    raise

                time.sleep(1 / self.samplesperS)
        finally:
            self.running = False

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
