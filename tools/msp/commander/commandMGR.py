from commander.connection import Connection
from commander.msp import MSP, MspPacket
from commander.cmd_transformer import Cmd


class commandMGR:
    def __init__(self, msp: MSP, client: Connection):
        self.msp = msp
        self.client = client

        self.cmds: dict[int, Cmd] = {}

    def add(self, cmd: Cmd):
        self.cmds[cmd.id] = cmd

    def process(self, id, input=None):
        raw_in_payload = self.cmds[id].process_in(input)
        p = MspPacket(direction=ord(">"), command=id, payload=raw_in_payload)

        row_out_packet = self.client.transact(self.msp.encode(p))

        if row_out_packet is None:
            return None

        p_out = self.msp.decode(row_out_packet)

        if isinstance(p_out, MspPacket):
            return self.cmds[id].process_out(p_out.payload)

        return None
