import numpy as np


class ControlDesign:
    def __init__(self):
        # Parámetros de la planta (Gemelo Digital)
        self.model_type = "FOPDT"
        self.K = 1.0
        self.tau = 1.0
        self.theta = 0.0
        self.ts = 0.1

        # Parámetros del controlador PID
        self.kp = 0.0
        self.ki = 0.0
        self.kd = 0.0

        # Límites de Hardware
        self.u_min = 0.0
        self.u_max = 0.0
        self.u_offset = 0.0

    def set_model(self, model_type, params):
        self.model_type = model_type
        self.K = params.get("K", 1.0)
        self.tau = params.get("tau", 1.0)
        self.theta = params.get("theta", 0.0)

    def set_hardware(self, u_min, u_max, u_offset, ts):
        self.u_min = u_min
        self.u_max = u_max
        self.u_offset = u_offset
        self.ts = ts

    def tune_simc(self, factor_agresividad=1.0):
        """Aplica las reglas de Skogestad (SIMC)"""
        tau_c = max(self.theta, 0.1 * self.tau) * factor_agresividad
        # Corrección por muestreo digital
        theta_eff = self.theta + (self.ts / 2)

        if self.model_type in ["FOPDT", "FO_SIMPLE"]:
            self.kp = (1 / self.K) * (self.tau / (tau_c + theta_eff))
            ti = min(self.tau, 4 * (tau_c + theta_eff))
            self.ki = self.kp / ti if ti != 0 else 0
            self.kd = 0

        elif self.model_type == "IPDT":
            self.kp = (1 / self.K) * (1 / (tau_c + theta_eff))
            ti = 4 * (tau_c + theta_eff)
            self.ki = self.kp / ti
            self.kd = 0

        elif self.model_type == "SOPDT":
            # Usamos una aproximación de sistema de segundo orden
            self.kp = (1 / self.K) * (self.tau / (tau_c + theta_eff))
            ti = min(self.tau, 4 * (tau_c + theta_eff))
            self.ki = self.kp / ti
            self.kd = self.kp * (
                self.tau / 2
            )  # Simplificación para la parte derivativa

        return {"Kp": self.kp, "Ki": self.ki, "Kd": self.kd}

    def simulate(self, setpoint, duration):
        """Simulación temporal paso a paso con saturación y tiempo muerto"""
        steps = int(duration / self.ts)
        t_axis = np.linspace(0, duration, steps)

        pv_plot = np.zeros(steps)
        op_plot = np.zeros(steps)

        # Buffer para el tiempo muerto (Delay Line)
        delay_steps = int(self.theta / self.ts)
        u_buffer = [self.u_offset] * (delay_steps + 1)

        # Estados internos del PID
        integral = 0.0
        last_error = 0.0

        # Estados internos de la Planta (Discretización)
        curr_pv = 0.0
        # Coeficientes para FOPDT: y[k] = a*y[k-1] + b*u[k-d-1]
        a = np.exp(-self.ts / max(self.tau, 0.001))
        b = self.K * (1 - a)

        for k in range(steps):
            error = setpoint - curr_pv

            # --- CALCULOS DEL PID ---
            p_term = self.kp * error
            integral += error * self.ts
            i_term = self.ki * integral
            d_term = self.kd * (error - last_error) / self.ts

            u_raw = self.u_offset + (p_term + i_term + d_term)

            # Saturación y Anti-Windup (si se satura, dejamos de integrar)
            u_saturada = np.clip(u_raw, self.u_min, self.u_max)
            if u_raw != u_saturada:
                integral -= error * self.ts  # Deshacemos la última integral

            # --- MANEJO DEL TIEMPO MUERTO ---
            u_buffer.append(u_saturada)
            u_to_plant = u_buffer.pop(0)

            # la planta solo responde a lo que sube de 1000
            u_fisica = u_to_plant - self.u_offset

            # --- RESPUESTA DE LA PLANTA ---
            if self.model_type == "IPDT":
                # Integrador puro
                curr_pv += self.K * u_fisica * self.ts
            elif self.model_type == "SOPDT":
                # Segundo orden simplificado (dos lags de primer orden)
                # Por simplicidad aquí usamos una aproximación rápida
                curr_pv = a * curr_pv + b * u_fisica
            else:
                # FOPDT y FO_SIMPLE
                curr_pv = a * curr_pv + b * u_fisica

            # Guardar para el gráfico
            pv_plot[k] = curr_pv
            op_plot[k] = u_saturada
            last_error = error

        return t_axis, pv_plot, op_plot
