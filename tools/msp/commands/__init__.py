from commander.commandMGR import commandMGR
from commander.cmd_transformer import Cmd
from commander.connection import Connection
from commander.msp import MSP


def init_cmds() -> list[Cmd]:
    cmds = []

    cmds.append(Cmd(101, "", "str"))
    cmds.append(Cmd(201, "", "str"))
    # cmds.append(Cmd(200, "str", ""))  # Set ctrl
    # cmds.append(Cmd(201, "H", ""))  # Set ctrl period
    # cmds.append(Cmd(250, "", "3f"))  # get dinamix telemetry
    # cmds.append(Cmd(270, "6H", ""))  # Set RC setpoints
    # cmds.append(Cmd(300, "3f", ""))  # Set P I D cts
    # cmds.append(Cmd(301, "", "6f"))  # Get pid telemetry
    # cmds.append(Cmd(302, "H", ""))  # set Hover

    return cmds


def init(ip, port, callback) -> commandMGR:
    client = Connection(ip, port, callback)
    msp = MSP()
    cmdMgr = commandMGR(msp, client)

    for cmd in init_cmds():
        cmdMgr.add(cmd)

    return cmdMgr
