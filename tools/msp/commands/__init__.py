from commands.commandsMgr import CommandsMgr
from commands.connection import Connection


def init(ip, port, callbacks) -> CommandsMgr:
    client = Connection(ip, port)
    for callback in callbacks:
        client.add_callback_ondisconect(callback)

    cmdMgr = CommandsMgr(client)

    for cmd in [1, 2, 11, 12, 13, 14, 15, 16]:
        cmdMgr.add(cmd)

    return cmdMgr
