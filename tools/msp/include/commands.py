from include.msp import MSP, MspPacket
import struct


class Commands:
    def __init__(self):
        self.commands = {}
        self.initcmds()

    def initcmds(self):
        self.add("set_ctr_passThrow", 200, "passThrow")
        self.add("set_ctr_altHold", 200, "altHold")
        self.add("set_ctr_period", 201, [10])
        self.add("set_ctr_pidCts", 300, [0.15, 0.21, 0.32])
        self.add("get_ctr_pidVals", 301, [])

    def add(self, name, command, payload):
        # curamos payload
        pay_type = type(payload)

        if pay_type is str:
            payload = list(payload.encode("utf-8"))
        if pay_type is list and len(payload) > 0 and type(payload[0]) is float:
            payload = list(b"".join(struct.pack("<f", val) for val in payload))

        msp = MSP()

        p = MspPacket(direction=ord(">"), command=command, payload=payload)

        byt = msp.encode(p)

        self.commands[name] = bytes.fromhex(byt.hex().upper())

    def get_all(self):
        return list(self.commands.keys())

    def get(self, cmd_name):
        if cmd_name in self.get_all():
            return self.commands[cmd_name]
        else:
            return "Error"
