from include.commands import Commands
from include.connection import Connection

client = Connection()
commands = Commands()

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

import struct
import time

data = []
result = client.transact(commands.get("get_ctr_pidVals"))
floats = [x[0] for x in struct.iter_unpack("<f", result[8:-1])]

for i in range(len(floats)):
    data.append([])
    data[i].append(floats[i])
data.append([0])


while 1:
    result = client.transact(commands.get("get_ctr_pidVals"))
    floats = [x[0] for x in struct.iter_unpack("<f", result[8:-1])]

    for i in range(len(floats)):
        data[i].append(floats[i])
    data[-1].append(data[-1][-1] + 1)

    if len(data[0]) > 100:
        for i in range(len(data)):
            data[i] = data[i][-100:]

    for i in range(len(floats)):
        plt.plot(data[-1], data[i])

    plt.pause(0.5)
