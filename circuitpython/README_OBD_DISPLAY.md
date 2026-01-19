# OBD-II BLE Dashboard for CircuitPython

Real-time OBD-II data display using BLE connection to Vgate iCar2 adapter.

## Hardware Requirements

- **Adafruit Feather ESP32-S3 Reverse TFT**
  - Built-in 240x135 color TFT display (ST7789)
  - Built-in BLE support
  - CircuitPython 9.0+ compatible

- **Vgate iCar2 BLE OBD-II Adapter**
  - Device name: "IOS-Vlink"
  - BLE 4.0
  - ELM327 v2.1 compatible

## Software Requirements

### CircuitPython Libraries

Copy these to `/lib/` on CIRCUITPY drive:

```
adafruit_st7789.mpy
adafruit_display_text/
```

Download from: https://circuitpython.org/libraries

### Files to Copy

Copy to CIRCUITPY drive root:

```
obd_ble_circuitpython.py
obd_display_esp32s3.py  → rename to code.py
```

## Installation

1. **Install CircuitPython 9.0+** on ESP32-S3 Reverse TFT
   - Download from https://circuitpython.org/board/adafruit_feather_esp32s3_reverse_tft/

2. **Copy required libraries** to `/lib/`:
   ```
   /lib/adafruit_st7789.mpy
   /lib/adafruit_display_text/
   ```

3. **Copy program files** to root:
   ```
   /obd_ble_circuitpython.py
   /code.py (renamed from obd_display_esp32s3.py)
   ```

4. **Plug Vgate iCar2 into car's OBD port**

5. **Turn on ignition** (engine doesn't need to run for most PIDs)

6. **Power ESP32-S3** - it will auto-connect and display data

## Display Layout

```
┌──────────────────────────────┐
│  OBD Dashboard               │ ← Title
├──────────────────────────────┤
│  Status: Connected           │ ← Connection status
│                              │
│  RPM: 2500                   │ ← Engine RPM (large)
│                              │
│  Speed: 65 mph               │ ← Vehicle speed
│  Batt: 12.6 V                │ ← Battery voltage
│  Coolant: 88 C               │ ← Coolant temperature
│  Throttle: 45 %              │ ← Throttle position
│                              │
│  Update #123                 │ ← Status message
└──────────────────────────────┘
```

## Color Coding

Values change color based on readings:

### RPM
- 🟢 Green: < 2000 RPM (idle/cruise)
- 🟡 Yellow: 2000-4000 RPM (normal)
- 🟠 Orange: 4000-6000 RPM (high)
- 🔴 Red: > 6000 RPM (redline)

### Battery Voltage
- 🟢 Green: ≥ 12.4V (healthy)
- 🟡 Yellow: 12.0-12.4V (moderate)
- 🟠 Orange: 11.5-12.0V (low)
- 🔴 Red: < 11.5V (critical)

### Coolant Temperature
- 🟢 Green: < 85°C (normal)
- 🟡 Yellow: 85-95°C (warm)
- 🟠 Orange: 95-105°C (hot)
- 🔴 Red: > 105°C (overheating)

### Throttle Position
- 🟢 Green: 0-25% (light)
- 🟡 Yellow: 25-50% (moderate)
- 🟠 Orange: 50-75% (heavy)
- 🔴 Red: > 75% (full throttle)

## Displayed PIDs

| PID  | Description              | Unit   | Update Rate |
|------|--------------------------|--------|-------------|
| ATRV | Battery Voltage          | Volts  | 0.5 Hz      |
| 010C | Engine RPM               | RPM    | 0.5 Hz      |
| 010D | Vehicle Speed            | MPH    | 0.5 Hz      |
| 0105 | Coolant Temperature      | °C     | 0.5 Hz      |
| 0111 | Throttle Position        | %      | 0.5 Hz      |
| 0104 | Engine Load              | %      | 0.2 Hz*     |
| 010F | Intake Air Temperature   | °C     | 0.2 Hz*     |
| 0110 | MAF Air Flow Rate        | g/s    | 0.2 Hz*     |

*Serial debug only, not displayed on screen

## Serial Debug Output

Connect to serial console (115200 baud) to see detailed debug info:

```
OBD-II BLE Dashboard
ESP32-S3 Reverse TFT
============================================================
[OBD BLE] Scanning for Vgate adapter (15s)...
[OBD BLE] Found: IOS-Vlink
[OBD BLE] Connecting...
[OBD BLE] Found service: 0x18f0
[OBD BLE] Found write char: 0x2af1
[OBD BLE] Found notify char: 0x2af0
[OBD BLE] Subscribed to notifications
[OBD BLE] Echo disabled
[OBD BLE] ✓ Connected and initialized
[Status] Connected!
[OBD] Starting main loop...
============================================================

[1] Reading OBD PIDs...
  Battery: 12.6V
  RPM: 850
  Speed: 0.0 km/h (0.0 mph)
  Coolant: 88 °C
  Throttle: 14.5 %
[1] Update complete

[2] Reading OBD PIDs...
  Battery: 12.6V
  RPM: 2500
  Speed: 65.2 km/h (40.5 mph)
  Coolant: 90 °C
  Throttle: 45.2 %
[2] Update complete
```

## Troubleshooting

### "Connection Failed!" on display
- Ensure Vgate adapter is plugged into OBD port
- Turn ignition to ACC or ON position
- Check adapter LED is blinking (advertising)
- Verify adapter not connected to phone app
- Try increasing scan timeout in code (line with `scan_timeout=15`)

### Display shows old/cached values
- This is normal - last valid values are retained when no new data
- If engine is off, RPM will show 0 or last value before shutdown
- Battery voltage updates even with engine off

### "No data" in serial output
- Vehicle may not support that PID
- Some PIDs require engine running (RPM, MAF)
- Some PIDs are vehicle-specific (check ELM327 documentation)

### BLE connection drops
- Check power supply to ESP32-S3 (USB power recommended)
- Ensure vehicle battery voltage is adequate (> 11.5V)
- Move ESP32-S3 closer to OBD adapter if possible

### Display is blank
- Check TFT backlight (should be on)
- Verify CircuitPython installed correctly
- Check serial output for errors
- Try pressing reset button on ESP32-S3

## Customization

### Change Update Rate

Edit `update_interval` in code.py:

```python
update_interval = 0.5  # 500ms (2 Hz)
```

### Add More PIDs to Display

1. Look up PID code in ELM327 documentation
2. Add label in display setup section
3. Add query and update function in main loop
4. See `obd_ble_circuitpython.py` for PID decoding formulas

### Change Display Colors

Colors defined at top of `code.py`:

```python
COLOR_WHITE = 0xFFFFFF
COLOR_GREEN = 0x00FF00
COLOR_YELLOW = 0xFFFF00
COLOR_RED = 0xFF0000
COLOR_CYAN = 0x00FFFF
COLOR_ORANGE = 0xFFA500
```

## Performance

**Measured on ESP32-S3:**
- BLE connection: 2-5 seconds
- Single PID query: 50-200ms
- Display update: ~500ms (5 PIDs)
- Main loop: 2 Hz (500ms per cycle)

## Credits

Based on protocol reverse-engineering from the MockStang project by analyzing real Vgate iCar2 adapter behavior with BLE sniffer tools.

## References

- [ELM327 Commands](https://www.elmelectronics.com/wp-content/uploads/2017/01/ELM327DS.pdf)
- [OBD-II PIDs](https://en.wikipedia.org/wiki/OBD-II_PIDs)
- [CircuitPython _bleio](https://docs.circuitpython.org/en/latest/shared-bindings/_bleio/)
- [Adafruit ST7789 Guide](https://learn.adafruit.com/adafruit-1-14-240x135-color-tft-breakout)
