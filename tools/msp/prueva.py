from commands import init


c = init("localhost", 12345, lambda: print("hola"))

c.add(1)
c.add(16)

if not c.connect():
    print("no conection")
    exit()

a = c.get_cmd(16).set(1564)
print(a)

cmd = c.get_cmd(1)

print(cmd.get_names())
print(cmd.get_signature())

data = []
data += [1500, 1500, 1500, 1500, 1500, 1500, [1500 for i in range(8)]]

print(data)


print(cmd.set(data))
