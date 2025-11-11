import asyncio
from bleak import BleakClient
import mysql.connector as mysql
from datetime import datetime

# BLE-laitteen MAC-osoite
DEVICE_ADDRESS = "AA:BB:CC:DD:EE:FF"
CHARACTERISTIC_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"

# MySQL-yhteys
DB_CONFIG = {
    "host": "172.20.241.9",
    "user": "dbaccess_rw",
    "password": "fasdjkf2389vw2c3k234vk2f3",
    "database": "measurements"
}

# Tallennusfunktio
def save_to_db(from_mac, to_mac, sv_a, sv_b, sv_c, sv_d):
    conn = mysql.connect(**DB_CONFIG)
    c = conn.cursor()
    sql = """
    INSERT INTO rawdata (groupid, from_mac, to_mac,
                         sensorvalue_a, sensorvalue_b, sensorvalue_c, sensorvalue_d)
    VALUES (%s, %s, %s, %s, %s, %s, %s)
    """
    data = (24, from_mac, to_mac, sv_a, sv_b, sv_c, sv_d)
    c.execute(sql, data)
    conn.commit()
    conn.close()
    print(f"[DB] Tallennettu: {data}")

# BLE-datan käsittely
def notification_handler(sender, data):
    text = data.decode("utf-8").strip()
    print(f"[BLE] Vastaanotettu: {text}")

    try:
        # Odotetaan dataa muodossa: from_mac,to_mac,a,b,c,d
        from_mac, to_mac, a, b, c_val, d = text.split(",")
        save_to_db(from_mac, to_mac, float(a), float(b), float(c_val), float(d))
    except ValueError:
        print("[BLE] Virheellinen dataformaatti!")

# BLE-pääohjelma
async def main():
    print(f"Yhdistetään laitteeseen {DEVICE_ADDRESS}...")
    async with BleakClient(DEVICE_ADDRESS) as client:
        if not client.is_connected:
            print("Yhteys epäonnistui!")
            return

        print("Yhteys muodostettu!")
        await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
        print("Kuunnellaan dataa... (Ctrl+C lopettaa)")
        try:
            while True:
                await asyncio.sleep(1)
        except KeyboardInterrupt:
            print("Katkaistaan yhteys...")
            await client.stop_notify(CHARACTERISTIC_UUID)

if __name__ == "__main__":
    asyncio.run(main())
