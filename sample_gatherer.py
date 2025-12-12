import serial
import csv
from datetime import datetime
import struct


# Gather 800 samples over UART2 (nRF5340DK)

SERIAL_PORT = "/dev/ttyACM3"
BAUD_RATE = 115200
TIMEOUT = 1  

CSV_FILE = "samples.csv"

FRAME_MARKER = b'\xAA\x55'
FRAME_SIZE = 6  

def read_frame(ser):
    """Wait until a full valid frame is received."""
    while True:
        byte = ser.read(1)
        if not byte:
            continue  

        if byte == b'\xAA':
            next_byte = ser.read(1)
            if not next_byte:
                continue

            if next_byte == b'\x55':
                frame = ser.read(FRAME_SIZE)
                if len(frame) == FRAME_SIZE:
                    return frame


def main():
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=TIMEOUT)
    print(f"Listening on {SERIAL_PORT} at {BAUD_RATE}bps...")

    sample_count = 0
    max_samples = 800

    with open(CSV_FILE, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["Timestamp", "X", "Y", "Z"])

        try:
            while sample_count < max_samples:
                frame_data = read_frame(ser)  # Read frames

                x, y, z = struct.unpack('<hhh', frame_data)
                timestamp = datetime.now().isoformat(timespec='microseconds')
                csv_writer.writerow([timestamp, x, y, z])

                sample_count += 1
                print(f"{sample_count}/{max_samples} -> X:{x} Y:{y} Z:{z}")

        except KeyboardInterrupt:
            print("Stopping early...")

    print("Finished collecting 800 samples.")


if __name__ == "__main__":
    main()

