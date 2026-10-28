"""JN5189 ISP flasher with status checks: erase + blank check + write (512 B chunks, last one padded with 0xFF) + reset.

Flash is write-only on this device (no read-back), so every response status is checked instead.
Usage: python isp_flash.py COM7 firmware.bin [--no-reset]
On this device ISP accepts only a full flash erase (a partial erase returns 0xF3 access invalid), so the network
and settings stored in flash are always lost.
"""
import struct
import sys
import zlib

import serial

KEY = bytes.fromhex("11223344556677881122334455667788")
STATUS = {0x00: "OK", 0xEF: "invalid mode", 0xF0: "bad state", 0xF1: "too long", 0xF2: "out of range",
          0xF3: "access invalid", 0xF4: "not supported", 0xF5: "invalid", 0xF6: "no response",
          0xF7: "not authorized", 0xF9: "read fail", 0xFC: "CRC error", 0xFE: "write fail", 0xFF: "not supported"}


def frame(cmd: int, payload: bytes) -> bytes:
    body = struct.pack(">BHB", 0, 8 + len(payload), cmd) + payload
    return body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)


def xfer(ser: serial.Serial, cmd: int, payload: bytes = b"", what: str = "") -> bytes:
    ser.write(frame(cmd, payload))
    hdr = ser.read(4)
    if len(hdr) < 4:
        sys.exit(f"FAIL {what}: no response")
    _, length, rtype = struct.unpack(">BHB", hdr)
    rest = ser.read(length - 4)
    data, crc = rest[:-4], struct.unpack(">I", rest[-4:])[0]
    if (zlib.crc32(hdr + data) & 0xFFFFFFFF) != crc:
        sys.exit(f"FAIL {what}: bad CRC in response")
    if rtype != cmd + 1:
        sys.exit(f"FAIL {what}: unexpected response type 0x{rtype:02x}")
    if data and data[0] != 0:
        sys.exit(f"FAIL {what}: status 0x{data[0]:02x} ({STATUS.get(data[0], '?')})")
    return data


port, path = sys.argv[1], sys.argv[2]
no_reset = "--no-reset" in sys.argv
image = open(path, "rb").read()

with serial.Serial(port, 115200, timeout=30) as ser:
    xfer(ser, 0x4E, b"\x00", "unlock default")
    xfer(ser, 0x4E, b"\x01" + KEY, "unlock ISP")
    d = xfer(ser, 0x4C, b"\x00", "flash info")
    _, base, length, sector, _, access = struct.unpack("<BIIIBB", d[1:16])
    print(f"FLASH base=0x{base:x} len=0x{length:x} sector=0x{sector:x} access=0x{access:02x}")
    if len(image) > length:
        sys.exit("FAIL image larger than flash")

    erase_len = length
    xfer(ser, 0x40, bytes([0x00, access]), "open flash")
    print(f"Erasing 0x{erase_len:x} bytes...")
    xfer(ser, 0x42, struct.pack("<BBII", 0, 0, base, erase_len), "erase")
    xfer(ser, 0x44, struct.pack("<BBII", 0, 0, base, erase_len), "blank check")
    xfer(ser, 0x4A, b"\x00", "close after erase")
    print("Erase + blank check OK")

    xfer(ser, 0x40, bytes([0x00, access]), "open flash for write")
    chunks = (len(image) + sector - 1) // sector
    for i in range(chunks):
        chunk = image[i * sector:(i + 1) * sector].ljust(sector, b"\xff")
        xfer(ser, 0x48, struct.pack("<BBII", 0, 0, base + i * sector, sector) + chunk, f"write chunk {i}")
        if i % 32 == 0 or i == chunks - 1:
            print(f"  written {min((i + 1) * sector, len(image))}/{len(image)}")
    xfer(ser, 0x4A, b"\x00", "close after write")
    print(f"Write OK: {chunks} chunks of {sector} bytes")

    if not no_reset:
        xfer(ser, 0x14, b"", "reset")
        print("Reset sent")
