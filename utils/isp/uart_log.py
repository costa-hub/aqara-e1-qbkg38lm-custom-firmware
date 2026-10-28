"""Read the firmware debug log from USART0 for N seconds. Usage: python uart_log.py COM7 60 out.txt"""
import sys
import time

import serial

port, seconds, out = sys.argv[1], float(sys.argv[2]), sys.argv[3]
deadline = time.time() + seconds
with serial.Serial(port, 115200, timeout=0.5) as ser, open(out, "w", encoding="utf-8", errors="replace") as f:
    while time.time() < deadline:
        data = ser.read(4096)
        if data:
            f.write(data.decode("utf-8", errors="replace"))
            f.flush()
