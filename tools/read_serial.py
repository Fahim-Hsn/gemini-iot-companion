import serial
import time
import sys

try:
    ser = serial.Serial('COM9', 115200, timeout=1)
    print("Connected to COM9. Reading logs for 4 seconds...")
    start = time.time()
    while time.time() - start < 4:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line:
            print(line.strip())
    ser.close()
except Exception as e:
    print(f"Error opening COM9: {e}")
