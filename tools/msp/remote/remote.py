import time
import threading


class Remote:
    def __init__(self, rcSetter, periodMS=10, default_cn=None):
        self.rcSetter = rcSetter
        self.default_cn = list(default_cn) if default_cn else [1500, 1500, 1000, 1500]
        self.currentval_rpty = list(self.default_cn)
        self.armed = False
        self.periodS = periodMS / 1000.0
        self.running = False
        self.thread = None
        self.lock = threading.Lock()

        self.sequence_running = False
        self.abort_sequence = False

    def start(self):
        if self.thread is None or not self.thread.is_alive():
            self.running = True
            self.thread = threading.Thread(target=self.update_loop, daemon=True)
            self.thread.start()

    def stop(self):
        self.unarm()  # Desarmar primero para limpiar canales
        self.running = False
        if self.thread:
            self.thread.join()

    def arm(self):
        if self.running:
            self.armed = True

    def unarm(self):
        """Resetea valores a default y detiene tests para enviar paquete de seguridad"""
        self.armed = False

        with self.lock:
            self.currentval_rpty = list(self.default_cn)

        self.stop_sequence()

    def update_channel(self, idx, val):
        with self.lock:
            self.currentval_rpty[idx] = val

    def get_actual_rc(self):
        with self.lock:
            data = list(self.currentval_rpty)
        data.append(2000 if self.armed else 1000)
        data.append(1500)
        return data

    def update_loop(self):
        while self.running:
            start_time = time.perf_counter()

            result = self.rcSetter(self.get_actual_rc())
            if result is None:
                self.running = False
                self.armed = False
                self.stop_sequence()
                break

            elapsed = time.perf_counter() - start_time
            time.sleep(max(0, self.periodS - elapsed))

    def play_step(
        self,
        channel_idx,
        start_val,
        duration_1,
        end_val,
        duration_2,
        repeats=1,
        on_finished=None,
    ):
        if not self.armed:
            return

        def wait_interruptible(seconds):
            """Espera en intervalos cortos para poder cancelar el test al instante"""
            steps = int(seconds / 0.1)
            for _ in range(steps):
                if self.abort_sequence or not self.armed or not self.running:
                    return True
                time.sleep(0.1)
            return False

        def seq():
            self.sequence_running = True
            self.abort_sequence = False
            try:
                for _ in range(repeats):
                    if self.abort_sequence or not self.armed or not self.running:
                        break

                    self.update_channel(channel_idx, start_val)
                    if wait_interruptible(duration_1):
                        break

                    self.update_channel(channel_idx, end_val)
                    if wait_interruptible(duration_2):
                        break
            finally:
                with self.lock:
                    self.currentval_rpty = list(self.default_cn)
                self.sequence_running = False
                if on_finished:
                    on_finished()

        threading.Thread(target=seq, daemon=True).start()

    def play_staircase(
        self,
        channel_idx,
        start_val,
        end_val,
        step_increment,
        duration_per_step,
        on_finished=None,
    ):
        if not self.armed:
            return

        def wait_interruptible(seconds):
            steps = int(seconds / 0.1)
            for _ in range(steps):
                if self.abort_sequence or not self.armed or not self.running:
                    return True
                time.sleep(0.1)
            return False

        def seq():
            self.sequence_running = True
            self.abort_sequence = False
            try:
                # 1. Generar la escalera de ida
                # Usamos un signo basado en si subimos o bajamos
                sign = 1 if end_val > start_val else -1
                steps_up = list(range(start_val, end_val + sign, step_increment * sign))

                # 2. Generar la escalera de vuelta (invertida)
                # Excluimos el último valor para no repetirlo y el primero para no repetir el inicio al terminar
                steps_down = steps_up[:-1][::-1]

                full_sequence = steps_up + steps_down

                for val in full_sequence:
                    if self.abort_sequence or not self.armed or not self.running:
                        break

                    self.update_channel(channel_idx, val)
                    # Esperamos el tiempo definido para que la velocidad se estabilice en ese nivel
                    if wait_interruptible(duration_per_step):
                        break

            finally:
                with self.lock:
                    # Volvemos a los valores por defecto (ej. throttle al mínimo)
                    self.currentval_rpty = list(self.default_cn)
                self.sequence_running = False
                if on_finished:
                    on_finished()

        threading.Thread(target=seq, daemon=True).start()

    def stop_sequence(self):
        self.abort_sequence = True
