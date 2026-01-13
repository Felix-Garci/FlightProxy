import numpy as np
from scipy.optimize import curve_fit


class SystemSolver:
    def __init__(self):
        # Datos recortados y listos para procesar
        self.t = None
        self.u = None
        self.y = None
        self.delta_u = 0  # Amplitud del escalón
        self.y0 = 0  # Valor inicial de la salida (bias)
        self.t0 = 0  # Tiempo donde ocurre el escalón

    def prepare_data(self, t, u, y, y0, t_start):
        """
        Prepara los datos restando el offset inicial para que el ajuste sea más preciso.
        t_start: el tiempo exacto donde el usuario detecta que empieza el escalón.
        """
        # Filtramos o normalizamos si es necesario
        self.t = np.array(t)
        self.u = np.array(u)
        self.y = np.array(y)

        # Parámetros base para el modelo
        self.y0 = y0
        self.t0 = t_start
        # Asumimos que el escalón es la diferencia entre el valor final e inicial de u
        self.delta_u = self.u[-1] - self.u[0]

    @staticmethod
    def ipdt_model(t, K, theta, y0, t0, delta_u):
        """Modelo Integrador + Tiempo Muerto: Y(s)/U(s) = K/s * e^(-theta*s)"""
        t_adj = t - t0 - theta
        res = np.full_like(t, y0)
        mask = t_adj > 0
        # La respuesta al escalón de un integrador es una rampa
        res[mask] = y0 + K * delta_u * t_adj[mask]
        return res

    @staticmethod
    def fopdt_model(t, K, tau, theta, y0, t0, delta_u):
        """Modelo de Primer Orden + Tiempo Muerto"""
        # t0 es cuando ocurre el cambio en la entrada
        # theta es el retraso adicional del sistema
        res = y0 + K * delta_u * (1 - np.exp(-np.maximum(0, t - t0 - theta) / tau))
        return np.where(t < (t0 + theta), y0, res)

    @staticmethod
    def sopdt_model(t, K, tau, zeta, theta, y0, t0, delta_u):
        """Modelo de Segundo Orden + Tiempo Muerto (Subamortiguado/Sobreamortiguado)"""
        t_adj = t - t0 - theta
        res = np.full_like(t, y0)

        mask = t_adj > 0
        ta = t_adj[mask]

        if zeta < 1:  # Subamortiguado
            wd = (np.sqrt(1 - zeta**2)) / tau
            val = 1 - np.exp(-zeta * ta / tau) * (
                np.cos(wd * ta) + (zeta / np.sqrt(1 - zeta**2)) * np.sin(wd * ta)
            )
        elif zeta == 1:  # Amortiguamiento crítico
            val = 1 - (1 + ta / tau) * np.exp(-ta / tau)
        else:  # Sobreamortiguado
            r1 = (-zeta + np.sqrt(zeta**2 - 1)) / tau
            r2 = (-zeta - np.sqrt(zeta**2 - 1)) / tau
            val = 1 - (r2 * np.exp(r1 * ta) - r1 * np.exp(r2 * ta)) / (r2 - r1)

        res[mask] = y0 + K * delta_u * val
        return res

    def fit_fopdt(self):
        """Ejecuta el ajuste para Primer Orden"""
        # Guess inicial [K, tau, theta]
        # K aprox = (y_final - y_inicial) / delta_u
        k_guess = (self.y[-1] - self.y0) / self.delta_u
        p0 = [k_guess, (self.t[-1] - self.t0) / 3, 0.1]

        # Definimos una función lambda para pasar los parámetros fijos
        func = lambda t, K, tau, theta: self.fopdt_model(
            t, K, tau, theta, self.y0, self.t0, self.delta_u
        )

        popt, _ = curve_fit(
            func,
            self.t,
            self.y,
            p0=p0,
            bounds=([-np.inf, 0.001, 0], [np.inf, np.inf, np.inf]),
        )
        return popt  # [K, tau, theta]

    def fit_sopdt(self):
        """Ejecuta el ajuste para Segundo Orden"""
        k_guess = (self.y[-1] - self.y0) / self.delta_u
        p0 = [k_guess, (self.t[-1] - self.t0) / 3, 0.7, 0.1]  # [K, tau, zeta, theta]

        func = lambda t, K, tau, zeta, theta: self.sopdt_model(
            t, K, tau, zeta, theta, self.y0, self.t0, self.delta_u
        )

        popt, _ = curve_fit(
            func,
            self.t,
            self.y,
            p0=p0,
            bounds=([-np.inf, 0.001, 0.001, 0], [np.inf, np.inf, 5.0, np.inf]),
        )
        return popt  # [K, tau, zeta, theta]

    def calculate_r2(self, y_true, y_pred):
        residuals = y_true - y_pred
        ss_res = np.sum(residuals**2)
        ss_tot = np.sum((y_true - np.mean(y_true)) ** 2)
        return 1 - (ss_res / ss_tot)
