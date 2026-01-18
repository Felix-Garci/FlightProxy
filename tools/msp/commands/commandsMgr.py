from commands.msp import MSP, MspPacket
from commands.connection import Connection
from commands.serialize import FlightProxyParser

import threading


class cmd:
    def __init__(self, id, transactor):
        self.id_: int = id
        self.cmd_on = False
        self.serialize_: FlightProxyParser
        self.transactor_ = transactor

    def setup(self):
        self.serialize_ = FlightProxyParser("str11", "response")

        response_payload = self.transactor_(100 + self.id_, [])
        response_payload = bytes(response_payload)

        signature = self.serialize_.deserialize(response_payload)

        response_payload = self.transactor_(200 + self.id_, [])
        response_payload = bytes(response_payload)

        names = self.serialize_.deserialize(response_payload)

        self.serialize_ = FlightProxyParser(signature["response"], names["response"])

        self.cmd_on = True

    def get(self):
        if self.cmd_on and self.serialize_.can_consume:
            response_payload = self.transactor_(300 + self.id_, [])
            response_payload = bytes(response_payload)
            data = self.serialize_.deserialize(response_payload)
            return data

        else:
            return None

    def set(self, data):
        if self.cmd_on and self.serialize_.can_produce:
            self.serialize_.serialize(data)
            response_payload = self.transactor_(400 + self.id_, [])
            print(response_payload)
            return True
        else:
            return False


class CommandsMgr:
    def __init__(self):
        self.c = Connection("localhost", 12345, self._on_disconect)
        self.m = MSP()
        self.cmds: dict[int, cmd] = {}

    def add(self, id):
        self.cmds[id] = cmd(id, self._transac)

    def connect(self):
        if not self.c.connect():
            return False

        for cmd in self.cmds.values():
            cmd.setup()

        return True

    def get_cmd(self, id):
        return self.cmds[id]

    def _on_disconect(self):
        for cmd in self.cmds.values():
            cmd.cmd_on = False

    def _transac(self, id, bytes_request: list[int]) -> None | list[int]:
        threading.Lock()

        p_request = MspPacket(direction=ord(">"), command=id, payload=bytes_request)

        row_request = self.m.encode(p_request)

        row_response = self.c.transact(row_request)

        if not row_response:
            return None

        p_response = self.m.decode(row_response)

        if not p_response:
            return None

        bytes_response = p_response.payload

        return bytes_response
