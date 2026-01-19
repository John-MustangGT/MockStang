"""
OBD-II BLE Dashboard for ESP32-S3 Reverse TFT

Connects to Vgate iCar2 BLE adapter and displays real-time OBD data
on the built-in 240x135 TFT display.

Hardware: Adafruit Feather ESP32-S3 Reverse TFT
Display: ST7789 240x135 color TFT
Connection: BLE to Vgate iCar2 OBD-II adapter

Usage:
    - Copy to CIRCUITPY/code.py
    - Ensure obd_ble_circuitpython.py is in same directory
    - Turn on car ignition
    - Script will auto-connect and display OBD data
"""

import time
import board
import displayio
import terminalio
from adafruit_display_text import label
import adafruit_st7789
from obd_ble_circuitpython import OBDBLEConnection

# =============================================================================
# Display Setup
# =============================================================================

# Release any existing displays
displayio.release_displays()

# Create SPI bus for TFT
tft_cs = board.TFT_CS
tft_dc = board.TFT_DC
tft_reset = board.TFT_RESET
tft_backlight = board.TFT_BACKLIGHT

spi = board.SPI()

# Create display bus
display_bus = displayio.FourWire(
    spi, command=tft_dc, chip_select=tft_cs, reset=tft_reset
)

# Create display (240x135, rotated)
display = adafruit_st7789.ST7789(
    display_bus,
    width=240,
    height=135,
    rotation=270,  # Portrait mode
    rowstart=40,
    colstart=53
)

# Turn on backlight
import digitalio
backlight = digitalio.DigitalInOut(tft_backlight)
backlight.direction = digitalio.Direction.OUTPUT
backlight.value = True

print("="*60)
print("OBD-II BLE Dashboard")
print("ESP32-S3 Reverse TFT")
print("="*60)

# =============================================================================
# Display Layout
# =============================================================================

# Create display group
splash = displayio.Group()
display.root_group = splash

# Colors
COLOR_WHITE = 0xFFFFFF
COLOR_GREEN = 0x00FF00
COLOR_YELLOW = 0xFFFF00
COLOR_RED = 0xFF0000
COLOR_CYAN = 0x00FFFF
COLOR_ORANGE = 0xFFA500

# Title
title = label.Label(
    terminalio.FONT,
    text="OBD Dashboard",
    color=COLOR_CYAN,
    x=10,
    y=8,
    scale=2
)
splash.append(title)

# Connection status
status_label = label.Label(
    terminalio.FONT,
    text="Status: Connecting...",
    color=COLOR_YELLOW,
    x=10,
    y=35
)
splash.append(status_label)

# RPM
rpm_label = label.Label(
    terminalio.FONT,
    text="RPM: ----",
    color=COLOR_WHITE,
    x=10,
    y=55,
    scale=2
)
splash.append(rpm_label)

# Speed
speed_label = label.Label(
    terminalio.FONT,
    text="Speed: -- mph",
    color=COLOR_WHITE,
    x=10,
    y=75
)
splash.append(speed_label)

# Battery voltage
voltage_label = label.Label(
    terminalio.FONT,
    text="Batt: --.- V",
    color=COLOR_GREEN,
    x=10,
    y=90
)
splash.append(voltage_label)

# Coolant temp
coolant_label = label.Label(
    terminalio.FONT,
    text="Coolant: --- C",
    color=COLOR_WHITE,
    x=10,
    y=105
)
splash.append(coolant_label)

# Throttle
throttle_label = label.Label(
    terminalio.FONT,
    text="Throttle: -- %",
    color=COLOR_WHITE,
    x=10,
    y=120
)
splash.append(throttle_label)

# Update message at bottom
update_label = label.Label(
    terminalio.FONT,
    text="Initializing...",
    color=COLOR_YELLOW,
    x=10,
    y=135
)
splash.append(update_label)

# =============================================================================
# Helper Functions
# =============================================================================

def update_status(text, color=COLOR_WHITE):
    """Update status label"""
    status_label.text = f"Status: {text}"
    status_label.color = color
    print(f"[Status] {text}")

def update_rpm(rpm):
    """Update RPM display"""
    if rpm is not None:
        rpm_label.text = f"RPM: {int(rpm)}"
        # Color code based on RPM
        if rpm > 6000:
            rpm_label.color = COLOR_RED
        elif rpm > 4000:
            rpm_label.color = COLOR_ORANGE
        elif rpm > 2000:
            rpm_label.color = COLOR_YELLOW
        else:
            rpm_label.color = COLOR_GREEN
    else:
        rpm_label.text = "RPM: ----"
        rpm_label.color = COLOR_WHITE

def update_speed(speed_kmh):
    """Update speed display (convert km/h to mph)"""
    if speed_kmh is not None:
        speed_mph = speed_kmh * 0.621371
        speed_label.text = f"Speed: {int(speed_mph)} mph"
        speed_label.color = COLOR_GREEN if speed_mph < 70 else COLOR_ORANGE
    else:
        speed_label.text = "Speed: -- mph"
        speed_label.color = COLOR_WHITE

def update_voltage(voltage):
    """Update battery voltage display"""
    if voltage is not None:
        # Parse voltage string (e.g., "12.6V")
        try:
            v = float(voltage.replace('V', ''))
            voltage_label.text = f"Batt: {v:.1f} V"
            # Color code based on voltage
            if v < 11.5:
                voltage_label.color = COLOR_RED
            elif v < 12.0:
                voltage_label.color = COLOR_ORANGE
            elif v < 12.4:
                voltage_label.color = COLOR_YELLOW
            else:
                voltage_label.color = COLOR_GREEN
        except:
            voltage_label.text = f"Batt: {voltage}"
            voltage_label.color = COLOR_WHITE
    else:
        voltage_label.text = "Batt: --.- V"
        voltage_label.color = COLOR_WHITE

def update_coolant(temp_c):
    """Update coolant temperature display"""
    if temp_c is not None:
        coolant_label.text = f"Coolant: {int(temp_c)} C"
        # Color code based on temperature
        if temp_c > 105:
            coolant_label.color = COLOR_RED
        elif temp_c > 95:
            coolant_label.color = COLOR_ORANGE
        elif temp_c > 85:
            coolant_label.color = COLOR_YELLOW
        else:
            coolant_label.color = COLOR_GREEN
    else:
        coolant_label.text = "Coolant: --- C"
        coolant_label.color = COLOR_WHITE

def update_throttle(throttle_pct):
    """Update throttle position display"""
    if throttle_pct is not None:
        throttle_label.text = f"Throttle: {int(throttle_pct)} %"
        # Color code based on throttle
        if throttle_pct > 75:
            throttle_label.color = COLOR_RED
        elif throttle_pct > 50:
            throttle_label.color = COLOR_ORANGE
        elif throttle_pct > 25:
            throttle_label.color = COLOR_YELLOW
        else:
            throttle_label.color = COLOR_GREEN
    else:
        throttle_label.text = "Throttle: -- %"
        throttle_label.color = COLOR_WHITE

def update_message(text):
    """Update bottom message"""
    update_label.text = text

# =============================================================================
# OBD Connection
# =============================================================================

update_status("Scanning for adapter...", COLOR_YELLOW)
update_message("Scanning BLE...")

# Create OBD connection
obd = OBDBLEConnection()

# Connect to adapter (15 second timeout)
print("\n[OBD] Connecting to Vgate adapter...")
if not obd.connect(scan_timeout=15):
    print("[OBD] ERROR: Failed to connect")
    update_status("Connection Failed!", COLOR_RED)
    update_message("No adapter found")

    # Show error for 5 seconds then retry
    time.sleep(5)

    # Reboot to retry
    import microcontroller
    microcontroller.reset()

# Connected!
update_status("Connected!", COLOR_GREEN)
update_message("Reading OBD data...")
print("[OBD] ✓ Connected successfully!")

# =============================================================================
# Main Loop - Read and Display OBD Data
# =============================================================================

loop_count = 0
last_update = 0
update_interval = 0.5  # Update every 500ms

# Cache last values
last_rpm = None
last_speed = None
last_voltage = None
last_coolant = None
last_throttle = None
last_load = None
last_intake_temp = None
last_maf = None

print("\n[OBD] Starting main loop...")
print("="*60)

try:
    while True:
        current_time = time.monotonic()

        # Update display every 500ms
        if current_time - last_update >= update_interval:
            last_update = current_time
            loop_count += 1

            # Read OBD data
            print(f"\n[{loop_count}] Reading OBD PIDs...")

            # Battery voltage (special AT command)
            try:
                voltage = obd._send_command("ATRV")
                if voltage and "V" in voltage:
                    # Extract just the voltage part
                    for line in voltage.split('\r'):
                        if 'V' in line and not line.startswith('AT'):
                            last_voltage = line.strip()
                            break
                    update_voltage(last_voltage)
                    print(f"  Battery: {last_voltage}")
            except Exception as e:
                print(f"  Battery: ERROR - {e}")

            # Engine RPM (010C)
            try:
                rpm = obd.get_rpm()
                if rpm is not None:
                    last_rpm = rpm
                    update_rpm(last_rpm)
                    print(f"  RPM: {int(last_rpm)}")
                else:
                    print(f"  RPM: No data")
            except Exception as e:
                print(f"  RPM: ERROR - {e}")

            # Vehicle speed (010D)
            try:
                speed = obd.get_speed()
                if speed is not None:
                    last_speed = speed
                    update_speed(last_speed)
                    print(f"  Speed: {last_speed:.1f} km/h ({last_speed * 0.621371:.1f} mph)")
                else:
                    print(f"  Speed: No data")
            except Exception as e:
                print(f"  Speed: ERROR - {e}")

            # Coolant temperature (0105)
            try:
                coolant = obd.get_coolant_temp()
                if coolant is not None:
                    last_coolant = coolant
                    update_coolant(last_coolant)
                    print(f"  Coolant: {last_coolant:.0f} °C")
                else:
                    print(f"  Coolant: No data")
            except Exception as e:
                print(f"  Coolant: ERROR - {e}")

            # Throttle position (0111)
            try:
                throttle = obd.get_throttle()
                if throttle is not None:
                    last_throttle = throttle
                    update_throttle(last_throttle)
                    print(f"  Throttle: {last_throttle:.1f} %")
                else:
                    print(f"  Throttle: No data")
            except Exception as e:
                print(f"  Throttle: ERROR - {e}")

            # Additional PIDs (less critical, read occasionally)
            if loop_count % 5 == 0:  # Every 2.5 seconds
                # Engine load (0104)
                try:
                    load = obd.get_load()
                    if load is not None:
                        last_load = load
                        print(f"  Load: {last_load:.1f} %")
                except Exception as e:
                    print(f"  Load: ERROR - {e}")

                # Intake air temperature (010F)
                try:
                    intake_temp = obd.get_intake_temp()
                    if intake_temp is not None:
                        last_intake_temp = intake_temp
                        print(f"  Intake Temp: {last_intake_temp:.0f} °C")
                except Exception as e:
                    print(f"  Intake Temp: ERROR - {e}")

                # MAF (0110)
                try:
                    maf = obd.get_maf()
                    if maf is not None:
                        last_maf = maf
                        print(f"  MAF: {last_maf:.2f} g/s")
                except Exception as e:
                    print(f"  MAF: ERROR - {e}")

            # Update message with loop count
            update_message(f"Update #{loop_count}")

            print(f"[{loop_count}] Update complete")

        # Small delay
        time.sleep(0.1)

except KeyboardInterrupt:
    print("\n\nShutting down...")
    update_status("Disconnected", COLOR_RED)
    update_message("Stopped by user")
    obd.disconnect()
    print("✓ Disconnected")

except Exception as e:
    print(f"\n\nERROR: {e}")
    import traceback
    traceback.print_exception(e)
    update_status("ERROR!", COLOR_RED)
    update_message(str(e)[:20])
    time.sleep(5)
