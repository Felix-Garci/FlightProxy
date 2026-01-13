import struct
from typing import Any, List


class Cmd:
    def __init__(self, id: int, in_shape: str, out_shape: str):
        """
        :param in_shape: Formato de entrada (ej: '3f' para 3 floats, 'str' para texto, 'H' para uint16 , vacio para nada '')
        :param out_shape: Formato de salida (mismos códigos)
        """
        self.id = id
        self.in_shape = in_shape
        self.out_shape = out_shape

    def process_in(self, data: Any) -> List[int]:
        """Transforma cualquier dato a una lista de bytes (enteros 0-255)"""
        if self.in_shape == "":
            return []

        if self.in_shape == "str":
            # Caso especial para strings
            return list(data.encode("utf-8"))

        # Para números: usamos little-endian '<'
        fmt = f"<{self.in_shape}"

        try:
            # Si data es una lista/tupla, se desempaqueta con *, si no, se pasa directo
            if isinstance(data, (list, tuple)):
                packed_bytes = struct.pack(fmt, *data)
            else:
                packed_bytes = struct.pack(fmt, data)
            return list(packed_bytes)
        except struct.error as e:
            raise ValueError(f"Error empaquetando datos con forma {self.in_shape}: {e}")

    def process_out(self, payload: List[int]) -> Any:
        """Transforma una lista de bytes de vuelta al tipo original"""
        if self.out_shape == "":
            return []
        raw_bytes = bytes(payload)

        if self.out_shape == "str":
            return raw_bytes.decode("utf-8")

        fmt = f"<{self.out_shape}"
        try:
            unpacked = struct.unpack(fmt, raw_bytes)
            # Si es un solo valor, lo devolvemos directo; si son varios, devolvemos la tupla/lista
            return unpacked[0] if len(unpacked) == 1 else list(unpacked)
        except struct.error as e:
            raise ValueError(
                f"Error desempaquetando payload con forma {self.out_shape}: {e}"
            )
