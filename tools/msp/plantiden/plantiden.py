from plantiden.solver import SystemSolver
import pandas as pd
import numpy as np
from scipy.optimize import curve_fit


class PlantIden:
    def __init__(self):
        self.df = None
        self.col_t = None
        self.col_u = None
        self.col_y = None

        self.t_start = None
        self.t_end = None

        self.solver = SystemSolver()

        self.results = {}

    def load_csv(self, file_path, sep=","):
        """Carga los datos desde un archivo CSV."""
        try:
            self.df = pd.read_csv(file_path, sep=sep)
            return True, f"Cargado: {len(self.df)} filas."
        except Exception as e:
            return False, str(e)

    def set_columns(self, t_col, u_col, y_col):
        """Define qué columnas corresponden a cada variable."""
        if self.df is not None:
            if all(col in self.df.columns for col in [t_col, u_col, y_col]):
                self.col_t = t_col
                self.col_u = u_col
                self.col_y = y_col
                return True
        return False

    def set_region(self, t_start, t_end):
        """Selecciona el rango de tiempo para el análisis."""
        self.t_start = t_start
        self.t_end = t_end

    def get_full_data(self):
        """Retorna los datos completos para el plot inicial de DPG."""
        if self.df is not None:
            return (
                self.df[self.col_t].values,
                self.df[self.col_u].values,
                self.df[self.col_y].values,
            )
        return None, None, None

    def run_identification(self, model_type="FOPDT"):
        if self.df is None:
            return False, "No hay datos"

        mask = (self.df[self.col_t] >= self.t_start) & (
            self.df[self.col_t] <= self.t_end
        )
        t_data = self.df.loc[mask, self.col_t].values
        u_data = self.df.loc[mask, self.col_u].values
        y_data = self.df.loc[mask, self.col_y].values

        diff_u = np.abs(np.diff(u_data))
        idx_step = np.argmax(diff_u) + 1  # +1 porque diff acorta el array en 1
        t_step = t_data[idx_step]

        y0_mejorado = np.mean(y_data[:idx_step]) if idx_step > 0 else y_data[0]

        self.solver.prepare_data(t_data, u_data, y_data, y0_mejorado, t_step)

        try:
            if model_type == "FOPDT":
                # K, tau, theta
                popt = self.solver.fit_fopdt()
                y_pred = self.solver.fopdt_model(
                    t_data, *popt, self.solver.y0, self.solver.t0, self.solver.delta_u
                )
                params = {"K": popt[0], "tau": popt[1], "theta": popt[2]}

            elif model_type == "SOPDT":
                # K, tau, zeta, theta
                popt = self.solver.fit_sopdt()
                y_pred = self.solver.sopdt_model(
                    t_data, *popt, self.solver.y0, self.solver.t0, self.solver.delta_u
                )
                params = {
                    "K": popt[0],
                    "tau": popt[1],
                    "zeta": popt[2],
                    "theta": popt[3],
                }

            elif model_type == "IPDT":
                # Integrador: K, theta
                # Guess inicial para K es la pendiente de la rampa
                k_guess = (y_data[-1] - y_data[0]) / (
                    self.solver.delta_u * (t_data[-1] - t_data[0])
                )
                func = lambda t, K, theta: self.solver.ipdt_model(
                    t, K, theta, self.solver.y0, self.solver.t0, self.solver.delta_u
                )
                popt, _ = curve_fit(
                    func, t_data, y_data, p0=[k_guess, 0.1], bounds=(0, np.inf)
                )
                y_pred = func(t_data, *popt)
                params = {"K": popt[0], "theta": popt[1]}

            elif model_type == "FO_SIMPLE":
                # Primer orden SIN retraso (theta = 0 fijo)
                func = lambda t, K, tau: self.solver.fopdt_model(
                    t, K, tau, 0, self.solver.y0, self.solver.t0, self.solver.delta_u
                )
                popt, _ = curve_fit(
                    func, t_data, y_data, p0=[1.0, 1.0], bounds=(0, np.inf)
                )
                y_pred = func(t_data, *popt)
                params = {"K": popt[0], "tau": popt[1], "theta": 0}

            self.results = {
                "model": model_type,
                "params": params,
                "r2": self.solver.calculate_r2(y_data, y_pred),
                "t_pred": t_data,
                "y_pred": y_pred,
            }
            return True, self.results

        except Exception as e:
            return False, f"Error: {str(e)}"

    def get_results_summary(self):
        """Devuelve un string formateado con los resultados para mostrar en la UI."""
        if not self.results:
            return "No hay resultados."

        p = self.results["params"]
        summary = f"Modelo: {self.results['model']}\n"
        summary += "\n".join([f"{k}: {v:.4f}" for k, v in p.items()])
        summary += f"\nR2: {self.results['r2']:.4f}"
        return summary
