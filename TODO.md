# MockStang Feature Roadmap

This document tracks potential features and enhancements for MockStang.

## ✅ Completed Features

- [x] ELM327 v1.5 protocol emulation
- [x] WiFi AP mode with vGate iCar Pro compatibility
- [x] Web dashboard with real-time parameter control
- [x] Mode 01 (Current Data) - 11 PIDs
- [x] Mode 03 (Read DTCs)
- [x] Mode 04 (Clear DTCs)
- [x] Mode 07 (Pending DTCs)
- [x] Mode 09 (Vehicle Information - VIN, ECU Name)
- [x] MIL (Check Engine Light) management
- [x] DTC storage and management (up to 8 codes)
- [x] EEPROM configuration persistence
- [x] Multi-device support (2 simultaneous connections)
- [x] WebSocket for real-time updates

## 🚧 In Progress

- [ ] **Driving Simulator** (High Priority)
  - Automated parameter changes simulating real driving
  - Multiple aggressiveness levels (gentle, normal, sport, drag race)
  - Realistic transitions and constraints

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

#### Additional Mode 01 PIDs
**Value**: Better compatibility with OBD apps
**Effort**: Low
**Missing PIDs**:
- `0x06`: Short term fuel trim
- `0x07`: Long term fuel trim
- `0x0E`: Timing advance
- `0x42`: Control module voltage
- `0x46`: Ambient air temperature
- `0x51`: Fuel type

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

#### Connection Statistics Dashboard
**Value**: Performance testing and debugging
**Effort**: Low
**Metrics**:
- Commands per second
- Connection duration
- Most queried PIDs
- Average response time
- Total commands processed

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
- **Bluetooth Support**: Classic Bluetooth or BLE mode
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

**Last Updated**: 2026-01-12
**Project**: MockStang WiFi OBD-II Emulator
