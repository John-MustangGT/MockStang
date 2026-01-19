# MockStang Feature Roadmap

This document tracks potential features and enhancements for MockStang.

## ✅ Completed Features

- [x] ELM327 v1.5 protocol emulation
- [x] WiFi AP mode with vGate iCar Pro compatibility
- [x] Web dashboard with real-time parameter control
- [x] Mode 01 (Current Data) - 27 PIDs (expanded from 11)
- [x] Mode 03 (Read DTCs)
- [x] Mode 04 (Clear DTCs)
- [x] Mode 07 (Pending DTCs)
- [x] Mode 09 (Vehicle Information - VIN, ECU Name)
- [x] MIL (Check Engine Light) management
- [x] DTC storage and management (up to 8 codes)
- [x] EEPROM configuration persistence
- [x] Multi-device support (2 simultaneous connections on ESP-01S, 4 on ESP32-S3)
- [x] WebSocket for real-time updates
- [x] **Driving Simulator** with 4 aggressiveness levels (GENTLE, NORMAL, SPORT, DRAG)
- [x] **Live Dashboard Updates** - Sliders automatically update during driving simulation
- [x] **Connection Statistics Dashboard** - Real-time monitoring of OBD-II app behavior
- [x] **Optional Serial Logging** - Configurable CMD/RESP logging
- [x] **Multi-Platform Support** - ESP-01S (WiFi) and ESP32-S3 (WiFi+BLE+Display)
- [x] **BLE Peripheral Mode** - Bluetooth Low Energy support (ESP32-S3)
- [x] **TFT Display Support** - Real-time parameter visualization (ESP32-S3)
- [x] **Expanded Mode 01 PIDs** - Fuel trim, timing advance, MAP, O2 sensors, battery voltage, oil temp, etc.

## 🚧 In Progress

- [ ] None currently

## 📋 Planned Features

### High Priority

#### Driving Scenario Presets
**Value**: Quick testing of different vehicle states
**Effort**: Low
**Implementation**: One-click buttons to set multiple parameters at once

Presets:
- **Idle**: 850 RPM, 0 speed, 0% throttle, normal temp
- **Cruising**: 2500 RPM, 100 km/h, 20% throttle, normal temp
- **Highway**: 3000 RPM, 120 km/h, 30% throttle
- **Acceleration**: Rising RPM/speed pattern
- **Cold Start**: 1200 RPM, low coolant temp (-10°C), high intake temp
- **Hot Idle**: 850 RPM, high coolant temp (105°C)
- **Fault Condition**: MIL on with P0420 catalyst efficiency code
- **Multiple Faults**: Several DTCs active (P0171, P0174, P0300)

### Medium Priority

#### Freeze Frame Data (Mode 02)
**Value**: Proper DTC testing with snapshot data
**Effort**: Medium
**Description**: When a DTC is stored, capture vehicle conditions at that moment (RPM, speed, temps, etc.). Return this data when Mode 02 is queried.

#### Save/Load Vehicle Profiles
**Value**: Share and quickly switch test scenarios
**Effort**: Medium
**Features**:
- Export current state as JSON file
- Import predefined vehicle profiles
- Built-in profiles for different vehicle types (sedan, truck, sports car)
- Share test scenarios between developers

#### Response Delay Simulation
**Value**: Test app behavior with slow/unreliable adapters
**Effort**: Low
**Features**:
- Adjustable delay slider (0-500ms)
- Random timeout simulation
- "Bad adapter" mode with intermittent failures
- Packet loss simulation

### Lower Priority

#### Mode 05 Support (O2 Sensor Monitoring)
**Value**: Complete OBD-II protocol coverage
**Effort**: Medium
**PIDs**: Test results for oxygen sensor monitoring

#### Mode 06 Support (Test Results)
**Value**: Advanced diagnostic testing
**Effort**: High
**Description**: On-board diagnostic test results for specific monitored systems

#### CAN Message Logging
**Value**: Debug protocol issues
**Effort**: Low
**Features**:
- Log all raw CAN messages
- Export to CSV or candump format
- Timestamp each message

#### Replay Captured Sessions
**Value**: Test with real vehicle data
**Effort**: Medium
**Description**: Import CAN bus captures from real vehicles and replay them through MockStang

#### GPS Simulation
**Value**: Test location-based features
**Effort**: Medium
**Description**: Simulate GPS coordinates changing during driving scenarios

#### Fuel Consumption Calculation
**Value**: Test fuel economy tracking
**Effort**: Low
**Description**: Calculate realistic fuel consumption based on RPM, speed, MAF

#### Advanced Fault Scenarios
**Value**: Test complex diagnostic cases
**Effort**: Medium
**Examples**:
- Misfire on specific cylinder
- Progressive sensor failure
- Intermittent faults
- Multiple related codes

#### OBD-II Protocol Switching
**Value**: Test protocol auto-detection
**Effort**: High
**Description**: Support multiple protocols (ISO 9141, KWP2000, etc.), not just CAN

## 💡 Ideas for Future Consideration

- **Mobile App**: Native iOS/Android app for remote control
- **Cloud Sync**: Share configurations across devices
- **Scripting Engine**: Lua or JavaScript for custom scenarios
- **Multi-ECU Simulation**: Simulate multiple ECUs on different CAN IDs
- **Security Features**: Test encrypted/authenticated OBD protocols
- **Emissions Testing**: Simulate readiness monitors and emissions tests

## 📝 Notes

- Features are prioritized based on testing utility and implementation effort
- High priority items are those most commonly needed for app development
- Medium priority items enhance realism and edge case testing
- Lower priority items are "nice to have" for specific use cases

## Contributing

If you implement any of these features or have suggestions for new ones, please:
1. Update this TODO.md
2. Document the feature in README.md
3. Add usage examples

---

**Last Updated**: 2026-01-13
**Project**: MockStang WiFi/BLE OBD-II Emulator
