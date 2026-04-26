from commands.commandsMgr import CommandsMgr
from commands.connection import Connection


def init(ip, port, callbacks) -> CommandsMgr:
    client = Connection(ip, port)
    for callback in callbacks:
        client.add_callback_ondisconect(callback)

    cmdMgr = CommandsMgr(client)

    for cmd in [
        1,
        2,
        3,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        12,
        13,
        14,
        15,
        19,
        20,
        21,
        22,
        23,
        24,
        25,
        26,
        27,
        # 30,
        # 31,
        40,
        41,
        42,
        43,
        44,
        45,
    ]:
        cmdMgr.add(cmd)

    return cmdMgr
