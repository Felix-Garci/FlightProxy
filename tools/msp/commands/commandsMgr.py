from commands.msp import MSP, MspPacket
from commands.connection import Connection
from commands.cmd import cmd

import threading


class CommandsMgr:
    def __init__(self, connection: Connection):
        self.client = connection  # Connection("localhost", 12345, self._on_disconect)
        self.client.add_callback_ondisconect(self._on_disconect)
        self.m = MSP()
        self.cmds: dict[int, cmd] = {}

    def add(self, id):
        self.cmds[id] = cmd(id, self._transac)

    def connect(self):
        if not self.client.connect():
            return False

        for comando in self.cmds.values():
            comando.setup()

        return True

    def get_cmd(self, id):
        return self.cmds[id]

    def _on_disconect(self):
        for comando in self.cmds.values():
            comando.cmd_on = False

    def _transac(self, id, bytes_request: list[int]) -> None | list[int]:
        threading.Lock()

        p_request = MspPacket(direction=ord(">"), command=id, payload=bytes_request)

        row_request = self.m.encode(p_request)

        row_response = self.client.transact(row_request)

        if not row_response:
            return None

        p_response = self.m.decode(row_response)

        if not p_response:
            return None

        bytes_response = p_response.payload

        return bytes_response
