<h1 align="center">Aqara E1 · no neutral · single rocker<br><sub>custom Zigbee firmware for QBKG38LM</sub></h1>

<p align="center">
  <a href="https://github.com/costa-hub/aqara-e1-qbkg38lm-custom-firmware/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/costa-hub/aqara-e1-qbkg38lm-custom-firmware?label=firmware"></a>
  <img alt="Chip" src="https://img.shields.io/badge/MCU-NXP%20JN5189-blue">
  <img alt="Zigbee2MQTT" src="https://img.shields.io/badge/Zigbee2MQTT-external%20converter-yellow">
  <a href="LICENSE.md"><img alt="License" src="https://img.shields.io/badge/license-GPL--3.0-green"></a>
</p>

<p align="center">
  <img src="images/QBKG38LM/front.jpg" width="320" alt="Power board">
  <img src="images/QBKG38LM/back.jpg" width="320" alt="Module board">
</p>

## Why

The stock firmware of the Aqara E1 wall switch (no neutral) waits after every press to see whether a double click is
coming. In **decoupled** mode - the rocker drives smart bulbs instead of the relay - that wait is a noticeable delay
between pressing the rocker and the light reacting.

This firmware reports a press **the moment it happens** and switches the relay at once. It also adds settings that the
stock firmware does not have.

## Stock vs this firmware

|  | Stock | This firmware |
|---|---|---|
| Reaction to a press | after the double-click window | immediate (`toggle` mode) |
| Rocker controls the relay / decoupled | ✅ | ✅ `operation_mode` |
| Action types | single, double | `toggle`, `momentary_pressed/released`, `single/double/triple_click`, `hold/release` (`button_mode`) |
| Indicator LED | normal / flipped | lit while off, while on, always, never (`led_mode`) |
| Power outage memory | ✅ | ✅ |
| Protection against an accidental factory reset | - | `prevent_reset` |
| Device temperature, anti-flicker mode, OTA updates | ✅ | not yet |

## Quick start

> [!WARNING]
> - **One-way trip.** The manufacturer locked the JN5189 programming port to write-only: the stock firmware cannot be
>   read out, backed up or put back. Aqara update files are encrypted.
> - **The circuit ground is the mains live wire.** Program and test the module on the bench only. Never connect a
>   USB-UART adapter, a scope or a bench supply while the switch is on mains.
> - The adapter must use **3.3 V** logic. You do this at your own risk.

### 1. Download
From the [latest release](https://github.com/costa-hub/aqara-e1-qbkg38lm-custom-firmware/releases/latest):
- `QBKG38LM_<version>.bin` - the firmware image
- `qbkg38lm_diy.mjs` - the Zigbee2MQTT external converter
- `isp_flash.py` - the flashing script (also in [`utils/isp`](utils/isp))

### 2. Wire up
Take the switch out of the wall and separate the module. The 6 programming pads sit in a row above the JN5189,
numbered **right to left**:

<p align="center"><img src="images/QBKG38LM/programming_pads.jpg" width="680" alt="Programming pads"></p>

| Pad | Signal | USB-UART | When |
|:---:|---|---|---|
| 1 | GND | GND | always |
| 2 | RESET (TPA14) | GND through a push button | to restart the chip |
| 3 | VBAT (TPB14) | 3.3V | always (powers the chip) |
| 4 | ISP | GND | only while restarting into the bootloader |
| 5 | RX (TPA12) | TX | always |
| 6 | TX (TPA11) | RX | always |

### 3. Flash
1. Hold **ISP** at GND, press **RESET**, release ISP: the chip is now in the bootloader.
2. Run (Python 3 + `pip install pyserial`; port `COMx` on Windows, `/dev/ttyUSB0` on Linux):
   ```bash
   python isp_flash.py COM7 QBKG38LM_1.0.0.bin
   ```
3. After `Reset sent` the red, then the blue LEDs flash once: the firmware is running.

<details>
<summary>What a good run looks like, and what to do when it fails</summary>

```text
FLASH base=0x0 len=0x9de00 sector=0x200 access=0x0a
Erasing 0x9de00 bytes...
Erase + blank check OK
  written 512/169136
  ...
Write OK: 331 chunks of 512 bytes
Reset sent
```

| Message | Meaning |
|---|---|
| `FAIL unlock default: no response` | Not in the bootloader. Repeat step 1; check that TX/RX are crossed, the port and the 3.3 V supply. |
| `FAIL open flash: status 0xf0 (bad state)` | The bootloader got stuck after a failed command. Repeat step 1. |
| `FAIL erase: status 0xf3 (access invalid)` | A partial erase was attempted; this chip only allows erasing everything. Use the script unchanged. |

Every flash erases the whole chip, including the Zigbee network: pair the switch again afterwards.
</details>

### 4. Pair with Zigbee2MQTT
1. Add `qbkg38lm_diy.mjs` as an external converter (Settings → Dev console → External converters) and restart
   Zigbee2MQTT. On Zigbee2MQTT 2.11+ `advanced.enable_external_js` has to be enabled.
2. If the switch was paired before, force-remove it.
3. Permit join and hold the rocker for 5 s. Red LEDs blink fast while joining; blue LEDs blink once joined.

The switch appears as `QBKG38LM` (model ID `lumi.switch.b1lc04.diy`). The ID differs from the stock
`lumi.switch.b1lc04` on purpose, so the built-in stock definition and its OTA never apply.

## Settings

| Setting | Values | What it does |
|---|---|---|
| `state` | `ON` / `OFF` | The relay. Commands apply at once: the switch keeps its radio on. |
| `operation_mode` | **`control_relay`**, `decoupled` | Whether the rocker switches the relay or only sends `action`. |
| `button_mode` | **`toggle`**, `momentary_on_off`, `multistate` | `toggle`: one `toggle` action per press, no waiting. `multistate` waits ~400 ms for double / triple clicks. |
| `led_mode` | **`on_when_off`**, `on_when_on`, `always`, `never` | When the blue LEDs are lit. A press shows as a short blink. |
| `power_outage_memory` | **`false`**, `true` | `true`: after a power cut the relay stays as it was. `false`: it is switched off. |
| `prevent_reset` | **`OFF`**, `ON` | `OFF`: a 5 s hold leaves the network. `ON`: the hold is reported as `hold` / `release` instead. |
| `action` | `toggle`, `momentary_pressed`, `momentary_released`, `single_click`, `double_click`, `triple_click`, `hold`, `release` | Rocker events for automations. |

Defaults in **bold**.

## Hardware notes
Pin map, relay drive, the coil supply switch and how to power the module on the bench:
[firmwares/QBKG38LM/README.md](firmwares/QBKG38LM/README.md).

## Building from source

<details>
<summary>Windows, without Docker (Git Bash)</summary>

1. [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) 14.2.Rel1 for
   `arm-none-eabi`, unpacked e.g. to `C:/arm-gnu-14.2`.
2. Python 3.12 with the build tools:
   ```bash
   python -m venv C:/fw-venv
   C:/fw-venv/Scripts/python.exe -m pip install -r utils/requirements-windows.txt
   ```
3. Build:
   ```bash
   export TOOLCHAIN_DIR='C:/arm-gnu-14.2' PYTHON_EXE='C:/fw-venv/Scripts/python.exe'
   export PATH="/c/arm-gnu-14.2/bin:/c/fw-venv/Scripts:$PATH"
   cmake --preset QBKG38LM -G Ninja -B build/QBKG38LM
   cmake --build build/QBKG38LM
   ```
   Output: `firmwares/QBKG38LM/QBKG38LM_<version>.bin` (`FW_VERSION` in `firmwares/CMakeLists.txt`).

`-DAPP_DEBUG=ON` builds `QBKG38LM_<version>_debug.bin` with a log on the chip's UART (115200); read it with
`python utils/isp/uart_log.py COM7 60 log.txt`.
</details>

On Linux the same steps work with the Linux toolchain; [the CI workflow](.github/workflows/build_firmwares.yml) is a
working example.

NXP references: [JN5189 product page](https://www.nxp.com/products/JN5189_88_T) (data sheet, user manual UM11138),
[NXP Zigbee documentation](https://mcuxpresso.nxp.com/mcuxsdk/25.12.00/html/middleware/wireless/zigbee/Docs/JNUG3132/OTA_upgrade_cluster/topics/ota_upgrade_cluster.html)
(JN-UG-3130…3133).

## Credits
- [**mgavryliuk/zcf-jn5189-ed-switches**](https://github.com/mgavryliuk/zcf-jn5189-ed-switches) - JN5189 firmware for
  Aqara wireless remotes that this project grew from: build system, Zigbee node, button handling, configuration cluster.
- [**ducktoast.com: Aqara E1 switch fix**](https://www.ducktoast.com/blog/electronics-projects/aqara-e1/aqara-e1-switch-fix/) -
  teardown and schematic of the E1 that made the circuit much easier to follow (it covers a different board revision).

## License
[GPL-3.0](LICENSE.md), as the project it is based on. This is a modified version of
[mgavryliuk/zcf-jn5189-ed-switches](https://github.com/mgavryliuk/zcf-jn5189-ed-switches); changes by costa,
October 2026:
- single target `QBKG38LM` (pins, Zigbee stack configuration, Zigbee2MQTT converter); the wireless remote targets are
  removed
- mains powered end device with the radio always on
- On/Off server, latching relay drive and coil supply switch, state kept across reboots
- operation mode, LED mode and power outage memory attributes; Configure Reporting support
- ZCL commands handled on every application endpoint; Zigbee stack configuration rebuilt when `app.zpscfg` changes
- Windows build, versioned firmware file name, ISP flashing tools
- unused NXP SDK parts, the Dev Container and the upstream flasher are removed

`sdk/` holds only the parts of the NXP JN5189 SDK this firmware is built from. They keep their own licenses
(BSD-3-Clause, Apache-2.0, NXP license of the Zigbee stack - see the file headers). `eZCL_ReportAttribute` in
`sdk/middleware/wireless/zigbee/ZCIF/Source/zcl.c` is patched (upstream) to disable the default response.
