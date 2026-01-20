import struct
import re
from typing import Any, Dict, List, Tuple


class FlightProxyParser:
    TYPE_MAP = {
        "i64": "q",
        "i32": "i",
        "i16": "h",
        "i8": "b",
        "u64": "Q",
        "u32": "I",
        "u16": "H",
        "u8": "B",
        "f32": "f",
        "f64": "d",
        "b": "?",
    }

    def __init__(self, signature: str, field_names: str):
        # 1. Extraer flags y limpiar firma (Comando 100+id)
        # Basado en AutoCommandFactory: los 2 últimos chars son flags
        self.can_consume = signature[-2] == "1"
        self.can_produce = signature[-1] == "1"

        self.raw_sig = signature[:-2]

        # 2. Parsear nombres (Comando 200+id)
        self.names = [n.strip() for n in field_names.split(",") if n.strip()]

        # 3. Tokenizar nivel superior
        if self.raw_sig.startswith("s<"):
            self.tokens = self._split_inner(self.raw_sig[2:-1])
        else:
            self.tokens = [self.raw_sig]

    def _split_inner(self, inner_str: str) -> List[str]:
        """Trocea respetando niveles de anidamiento < >."""
        parts = []
        depth = 0
        current = ""
        for char in inner_str:
            if char == "<":
                depth += 1
            elif char == ">":
                depth -= 1
            if char == "," and depth == 0:
                parts.append(current)
                current = ""
            else:
                current += char
        if current:
            parts.append(current)
        return parts

    # --- DESERIALIZACIÓN (Bytes -> Datos Reales) ---

    def deserialize(self, buffer: bytes) -> Dict[str, Any]:
        offset = 0
        result = {}
        for i, name in enumerate(self.names):
            if i >= len(self.tokens):
                break
            val, size = self._unpack_recursive(self.tokens[i], buffer, offset)
            result[name] = val
            offset += size
        return result

    def _unpack_recursive(
        self, sig: str, buffer: bytes, offset: int
    ) -> Tuple[Any, int]:
        # A. Struct Anidado
        if sig.startswith("s<"):
            inner_tokens = self._split_inner(sig[2:-1])
            struct_data = []
            current_offset = 0
            for t in inner_tokens:
                val, size = self._unpack_recursive(t, buffer, offset + current_offset)
                struct_data.append(val)
                current_offset += size
            return struct_data, current_offset

        # B. Strings (Serializer.h: uint32_t len + data)
        if sig == "str":
            length = struct.unpack_from("<I", buffer, offset)[0]
            str_data = buffer[offset + 4 : offset + 4 + length].decode("utf-8")
            return str_data, 4 + length

        # C. Vectores (uint32_t len + N elementos)
        if sig.startswith("vec<"):
            inner_type = sig[4:-1]
            count = struct.unpack_from("<I", buffer, offset)[0]
            current_offset = 4
            elements = []
            for _ in range(count):
                val, size = self._unpack_recursive(
                    inner_type, buffer, offset + current_offset
                )
                elements.append(val)
                current_offset += size
            return elements, current_offset

        # D. Arrays (Fijos)
        if sig.startswith("arr<"):
            # Usamos split_inner para separar tipo de tamaño de forma segura
            content_parts = self._split_inner(sig[4:-1])
            inner_type = ",".join(content_parts[:-1])
            count = int(content_parts[-1])
            current_offset = 0
            elements = []
            for _ in range(count):
                val, size = self._unpack_recursive(
                    inner_type, buffer, offset + current_offset
                )
                elements.append(val)
                current_offset += size
            return elements, current_offset

        # E. Tipos básicos (TypeSignature.h)
        fmt = "<" + self.TYPE_MAP[sig]
        return struct.unpack_from(fmt, buffer, offset)[0], struct.calcsize(fmt)

    # --- SERIALIZACIÓN (Datos Reales -> Bytes) ---

    def serialize(self, data_dict: Dict[str, Any]) -> bytes:
        """Convierte el diccionario de datos a bytes para enviar al C++."""
        buffer = bytearray()
        for i, name in enumerate(self.names):
            if i < len(self.tokens):
                val = data_dict.get(name)
                buffer.extend(self._pack_recursive(self.tokens[i], val))
        return bytes(buffer)

    def _pack_recursive(self, sig: str, value: Any) -> bytes:
        # A. Struct Anidado (recibe lista de valores)
        if sig.startswith("s<"):
            inner_tokens = self._split_inner(sig[2:-1])
            res = bytearray()
            for i, t in enumerate(inner_tokens):
                res.extend(self._pack_recursive(t, value[i]))
            return bytes(res)

        # B. Strings
        if sig == "str":
            encoded = str(value).encode("utf-8")
            return struct.pack("<I", len(encoded)) + encoded

        # C. Vectores
        if sig.startswith("vec<"):
            inner_type = sig[4:-1]
            res = struct.pack("<I", len(value))
            for item in value:
                res.extend(self._pack_recursive(inner_type, item))
            return bytes(res)

        # D. Arrays
        if sig.startswith("arr<"):
            content_parts = self._split_inner(sig[4:-1])
            inner_type = ",".join(content_parts[:-1])
            res = bytearray()
            for item in value:
                res.extend(self._pack_recursive(inner_type, item))
            return bytes(res)

        # E. Básicos
        return struct.pack("<" + self.TYPE_MAP[sig], value)
