from commands.commandsMgr import CommandsMgr
from commands.connection import Connection


def init(ip, port, callbacks) -> CommandsMgr:
    client = Connection(ip, port)
    for callback in callbacks:
        client.add_callback_ondisconect(callback)

    cmdMgr = CommandsMgr(client)

    for cmd in [1, 2, 3, 11, 19, 20, 21, 22, 23, 24, 25, 26, 27]:
        cmdMgr.add(cmd)

    return cmdMgr
