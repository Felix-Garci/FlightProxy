from commands.commandsMgr import CommandsMgr

c = CommandsMgr()

c.add(15)

c.connect()

cmd = c.get_cmd(15)


cmd.set({"p": 1, "i": 2, "d": 3})
