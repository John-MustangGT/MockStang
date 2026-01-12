# MockStang User Manual

**Version 1.0**
WiFi OBD-II Emulator for ESP-01/ESP-01S

---

## Table of Contents

1. [Introduction](#introduction)
2. [Hardware Requirements](#hardware-requirements)
3. [Initial Setup](#initial-setup)
4. [Connecting to MockStang](#connecting-to-mockstang)
5. [Web Dashboard](#web-dashboard)
6. [Driving Simulator](#driving-simulator)
7. [MIL & Diagnostic Codes](#mil--diagnostic-codes)
8. [Manual Parameter Control](#manual-parameter-control)
9. [Settings & Configuration](#settings--configuration)
10. [OBD-II Protocol Support](#obd-ii-protocol-support)
11. [Troubleshooting](#troubleshooting)

---

## Introduction

MockStang is a WiFi-based OBD-II emulator that mimics the behavior of a vGate iCar Pro adapter. It's designed for testing and developing OBD-II applications without needing a real vehicle.

**Key Features:**
- 🚗 Realistic driving simulator with 4 aggressiveness levels
- 🔧 Full ELM327 v1.5 protocol support
- 💡 MIL (Check Engine Light) and DTC management
- 🌐 Web-based control dashboard
- 📱 Support for 2 simultaneous WiFi connections
- 💾 Persistent configuration storage (EEPROM)
- 🔄 Real-time parameter updates via WebSocket

---

## Hardware Requirements

### Required Hardware
- **ESP-01** or **ESP-01S** module (ESP8266-based)
- USB-to-Serial adapter (for programming)
- 3.3V power supply

### Recommended Specifications
- **Flash Memory**: 1MB minimum
- **Upload Speed**: 57600 baud (for ESP-01 compatibility)

### Development Tools
- PlatformIO (recommended) or Arduino IDE
- USB cable
- Computer with WiFi capability

---

## Initial Setup

### 1. Flash the Firmware

Using PlatformIO:
```bash
cd MockStang
pio run --target upload
```

Using Arduino IDE:
1. Open `src/mockstang.ino`
2. Select board: "Generic ESP8266 Module"
3. Set upload speed to 57600 baud
4. Click Upload

### 2. First Boot

On first boot, MockStang will:
- Generate a WiFi SSID based on MAC address: `iCAR_PRO_XXXX`
- Create an open WiFi network (no password)
- Configure IP address: `192.168.0.10`
- Start ELM327 server on port: `35000`
- Start web server on port: `80`

### 3. Default Configuration

**Default Vehicle State:**
- RPM: 850 (idle)
- Speed: 0 km/h
- Coolant Temp: 90°C
- Intake Temp: 25°C
- Throttle: 0%
- MAF: 2.5 g/s
- Fuel Level: 75%

**Default VIN:** `1ZVBP8AM5D5123456`

---

## Connecting to MockStang

### WiFi Connection

1. **Find the Network**
   - Look for WiFi network: `iCAR_PRO_XXXX` (where XXXX = last 2 MAC octets)
   - Network is open by default (no password)

2. **Connect Device**
   - Phone, tablet, or laptop
   - Join the `iCAR_PRO_XXXX` network
   - Device IP will be assigned automatically (192.168.0.x)

3. **Access Web Dashboard**
   - Open browser
   - Navigate to: `http://192.168.0.10`
   - Dashboard loads automatically

### OBD-II Application Connection

For OBD apps (like OBD Auto Doctor, Torque, etc.):

1. **Connection Type**: WiFi/Network
2. **IP Address**: `192.168.0.10`
3. **Port**: `35000`
4. **Protocol**: ISO 15765-4 (CAN 11/500)

---

## Web Dashboard

### Overview

The web dashboard provides real-time control and monitoring of MockStang. Access at `http://192.168.0.10`.

### Dashboard Sections

#### 1. Connection Status Card

**Elements:**
- **WebSocket Status Indicator**:
  - 🟢 Green "Connected" = Live connection
  - 🔴 Red "Disconnected" = No connection
- **Reset Runtime Button**: Resets engine runtime counter to 0
- **Clear Log Button**: Clears the OBD-II activity monitor
- **⚙️ Settings Button**: Opens configuration page

**Usage:**
- Monitor connection status to ensure real-time updates
- Use Reset Runtime when starting a new test session
- Clear Log to remove old activity data

---

#### 2. Driving Simulator Card

**Purpose:** Automatically animate vehicle parameters to simulate realistic driving scenarios.

**Mode Buttons:**

| Button | Description | Duration |
|--------|-------------|----------|
| **OFF** | Manual control only, no automation | N/A |
| **GENTLE** | Cold start, gentle acceleration (0-50 km/h) | Continuous |
| **NORMAL** | Normal driving cycle (accel, cruise, decel) | 22 seconds |
| **SPORT** | Aggressive driving with hard acceleration | 20 seconds |
| **DRAG RACE** | Full throttle drag race with launch | 18 seconds |

**Visual Feedback:**
- Active mode button turns green
- Status display shows current mode and parameters
- All parameters update automatically in real-time

**Detailed Mode Behavior:**

**GENTLE Mode:**
- Acceleration: 0-50 km/h in 5 seconds
- RPM: 850-2850 during accel, settles to 2000
- Throttle: 30% ±4% (realistic fluctuation)
- Coolant: Warms from 50°C to 90°C
- Behavior: Gentle driver making small corrections
- Best for: Testing cold start and warm-up behavior

**NORMAL Mode:**
- Acceleration: 0-80 km/h in 7 seconds
- Cruise: 10 seconds at 80 km/h, 2500 RPM
- Deceleration: 5 seconds to stop
- Throttle: 50% accel, 25% ±3% cruise
- Behavior: Regular driver with typical variations
- Best for: Testing everyday driving data logging

**SPORT Mode:**
- Acceleration: 0-120 km/h in 8 seconds
- RPM: Up to 6500 during acceleration
- Cruise: 8 seconds at 120 km/h, 3500 RPM
- Hard braking: 4 seconds to stop
- Throttle: 85% ±5% (aggressive variations)
- Behavior: Enthusiastic driver with quick inputs
- Best for: Testing high-performance scenarios

**DRAG RACE Mode:**
- Launch: 1 second with wheel spin (3000-5000 RPM)
- Acceleration: 0-180 km/h in 12 seconds total
- RPM: Up to 7000, with ±200 RPM at launch
- Braking: 6 seconds hard deceleration
- Throttle: 100% throughout acceleration
- Behavior: Maximum performance with launch vibration
- Best for: Testing extreme conditions and rapid changes

**Tips:**
- Click OFF to stop simulation and return to manual control
- Parameters continue to fluctuate realistically (noise simulation)
- Manual slider adjustments are disabled while simulator is active
- Perfect for long-duration testing without manual input

---

#### 3. Car Parameters Card

**Purpose:** Manual control of individual vehicle parameters when simulator is OFF.

**Sliders:**

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **RPM** | 0-7000 | 850 | Engine revolutions per minute |
| **Speed** | 0-200 km/h | 0 | Vehicle speed |
| **Coolant Temp** | -40°C to 150°C | 90°C | Engine coolant temperature |
| **Intake Temp** | -40°C to 150°C | 25°C | Intake air temperature |
| **Throttle** | 0-100% | 0% | Throttle position |
| **MAF** | 0-1000 (×0.01) | 2.50 g/s | Mass air flow sensor |
| **Fuel Level** | 0-100% | 75% | Fuel tank level |
| **Barometric** | 80-110 kPa | 101 kPa | Barometric pressure |

**Usage:**
1. Ensure Driving Simulator is set to OFF
2. Drag slider to adjust value
3. Current value displays on right
4. Changes take effect immediately
5. OBD apps see updated values instantly

**Tips:**
- Use sliders for precise control during debugging
- MAF displays as grams/second (divided by 100)
- Temperature values stored as °C internally, converted for OBD
- All changes broadcast via WebSocket

---

#### 4. MIL & Diagnostic Codes Card

**Purpose:** Simulate Check Engine Light and diagnostic trouble codes.

**Check Engine Light Control:**
- **OFF Button** (Gray): MIL off, no warning light
- **ON Button** (Red): MIL on, simulates active fault
- **Status Display**: Shows number of stored codes

**Add DTC Section:**

**DTC Type Selector:**
- **P0xxx**: Powertrain (SAE defined)
- **P1xxx**: Powertrain (manufacturer specific)
- **P2xxx**: Powertrain (SAE defined)
- **P3xxx**: Powertrain (SAE defined)
- **C0xxx**: Chassis
- **B0xxx**: Body
- **U0xxx**: Network

**Code Input:**
- Enter 4-digit hex code (e.g., "0420" for P0420)
- Click "Add Code" to store
- Maximum 8 DTCs can be stored

**Common Test Codes:**
- **P0420**: Catalyst System Efficiency Below Threshold
- **P0171**: System Too Lean (Bank 1)
- **P0300**: Random/Multiple Cylinder Misfire
- **P0442**: EVAP Leak Detected (small)
- **C0050**: Right Front Wheel Speed Sensor Circuit
- **B0001**: Driver Airbag Circuit

**Stored DTCs List:**
- Shows all currently stored codes
- "Remove" button deletes individual code
- "Clear All DTCs" button removes all codes and turns off MIL

**OBD-II Integration:**
- Mode 01 PID 01: Reports MIL status and DTC count
- Mode 03: Returns stored DTC list
- Mode 04: Clears all DTCs (same as "Clear All" button)
- Mode 07: Reports pending DTCs (currently returns none)

**Automatic Behavior:**
- Adding a DTC automatically turns on MIL
- Removing last DTC automatically turns off MIL
- Manual MIL toggle available for testing

---

#### 5. OBD-II Activity Monitor

**Purpose:** Real-time log of all OBD-II commands and responses.

**Display Format:**
```
[HH:MM:SS] ← 0100
[HH:MM:SS] → 4100B81B8003\r\r>
```

**Color Coding:**
- 🟢 Green: Commands received (←)
- 🔵 Blue: Responses sent (→)
- 🕐 Gray: Timestamp

**Usage:**
- Monitor protocol communication
- Debug command/response pairs
- Verify correct data formatting
- See escape characters (\r) in responses

**Tips:**
- Scrolls automatically to latest entry
- Click "Clear Log" to reset
- Useful for troubleshooting connection issues
- Shows raw ELM327 protocol data

---

## Settings & Configuration

### Accessing Settings

1. Click "⚙️ Settings" button on dashboard
2. Or navigate to: `http://192.168.0.10/settings`

### Configuration Sections

#### Network Settings

**Custom SSID:**
- Toggle: Enable/disable custom SSID
- Input: Enter custom network name
- Default: `iCAR_PRO_XXXX` (MAC-based)

**WiFi Password:**
- Toggle: Enable/disable password protection
- Input: Enter password (WPA2)
- Default: No password (open network)

**IP Address:**
- Format: XXX.XXX.XXX.XXX
- Default: `192.168.0.10`
- Range: 192.168.0.1 - 192.168.0.254

**Subnet Mask:**
- Default: `255.255.255.0`
- Typically unchanged

**Gateway:**
- Default: `192.168.0.10` (same as IP)
- For standalone operation

#### Vehicle Information

**VIN (Vehicle Identification Number):**
- Length: Exactly 17 characters
- Format: Alphanumeric
- Default: `1ZVBP8AM5D5123456`
- Returned via Mode 09 PID 02

**Usage:**
- Some OBD apps require valid VIN
- Used for vehicle identification
- Stored in EEPROM

#### Default PID Values

Set starting values for all parameters:

| Setting | Range | Default |
|---------|-------|---------|
| Default RPM | 0-7000 | 850 |
| Default Speed | 0-200 km/h | 0 |
| Default Coolant Temp | -40 to 150°C | 90 |
| Default Intake Temp | -40 to 150°C | 25 |
| Default Throttle | 0-100% | 0 |
| Default MAF | 0-1000 | 250 |
| Default Fuel Level | 0-100% | 75 |
| Default Barometric | 80-110 kPa | 101 |

**Purpose:**
- Set initial state on boot
- Restore these values on reset
- Customize for different test scenarios

### Configuration Actions

**Save Configuration:**
- Writes settings to EEPROM
- Survives power cycles
- Takes effect immediately

**Factory Reset:**
- Restores all defaults
- Clears EEPROM
- Requires restart

**Export Config (JSON):**
- Downloads current config as JSON
- Useful for backup
- Can be imported later (manually edit EEPROM)

---

## OBD-II Protocol Support

### Supported Modes

MockStang implements the following OBD-II modes:

#### Mode 01: Current Data

**Supported PIDs:**

| PID | Description | Data Format |
|-----|-------------|-------------|
| 0x00 | Supported PIDs [01-20] | Bitmap |
| 0x01 | Monitor status since DTCs cleared | MIL status + DTC count |
| 0x03 | Fuel system status | Closed loop |
| 0x04 | Calculated engine load | Percentage |
| 0x05 | Engine coolant temperature | °C + 40 |
| 0x0C | Engine RPM | RPM × 4 |
| 0x0D | Vehicle speed | km/h |
| 0x0F | Intake air temperature | °C + 40 |
| 0x10 | MAF air flow rate | g/s × 100 |
| 0x11 | Throttle position | Percentage |
| 0x1F | Run time since engine start | Seconds |
| 0x20 | Supported PIDs [21-40] | Bitmap |
| 0x21 | Distance traveled with MIL on | km |
| 0x2F | Fuel tank level input | Percentage |
| 0x33 | Barometric pressure | kPa |
| 0x40 | Supported PIDs [41-60] | Bitmap |

#### Mode 03: Show Stored DTCs

- Returns all stored diagnostic trouble codes
- Format: 2 bytes per DTC
- Shows count + codes
- Example: `03 0420 0171` (2 codes: P0420, P0171)

#### Mode 04: Clear DTCs and MIL

- Clears all stored codes
- Turns off MIL
- Resets MIL distance
- Response: `44` (acknowledge)

#### Mode 07: Show Pending DTCs

- Returns pending codes (not yet confirmed)
- Currently returns "NO DATA"
- Reserved for future enhancement

#### Mode 09: Vehicle Information

**Supported PIDs:**

| PID | Description | Data Format |
|-----|-------------|-------------|
| 0x00 | Supported PIDs [01-20] | Bitmap |
| 0x02 | Vehicle Identification Number | 17 ASCII chars (multi-line) |
| 0x0A | ECU name | ASCII string |

**VIN Format:**
- Split across 3 CAN messages
- Message 1: Byte count + chars 1-5
- Message 2: Sequence 02 + chars 6-12
- Message 3: Sequence 03 + chars 13-17

### ELM327 AT Commands

**Supported Commands:**

| Command | Description | Response |
|---------|-------------|----------|
| ATZ | Reset adapter | "ELM327 v1.5" |
| ATI | Print ID | "ELM327 v1.5" |
| AT@1 | Display device description | "MockStang ESP-01S" |
| AT@2 | Display device identifier | "MockStang ESP-01S" |
| ATD | Set defaults | "OK" |
| ATE0/ATE1 | Echo off/on | "OK" |
| ATL0/ATL1 | Linefeeds off/on | "OK" |
| ATS0/ATS1 | Spaces off/on | "OK" |
| ATH0/ATH1 | Headers off/on | "OK" |
| ATM0/ATM1 | Memory off/on | "OK" |
| ATSP[n] | Set protocol | "OK" |
| ATTP[n] | Try protocol | "OK" |
| ATST[XX] | Set timeout | "OK" |
| ATDP | Describe protocol | "ISO 15765-4 (CAN 11/500)" |
| ATDPN | Describe protocol number | "6" |
| ATAT[0-2] | Adaptive timing | "OK" |
| ATWS | Warm start | "ELM327 v1.5" |
| ATSW[XX] | Set wakeup message | "OK" |
| ATCAF[0-1] | CAN auto formatting | "OK" |
| ATRV | Read voltage | "12.8V" |

### Protocol Details

**CAN Format:**
- Protocol: ISO 15765-4 (CAN 11/500)
- Baud Rate: 500 kbps
- CAN ID: 0x7E8 (ECU response)
- Length Byte: Included when headers on
- Format: `[Header] [Length] [Mode] [PID] [Data...]`

**Response Timing:**
- AT Commands: Instant
- OBD Queries: 35ms delay (simulates real ECU)
- Timeout: Configurable via ATST

---

## Troubleshooting

### Connection Issues

**Problem: Can't find WiFi network**
- Verify ESP-01 is powered (3.3V)
- Check serial monitor for boot messages
- SSID should appear as `iCAR_PRO_XXXX`
- Try restarting ESP-01

**Problem: Can't connect to network**
- Ensure device WiFi is enabled
- Network is open (no password by default)
- Check for WiFi conflicts (channel 6)
- Maximum 2 devices can connect

**Problem: Can't access web dashboard**
- Verify connected to correct network
- Check IP: Should be `192.168.0.10`
- Try different browser
- Clear browser cache

### OBD App Issues

**Problem: App says "No adapter found"**
- Verify IP address: `192.168.0.10`
- Port should be: `35000`
- Check app supports WiFi/network connection
- Try manual connection configuration

**Problem: App connects then disconnects**
- Check serial monitor for errors
- Verify protocol: ISO 15765-4 (CAN 11/500)
- Try resetting adapter (ATZ command)
- Update firmware to latest version

**Problem: Incorrect data values**
- Check PID encoding (e.g., RPM × 4)
- Verify Mode 01 support in app
- Check for spaces/headers settings (ATS/ATH)
- Monitor activity log for protocol errors

### Web Dashboard Issues

**Problem: WebSocket shows "Disconnected"**
- Refresh browser page
- Check network connection
- Verify ESP-01 hasn't crashed (serial monitor)
- Try different browser

**Problem: Sliders don't work**
- Check if Driving Simulator is active (turn OFF)
- Verify WebSocket is connected
- Refresh page
- Check JavaScript console for errors

**Problem: Driving Simulator stuck**
- Click OFF button to stop
- Refresh page
- Power cycle ESP-01 if needed
- Check serial monitor for watchdog resets

### Hardware Issues

**Problem: ESP-01 won't program**
- Reduce upload speed to 57600 baud
- Check GPIO0 pulled to ground (programming mode)
- Verify USB-to-Serial adapter connections
- Try different USB cable/port

**Problem: ESP-01 boots but crashes**
- Check power supply (3.3V, sufficient current)
- Monitor serial output for exceptions
- Verify correct flash size (1MB)
- Check for memory leaks (long running)

**Problem: Intermittent disconnections**
- Verify stable 3.3V power supply
- Check for WiFi interference
- Reduce distance to ESP-01
- Try different WiFi channel (settings)

### Data Issues

**Problem: Parameters don't change**
- Verify WebSocket connected
- Check if simulator is running
- Try manual slider adjustment
- Power cycle and retry

**Problem: Unrealistic values**
- Check PID calculations (see OBD-II spec)
- Verify temperature offsets (+40)
- Check RPM encoding (× 4)
- Review Mode 01 PID data format

**Problem: DTC codes not working**
- Enter exactly 4 hex digits
- Check DTC type (P0xxx, C0xxx, etc.)
- Verify OBD app supports Mode 03
- Check activity monitor for commands

---

## Advanced Usage

### Serial Monitor

Connect serial terminal at 115200 baud to see:
- Boot messages
- Network configuration
- Connection attempts
- Command/response pairs
- Debug information

**Useful for:**
- Troubleshooting protocol issues
- Verifying configuration
- Monitoring system health
- Debugging custom modifications

### Multiple Devices

MockStang supports 2 simultaneous WiFi connections:
- **Device 1**: OBD application
- **Device 2**: Web dashboard for monitoring

**Setup:**
1. Connect Device 1 (phone with OBD app)
2. Connect Device 2 (laptop with web browser)
3. Control parameters while app is connected
4. Monitor activity in real-time

### Custom Scenarios

Create custom test scenarios by combining features:

**Cold Start Test:**
1. Set coolant temp to 0°C
2. Set RPM to 1200
3. Set intake temp to -10°C
4. Start GENTLE mode
5. Watch engine warm up

**High Performance Test:**
1. Set coolant temp to 105°C
2. Add P0420 DTC
3. Start SPORT mode
4. Test app behavior under stress

**Fault Condition Test:**
1. Add multiple DTCs (P0171, P0174, P0300)
2. Set MIL on
3. Set MIL distance to 500 km
4. Test diagnostic functionality

---

## Specifications

### Technical Specifications

**Hardware:**
- Platform: ESP8266 (ESP-01/ESP-01S)
- Flash: 1MB minimum
- RAM: ~30KB used
- Clock: 80MHz

**Network:**
- WiFi: 802.11 b/g/n (2.4GHz)
- Mode: Access Point
- Security: Open or WPA2-PSK
- Connections: 2 simultaneous
- IP: 192.168.0.10 (default)

**Protocols:**
- ELM327: v1.5
- OBD-II: ISO 15765-4 (CAN 11/500)
- TCP Port: 35000 (OBD)
- HTTP Port: 80 (Web)
- WebSocket: Real-time updates

**Performance:**
- Response Time: <50ms
- Update Rate: 1ms (driving simulator)
- WebSocket Latency: <20ms
- Max DTCs: 8
- Max PIDs: 15 (Mode 01)

### Limitations

**Current Limitations:**
- No Bluetooth support (WiFi only)
- Single ECU simulation (0x7E8)
- No multi-line PID responses (except VIN)
- No freeze frame data (Mode 02)
- No Mode 05/06 support
- Pending DTCs always empty (Mode 07)
- No protocol auto-detection

**Memory Constraints:**
- EEPROM: 512 bytes (configuration)
- WebSocket buffer: 256 bytes
- Command buffer: 64 bytes
- Response buffer: 128 bytes

---

## Appendix

### Default Configuration Values

```json
{
  "network": {
    "ssid_prefix": "iCAR_PRO_",
    "password": "",
    "ip": "192.168.0.10",
    "gateway": "192.168.0.10",
    "subnet": "255.255.255.0"
  },
  "vehicle": {
    "vin": "1ZVBP8AM5D5123456",
    "rpm": 850,
    "speed": 0,
    "coolant_temp": 90,
    "intake_temp": 25,
    "throttle": 0,
    "maf": 250,
    "fuel_level": 75,
    "barometric": 101
  },
  "obd": {
    "protocol": "ISO 15765-4 (CAN 11/500)",
    "port": 35000,
    "device_id": "MockStang ESP-01S",
    "voltage": "12.8V"
  }
}
```

### Quick Reference

**Common Commands:**
- Web Dashboard: `http://192.168.0.10`
- Settings Page: `http://192.168.0.10/settings`
- OBD Connection: `192.168.0.10:35000`
- Reset Runtime: Dashboard → Reset Runtime button
- Factory Reset: Settings → Factory Reset
- Clear DTCs: Dashboard → Clear All DTCs

**Support Resources:**
- GitHub: [MockStang Repository](https://github.com/John-MustangGT/MockStang)
- Issues: Report bugs via GitHub Issues
- TODO: See TODO.md for planned features

---

**Document Version:** 1.0
**Last Updated:** 2026-01-12
**Firmware Version:** Compatible with latest main branch

For questions, issues, or contributions, visit the GitHub repository.
