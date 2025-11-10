import asyncio
import struct
import time
import matplotlib.pyplot as plt
import numpy as np
from bleak import BleakClient

address = "D9:B0:06:60:2f:49"
GYROSCOPE_UUID = "00001526-1212-efde-1523-785feabcd123"
DIRECTION_UUID = "00001524-1212-efde-1523-785feabcd123"

max_len = 15

dtype = [('time', 'f8'), ('value', 'u2')]

x_values = np.zeros(max_len, dtype=dtype)
y_values = np.zeros(max_len, dtype=dtype)
z_values = np.zeros(max_len, dtype=dtype)
indication_values = np.zeros(max_len, dtype=[('time', 'f8'), ('value', 'u2')])

x_count = 0
y_count = 0
z_count = 0
direction_count = 0

def handle_indication(sender, data: bytearray):
    global direction_count
    value = struct.unpack("<H", data)[0]  # Pura vastaanotettu arvo ja anna muuttujan value arvoksi
    timestamp = (time.perf_counter() - start_time) * 1000

    # Tarkista arrayn pituus ja laita tuple (aika, arvo)
    if direction_count < len(indication_values):
        indication_values[direction_count] = (timestamp, value)
        direction_count += 1
    
def handle_data(sender, data: bytearray):

    global x_count, y_count, z_count
    value = struct.unpack("<H", data)[0]
    timestamp = (time.perf_counter() - start_time) * 1000

    # Lähetin lisää lähetettyy arvoon x*10^4, josta tunnistaa akselin
    if value > 30000:  # Tunnista Z-akseli
        v = value - 30000
        if v <= 2000 and z_count < len(z_values):
            z_values[z_count] = (timestamp, v)
            z_count += 1
    elif value > 20000:  # Tunnista Y-akseli
        v = value - 20000
        if v <= 2000 and y_count < len(y_values):
            y_values[y_count] = (timestamp, v)
            y_count += 1
    elif value > 10000:  # Tunnista Z-akseli
        v = value - 10000
        if v <= 2000 and x_count < len(x_values):
            x_values[x_count] = (timestamp, v)
            x_count += 1

async def main():
    global start_time
    async with BleakClient(address) as client:
        try:
            await client.connect()
            print(f"Yhdistetty laitteeseen: {address}")
        except Exception as e:
            print(e)
        
        start_time = time.perf_counter()

        char = client.services.get_characteristic(DIRECTION_UUID)
        print(char.properties)

        # Aloita indikaatioiden ja ilmoituksien kuuntelu
        await client.start_notify(GYROSCOPE_UUID, handle_data)
        await client.start_notify(DIRECTION_UUID, handle_indication)
        print("Aloitettu kuuntelu...")

        # Keep running to receive notifications
        await asyncio.sleep(1)

        await client.stop_notify(GYROSCOPE_UUID)
        await client.stop_notify(DIRECTION_UUID)
        print("Lopetettu kuuntelu")

        #print("X:", x_values)
        #print("Y:", y_values)
        #print("Z:", z_values)
        print(indication_values) # Lähinnä debuggausta varten
asyncio.run(main())

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6), sharex=True)

# Gyroskooppidatan ehtotarkistus ja plottaus
if x_count > 0:
    ax1.plot(x_values['time'][:x_count], x_values['value'][:x_count], 'r-o', label='X')
if y_count > 0:
    ax1.plot(y_values['time'][:y_count], y_values['value'][:y_count], 'g-o', label='Y')
if z_count > 0:
    ax1.plot(z_values['time'][:z_count], z_values['value'][:z_count], 'b-o', label='Z')

ax1.set_ylabel("Akselin arvo")
ax1.set_title("Gyroskooppidata")
ax1.grid(True)
ax1.legend()

# Tarkista, onko indikaatioita vastaanotettu ja plottaa
if direction_count > 0:
    ax2.plot(indication_values['time'][:direction_count], 
             indication_values['value'][:direction_count], 
             'm-s', label='Napin arvo')

    ax2.set_xlabel("Aika (ms)")
    ax2.set_ylabel("Painallusarvo")
    ax2.set_title("Nappi-indikaatiot")
    ax2.grid(True)
    ax2.legend()

plt.tight_layout()
plt.show()

