from include.commands import Commands
from include.connection import Connection

client = Connection()
commands = Commands()

while 1:
    cmds = commands.get_all()
    for cmd in range(len(cmds)):
        print(cmd, end=" : ")
        print(cmds[cmd])

    cmd_idx = input(">")

    if cmd_idx == "q":
        break
    else:
        cmd_selected = cmds[int(cmd_idx)]

    payload = commands.get(cmd_selected)
    response = client.transact(payload)
    print(response.hex(" "))
