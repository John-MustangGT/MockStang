#ifndef PID_HANDLER_H
#define PID_HANDLER_H

#include <Arduino.h>
#include "config.h"
#include "elm327_protocol.h"
#include "config_manager.h"

class PIDHandler {
private:
    CarState currentState;
    ELM327Protocol* elm;
    ConfigManager* config;
    unsigned long startTime;

    // Driving simulator state
    DriveMode driveMode;
    unsigned long driveStartTime;
    uint8_t drivePhase;  // 0=accel, 1=cruise, 2=decel, 3=stopped

public:
    PIDHandler(ELM327Protocol* elmProtocol, ConfigManager* configMgr) : elm(elmProtocol), config(configMgr) {
        currentState = DEFAULT_CAR_STATE;
        startTime = millis();
        driveMode = DRIVE_OFF;
        driveStartTime = 0;
        drivePhase = 0;
    }

    // Update a specific car parameter
    void updateRPM(uint16_t rpm) { currentState.rpm = rpm; }
    void updateSpeed(uint8_t speed) { currentState.speed = speed; }
    void updateCoolantTemp(uint8_t temp) { currentState.coolant_temp = temp; }
    void updateIntakeTemp(uint8_t temp) { currentState.intake_temp = temp; }
    void updateThrottle(uint8_t throttle) { currentState.throttle = throttle; }
    void updateMAF(uint16_t maf) { currentState.maf = maf; }
    void updateFuelLevel(uint8_t level) { currentState.fuel_level = level; }
    void updateBarometric(uint8_t baro) { currentState.barometric = baro; }
    void updateMILDistance(uint16_t dist) { currentState.mil_distance = dist; }

    // MIL and DTC management
    void setMIL(bool on) { currentState.mil_on = on; }
    bool getMIL() { return currentState.mil_on; }

    // Add a DTC (P0xxx, P2xxx, etc.)
    bool addDTC(uint16_t dtc) {
        if (currentState.dtc_count >= MAX_DTCS) return false;
        // Check if DTC already exists
        for (uint8_t i = 0; i < currentState.dtc_count; i++) {
            if (currentState.dtcs[i] == dtc) return false;
        }
        currentState.dtcs[currentState.dtc_count++] = dtc;
        currentState.mil_on = true; // Turn on MIL when DTC added
        return true;
    }

    // Remove a specific DTC
    bool removeDTC(uint16_t dtc) {
        for (uint8_t i = 0; i < currentState.dtc_count; i++) {
            if (currentState.dtcs[i] == dtc) {
                // Shift remaining DTCs down
                for (uint8_t j = i; j < currentState.dtc_count - 1; j++) {
                    currentState.dtcs[j] = currentState.dtcs[j + 1];
                }
                currentState.dtc_count--;
                if (currentState.dtc_count == 0) {
                    currentState.mil_on = false; // Turn off MIL when no DTCs
                }
                return true;
            }
        }
        return false;
    }

    // Clear all DTCs
    void clearDTCs() {
        currentState.dtc_count = 0;
        currentState.mil_on = false;
        currentState.mil_distance = 0;
        for (uint8_t i = 0; i < MAX_DTCS; i++) {
            currentState.dtcs[i] = 0;
        }
    }

    uint8_t getDTCCount() { return currentState.dtc_count; }
    uint16_t getDTC(uint8_t index) {
        if (index < currentState.dtc_count) {
            return currentState.dtcs[index];
        }
        return 0;
    }

    // Get current car state (for web interface)
    CarState getState() { return currentState; }

    // Update runtime counter
    void updateRuntime() {
        currentState.runtime = (millis() - startTime) / 1000;
    }

    // Reset runtime counter
    void resetRuntime() {
        startTime = millis();
        currentState.runtime = 0;
    }

    // Driving Simulator Control
    void setDriveMode(DriveMode mode) {
        driveMode = mode;
        driveStartTime = millis();
        drivePhase = 0;
        if (mode == DRIVE_OFF) {
            // Stop at current state
        } else {
            // Reset to idle state at start
            currentState.speed = 0;
            currentState.rpm = 850;
            currentState.throttle = 0;
            if (mode == DRIVE_GENTLE) {
                currentState.coolant_temp = 50;  // Cold engine
            } else {
                currentState.coolant_temp = 90;  // Warm engine
            }
        }
    }

    DriveMode getDriveMode() { return driveMode; }

    // Update driving simulator (call from main loop)
    void updateDrivingSimulator() {
        if (driveMode == DRIVE_OFF) return;

        unsigned long elapsed = millis() - driveStartTime;
        float elapsedSec = elapsed / 1000.0;

        switch (driveMode) {
            case DRIVE_GENTLE: {
                // 0-50 km/h in 5s, then hold, engine warming up
                // Add realistic throttle noise - small variations every 1-2 seconds
                float noisePhase = fmod(elapsedSec * 2.0, 6.28);  // 0-2π over ~3 seconds
                int8_t throttleNoise = (int8_t)(sin(noisePhase) * 4.0);  // +/- 4%
                int8_t rpmNoise = (int8_t)(sin(noisePhase * 1.3) * 50);  // +/- 50 RPM (different phase)

                if (drivePhase == 0) {  // Acceleration
                    if (elapsedSec < 5.0) {
                        currentState.speed = (uint8_t)(elapsedSec * 10);  // 0-50 km/h
                        currentState.rpm = 850 + (uint16_t)(elapsedSec * 400) + rpmNoise;  // 850-2850 RPM + noise
                        currentState.throttle = 30 + throttleNoise;  // 30% +/- 4%
                        // Engine warming up
                        currentState.coolant_temp = 50 + (uint8_t)(elapsedSec * 4);  // 50-70°C
                    } else {
                        drivePhase = 1;
                    }
                } else if (drivePhase == 1) {  // Cruise
                    currentState.speed = 50;
                    currentState.rpm = 2000 + rpmNoise;  // Slight RPM fluctuation
                    currentState.throttle = 15 + throttleNoise;  // 15% +/- 4%
                    // Continue warming
                    if (currentState.coolant_temp < 90) {
                        currentState.coolant_temp++;
                    }
                }
                currentState.maf = 200 + currentState.throttle * 10;
                break;
            }

            case DRIVE_NORMAL: {
                // 0-80 km/h in 7s, cruise, then slow to stop
                // Moderate driver adjustments - slower, smoother variations
                float noisePhase = fmod(elapsedSec * 1.5, 6.28);  // ~4 second cycle
                int8_t throttleNoise = (int8_t)(sin(noisePhase) * 3.0);  // +/- 3%
                int8_t rpmNoise = (int8_t)(sin(noisePhase * 1.2) * 40);  // +/- 40 RPM

                if (drivePhase == 0) {  // Acceleration
                    if (elapsedSec < 7.0) {
                        currentState.speed = (uint8_t)(elapsedSec * 11.4);  // 0-80 km/h
                        currentState.rpm = 850 + (uint16_t)(elapsedSec * 500) + rpmNoise;  // 850-4350 RPM
                        currentState.throttle = 50 + throttleNoise;
                    } else {
                        drivePhase = 1;
                    }
                } else if (drivePhase == 1) {  // Cruise for 10s
                    if (elapsedSec < 17.0) {
                        currentState.speed = 80;
                        currentState.rpm = 2500 + rpmNoise;
                        currentState.throttle = 25 + throttleNoise;
                    } else {
                        drivePhase = 2;
                    }
                } else if (drivePhase == 2) {  // Deceleration
                    float decelTime = elapsedSec - 17.0;
                    if (decelTime < 5.0) {
                        currentState.speed = 80 - (uint8_t)(decelTime * 16);  // 80-0 km/h
                        currentState.rpm = 2500 - (uint16_t)(decelTime * 330);  // 2500-850 RPM
                        currentState.throttle = 0;
                    } else {
                        drivePhase = 3;
                    }
                } else {  // Stopped
                    currentState.speed = 0;
                    currentState.rpm = 850 + (rpmNoise / 2);  // Small idle fluctuation
                    currentState.throttle = 0;
                }
                currentState.maf = 200 + currentState.throttle * 8;
                break;
            }

            case DRIVE_SPORT: {
                // 0-120 km/h in 8s, hard acceleration
                // Aggressive driver - sharper, faster variations
                float noisePhase = fmod(elapsedSec * 3.0, 6.28);  // ~2 second cycle (faster)
                int8_t throttleNoise = (int8_t)(sin(noisePhase) * 5.0);  // +/- 5% (more aggressive)
                int8_t rpmNoise = (int8_t)(sin(noisePhase * 1.4) * 80);  // +/- 80 RPM

                if (drivePhase == 0) {  // Acceleration
                    if (elapsedSec < 8.0) {
                        currentState.speed = (uint8_t)(elapsedSec * 15);  // 0-120 km/h
                        currentState.rpm = 1500 + (uint16_t)(elapsedSec * 625) + rpmNoise;  // 1500-6500 RPM
                        currentState.throttle = 85 + throttleNoise;
                    } else {
                        drivePhase = 1;
                    }
                } else if (drivePhase == 1) {  // Cruise for 8s
                    if (elapsedSec < 16.0) {
                        currentState.speed = 120;
                        currentState.rpm = 3500 + rpmNoise;
                        currentState.throttle = 40 + throttleNoise;
                    } else {
                        drivePhase = 2;
                    }
                } else if (drivePhase == 2) {  // Hard braking
                    float decelTime = elapsedSec - 16.0;
                    if (decelTime < 4.0) {
                        currentState.speed = 120 - (uint8_t)(decelTime * 30);  // 120-0 km/h
                        currentState.rpm = 3500 - (uint16_t)(decelTime * 662);  // 3500-850 RPM
                        currentState.throttle = 0;
                    } else {
                        drivePhase = 3;
                    }
                } else {  // Stopped
                    currentState.speed = 0;
                    currentState.rpm = 850 + rpmNoise;  // Even at idle, sporty engine fluctuates more
                    currentState.throttle = 0;
                }
                currentState.maf = 200 + currentState.throttle * 10;
                break;
            }

            case DRIVE_DRAG: {
                // Drag race: 0-180 km/h in 12s flat out, then hard brake
                // High frequency vibration at launch, then power delivery variations
                float noisePhase = fmod(elapsedSec * 5.0, 6.28);  // ~1.2 second cycle (very fast)

                if (drivePhase == 0) {  // Launch and acceleration
                    if (elapsedSec < 12.0) {
                        currentState.speed = (uint8_t)(elapsedSec * 15);  // 0-180 km/h
                        if (elapsedSec < 1.0) {
                            // Launch - wheel spin, high frequency vibration
                            int16_t launchNoise = (int16_t)(sin(noisePhase) * 200);  // +/- 200 RPM (wheel spin)
                            currentState.rpm = 3000 + (uint16_t)(elapsedSec * 2000) + launchNoise;  // 3000-5000 RPM
                            currentState.throttle = 100;  // Floored
                        } else {
                            // Full acceleration - power delivery variations
                            int16_t rpmNoise = (int16_t)(sin(noisePhase * 1.1) * 100);  // +/- 100 RPM
                            currentState.rpm = 2000 + (uint16_t)((elapsedSec - 1.0) * 454) + rpmNoise;  // 2000-7000 RPM
                            currentState.throttle = 100;  // Throttle pinned
                        }
                    } else {
                        drivePhase = 1;
                    }
                } else if (drivePhase == 1) {  // Hard braking
                    float brakeTime = elapsedSec - 12.0;
                    if (brakeTime < 6.0) {
                        currentState.speed = 180 - (uint8_t)(brakeTime * 30);  // 180-0 km/h
                        currentState.rpm = 7000 - (uint16_t)(brakeTime * 1025);  // 7000-850 RPM
                        currentState.throttle = 0;
                    } else {
                        drivePhase = 2;
                    }
                } else {  // Stopped
                    currentState.speed = 0;
                    currentState.rpm = 850;
                    currentState.throttle = 0;
                }
                currentState.maf = 200 + currentState.throttle * 12;
                break;
            }

            default:
                break;
        }

        // Calculate common PIDs for all drive modes
        if (driveMode != DRIVE_OFF) {
            // MAP (Manifold Absolute Pressure): vacuum at idle, increases with throttle
            // Idle: ~35 kPa, WOT: ~100 kPa (atmospheric)
            currentState.map = 35 + (currentState.throttle * 65 / 100);

            // Timing advance: varies with RPM and load
            // Idle: ~15°, cruise: ~30°, WOT: ~20°
            if (currentState.throttle > 80) {
                currentState.timing_advance = 15 + (currentState.rpm / 400);  // Less advance at WOT
            } else {
                currentState.timing_advance = 15 + (currentState.rpm / 250);  // More advance during cruise
            }
            if (currentState.timing_advance > 35) currentState.timing_advance = 35;

            // Fuel trims: slight variation based on throttle
            currentState.short_fuel_trim = (int8_t)(sin(elapsedSec) * 3);  // ±3% variation
            // Long fuel trim stays relatively constant (set in defaults)

            // O2 sensor voltage: cycles around 0.45V (stoichiometric)
            // 0.45V = 90 units (0.005V per unit)
            currentState.o2_voltage = 90 + (uint8_t)(sin(elapsedSec * 5) * 20);  // 0.35-0.55V cycling

            // EGR: active during cruise, off at idle and WOT
            if (currentState.speed > 30 && currentState.throttle > 10 && currentState.throttle < 60) {
                currentState.egr = 15 + (currentState.throttle / 4);  // 15-30% during cruise
            } else {
                currentState.egr = 0;  // Off at idle or WOT
            }

            // Battery voltage: slight variation during operation
            currentState.battery_voltage = 14000 + (uint16_t)(sin(elapsedSec * 0.5) * 300);  // 13.7-14.3V

            // Oil temperature: follows coolant but ~5° higher
            currentState.oil_temp = currentState.coolant_temp + 5;
            if (currentState.oil_temp > 100) currentState.oil_temp = 100;

            // Fuel pressure stays relatively constant (set in defaults)
            // Distance tracking would increment here if we tracked actual movement
        }
    }

    // Handle OBD-II mode 01 request
    String handleMode01(uint8_t pid) {
        uint8_t data[4];
        uint8_t dataLen = 0;

        switch (pid) {
            case 0x00:  // PIDs supported [01-20]
                // Bitmap of supported PIDs
                // We support: 0x01, 0x03, 0x04, 0x05, 0x06, 0x07, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x13, 0x14, 0x1F
                data[0] = 0b10111100;  // 01-08: 0x01, 0x03, 0x04, 0x05, 0x06, 0x07
                data[1] = 0b00111111;  // 09-10: 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
                data[2] = 0b10110000;  // 11-18: 0x11, 0x13, 0x14
                data[3] = 0b00000011;  // 19-20: 0x1F, 0x20 (supports next range)
                dataLen = 4;
                break;

            case 0x01: {  // Monitor status since DTCs cleared
                // Byte A: MIL status and DTC count
                // Bit 7: MIL status (0=off, 1=on)
                // Bits 6-0: Number of DTCs (0-127)
                uint8_t dtcCount = currentState.dtc_count > 127 ? 127 : currentState.dtc_count;
                data[0] = (currentState.mil_on ? 0x80 : 0x00) | dtcCount;
                data[1] = 0x07;  // Test available flags
                data[2] = 0x65;  // Test complete flags
                data[3] = 0x04;  // Additional flags
                dataLen = 4;
                break;
            }

            case 0x03:  // Fuel system status
                // Closed loop, using O2 sensor
                data[0] = 0x02;  // System 1: Closed loop
                data[1] = 0x00;  // System 2: Not available
                dataLen = 2;
                break;

            case 0x04:  // Calculated engine load
                // Return 20% load (arbitrary)
                data[0] = (20 * 255) / 100;
                dataLen = 1;
                break;

            case 0x05:  // Engine coolant temperature
                data[0] = currentState.coolant_temp + 40;  // °C + 40
                dataLen = 1;
                break;

            case 0x06:  // Short term fuel trim - Bank 1
                // Formula: ((A-128) * 100) / 128 = percent
                // Range: -100% (lean) to +99.2% (rich)
                data[0] = ((int16_t)currentState.short_fuel_trim * 128 / 100) + 128;
                dataLen = 1;
                break;

            case 0x07:  // Long term fuel trim - Bank 1
                // Formula: ((A-128) * 100) / 128 = percent
                // Range: -100% (lean) to +99.2% (rich)
                data[0] = ((int16_t)currentState.long_fuel_trim * 128 / 100) + 128;
                dataLen = 1;
                break;

            case 0x0B:  // Intake manifold absolute pressure
                // Direct kPa value (0-255)
                data[0] = currentState.map;
                dataLen = 1;
                break;

            case 0x0C: {  // Engine RPM
                // RPM is encoded as (RPM * 4) as 16-bit big-endian
                // Formula: RPM = ((A * 256) + B) / 4
                uint16_t encodedRPM = currentState.rpm * 4;
                data[0] = encodedRPM >> 8;
                data[1] = encodedRPM & 0xFF;
                dataLen = 2;
                break;
            }

            case 0x0D:  // Vehicle speed
                data[0] = currentState.speed;
                dataLen = 1;
                break;

            case 0x0E:  // Timing advance
                // Formula: A/2 - 64 = degrees
                // Range: -64° to +63.5°
                data[0] = ((int16_t)currentState.timing_advance + 64) * 2;
                dataLen = 1;
                break;

            case 0x0F:  // Intake air temperature
                data[0] = currentState.intake_temp + 40;  // °C + 40
                dataLen = 1;
                break;

            case 0x10:  // MAF air flow rate
                data[0] = currentState.maf >> 8;
                data[1] = currentState.maf & 0xFF;
                dataLen = 2;
                break;

            case 0x11:  // Throttle position
                data[0] = (currentState.throttle * 255) / 100;
                dataLen = 1;
                break;

            case 0x13:  // O2 sensors present
                // 2 banks × 4 sensors per bank bitmap
                // We'll indicate bank 1 sensor 1 is present (bit 0)
                data[0] = 0x01;  // Bank 1 - Sensor 1 present
                dataLen = 1;
                break;

            case 0x14:  // O2 Sensor 1 (Bank 1, Sensor 1)
                // Byte A: O2 sensor voltage (0-1.275V in 0.005V increments)
                // Byte B: Short term fuel trim (same as PID 0x06)
                data[0] = currentState.o2_voltage;  // Already scaled (*200)
                data[1] = ((int16_t)currentState.short_fuel_trim * 128 / 100) + 128;
                dataLen = 2;
                break;

            case 0x1F:  // Run time since engine start
                updateRuntime();
                data[0] = currentState.runtime >> 8;
                data[1] = currentState.runtime & 0xFF;
                dataLen = 2;
                break;

            case 0x20:  // PIDs supported [21-40]
                // We support: 0x21, 0x23, 0x2C, 0x2F, 0x31, 0x33
                data[0] = 0b10100000;  // 21-28: 0x21, 0x23
                data[1] = 0b00001010;  // 29-30: 0x2C, 0x2F
                data[2] = 0b10100000;  // 31-38: 0x31, 0x33
                data[3] = 0b00000001;  // 39-40: 0x40 (supports next range)
                dataLen = 4;
                break;

            case 0x21:  // Distance traveled with MIL on
                data[0] = currentState.mil_distance >> 8;
                data[1] = currentState.mil_distance & 0xFF;
                dataLen = 2;
                break;

            case 0x23: {  // Fuel rail pressure
                // Formula: ((A*256)+B) * 0.079 = kPa
                // We store in kPa, need to encode as value * 0.079 = stored
                // So stored value = kPa / 0.079 ≈ kPa * 12.7
                uint16_t encoded = currentState.fuel_pressure * 10 / 79;  // Approximate conversion
                data[0] = encoded >> 8;
                data[1] = encoded & 0xFF;
                dataLen = 2;
                break;
            }

            case 0x2C:  // Commanded EGR
                // 0-100% encoded as 0-255
                data[0] = (currentState.egr * 255) / 100;
                dataLen = 1;
                break;

            case 0x2F:  // Fuel tank level input
                data[0] = (currentState.fuel_level * 255) / 100;
                dataLen = 1;
                break;

            case 0x31:  // Distance traveled since codes cleared
                data[0] = currentState.distance_mil_clear >> 8;
                data[1] = currentState.distance_mil_clear & 0xFF;
                dataLen = 2;
                break;

            case 0x33:  // Barometric pressure
                data[0] = currentState.barometric;
                dataLen = 1;
                break;

            case 0x40:  // PIDs supported [41-60]
                // We support: 0x42, 0x45, 0x46, 0x51, 0x5C
                data[0] = 0b01011000;  // 41-48: 0x42, 0x45, 0x46
                data[1] = 0b10000000;  // 49-50: 0x51
                data[2] = 0b00010000;  // 51-58: 0x5C
                data[3] = 0b00000000;  // 59-60: none (no further ranges)
                dataLen = 4;
                break;

            case 0x42: {  // Control module voltage
                // Formula: ((A*256)+B)/1000 = Volts
                // We store in millivolts, encode as-is
                data[0] = currentState.battery_voltage >> 8;
                data[1] = currentState.battery_voltage & 0xFF;
                dataLen = 2;
                break;
            }

            case 0x45:  // Relative throttle position
                // Same as absolute throttle for simplicity
                data[0] = (currentState.throttle * 255) / 100;
                dataLen = 1;
                break;

            case 0x46:  // Ambient air temperature
                data[0] = currentState.ambient_temp + 40;  // °C + 40
                dataLen = 1;
                break;

            case 0x51:  // Fuel Type
                // 01 = Gasoline, 02 = Methanol, 03 = Ethanol, 04 = Diesel, etc.
                data[0] = 0x01;  // Gasoline
                dataLen = 1;
                break;

            case 0x5C:  // Engine oil temperature
                data[0] = currentState.oil_temp + 40;  // °C + 40
                dataLen = 1;
                break;

            default:
                // Unsupported PID
                return "NO DATA\r\r>";
        }

        return elm->formatOBDResponse(0x01, pid, data, dataLen);
    }

    // Handle OBD request string (e.g., "01 0C" for RPM)
    // Handle OBD-II mode 03 request (read DTCs)
    String handleMode03() {
        if (currentState.dtc_count == 0) {
            return "NO DATA\r\r>";
        }

        String response = "";
        char buf[8];

        // First byte: number of DTCs
        sprintf(buf, "%02X", currentState.dtc_count);
        response += buf;

        // Return all DTCs (2 bytes each)
        for (uint8_t i = 0; i < currentState.dtc_count; i++) {
            uint16_t dtc = currentState.dtcs[i];
            if (elm->getSpaces()) response += " ";
            sprintf(buf, "%02X", (dtc >> 8) & 0xFF);
            response += buf;
            if (elm->getSpaces()) response += " ";
            sprintf(buf, "%02X", dtc & 0xFF);
            response += buf;
        }

        response += "\r\r>";
        return response;
    }

    // Handle OBD-II mode 04 request (clear DTCs)
    String handleMode04() {
        clearDTCs();
        return "44\r\r>";  // Mode 04 response
    }

    // Handle OBD-II mode 07 request (pending DTCs)
    String handleMode07() {
        // For simplicity, we'll report no pending DTCs
        // (Real systems would track pending vs confirmed DTCs separately)
        return "NO DATA\r\r>";
    }

    // Handle OBD-II mode 09 request (vehicle information)
    String handleMode09(uint8_t pid) {
        char buf[16];  // Increased buffer size to handle spaced output
        String response = "";

        switch (pid) {
            case 0x00: {  // Supported PIDs [01-20]
                // We support: 0x02 (VIN), 0x0A (ECU name)
                if (elm->getHeaders()) {
                    response = "7E8";
                    if (elm->getSpaces()) response += " ";
                    sprintf(buf, "06");
                    response += buf;
                }
                if (elm->getSpaces()) response += " ";
                sprintf(buf, "49");
                response += buf;
                if (elm->getSpaces()) response += " ";
                sprintf(buf, "00");
                response += buf;
                if (elm->getSpaces()) response += " ";
                sprintf(buf, "54");  // 0x02 and 0x0A supported
                response += buf;
                if (elm->getSpaces()) response += " ";
                sprintf(buf, "00");
                response += buf;
                if (elm->getSpaces()) response += " ";
                sprintf(buf, "00");
                response += buf;
                if (elm->getSpaces()) response += " ";
                sprintf(buf, "00");
                response += buf;
                response += "\r\r>";
                return response;
            }

            case 0x02: {  // VIN (17 characters, multi-line response)
                // VIN is sent as multi-line CAN messages
                // Line 1: 49 02 01 XX XX XX XX (message count + first 5 chars)
                // Line 2: 49 02 02 XX XX XX XX (next 7 chars)
                // Line 3: 49 02 03 XX XX XX XX (last 5 chars)

                const char* vin = config->getVIN();
                int vinLen = strlen(vin);
                if (vinLen != 17) {
                    return "NO DATA\r\r>";
                }

                // Message 1: byte count (01) + first 5 VIN chars
                if (elm->getHeaders()) {
                    response = "7E8";
                    if (elm->getSpaces()) response += " ";
                }
                sprintf(buf, "%s49%s02%s01",
                        elm->getSpaces() ? "06 " : "06",
                        elm->getSpaces() ? " " : "",
                        elm->getSpaces() ? " " : "");
                response += buf;
                for (int i = 0; i < 5 && i < vinLen; i++) {
                    if (elm->getSpaces()) response += " ";
                    sprintf(buf, "%02X", (uint8_t)vin[i]);
                    response += buf;
                }
                response += "\r";

                // Message 2: sequence 02 + next 7 VIN chars
                if (elm->getHeaders()) {
                    response += "7E8";
                    if (elm->getSpaces()) response += " ";
                }
                sprintf(buf, "%s49%s02%s02",
                        elm->getSpaces() ? "08 " : "08",
                        elm->getSpaces() ? " " : "",
                        elm->getSpaces() ? " " : "");
                response += buf;
                for (int i = 5; i < 12 && i < vinLen; i++) {
                    if (elm->getSpaces()) response += " ";
                    sprintf(buf, "%02X", (uint8_t)vin[i]);
                    response += buf;
                }
                response += "\r";

                // Message 3: sequence 03 + last 5 VIN chars
                if (elm->getHeaders()) {
                    response += "7E8";
                    if (elm->getSpaces()) response += " ";
                }
                sprintf(buf, "%s49%s02%s03",
                        elm->getSpaces() ? "06 " : "06",
                        elm->getSpaces() ? " " : "",
                        elm->getSpaces() ? " " : "");
                response += buf;
                for (int i = 12; i < 17 && i < vinLen; i++) {
                    if (elm->getSpaces()) response += " ";
                    sprintf(buf, "%02X", (uint8_t)vin[i]);
                    response += buf;
                }
                response += "\r\r>";
                return response;
            }

            case 0x0A: {  // ECU Name
                const char* ecuName = ELM_DEVICE_ID;
                int nameLen = strlen(ecuName);
                if (nameLen > 20) nameLen = 20;  // Limit to 20 chars

                if (elm->getHeaders()) {
                    response = "7E8";
                    if (elm->getSpaces()) response += " ";
                }

                // Length byte
                uint8_t length = 2 + nameLen;  // mode + PID + name bytes
                sprintf(buf, "%02X", length);
                response += buf;
                if (elm->getSpaces()) response += " ";

                // Mode + PID
                sprintf(buf, "49%s0A", elm->getSpaces() ? " " : "");
                response += buf;

                // ECU name as ASCII bytes
                for (int i = 0; i < nameLen; i++) {
                    if (elm->getSpaces()) response += " ";
                    sprintf(buf, "%02X", (uint8_t)ecuName[i]);
                    response += buf;
                }
                response += "\r\r>";
                return response;
            }

            default:
                return "NO DATA\r\r>";
        }
    }

    String handleRequest(String request) {
        request.trim();
        request.toUpperCase();
        request.replace(" ", "");

        // Must be at least 2 characters (mode)
        if (request.length() < 2) {
            return "?\r\r>";
        }

        // Parse mode
        uint8_t mode = strtol(request.substring(0, 2).c_str(), NULL, 16);

        // Handle different modes
        switch (mode) {
            case 0x01:  // Current data
                // Parse PID (if present)
                if (request.length() >= 4) {
                    uint8_t pid = strtol(request.substring(2, 4).c_str(), NULL, 16);
                    return handleMode01(pid);
                }
                return "?\r\r>";

            case 0x03:  // Show stored DTCs
                return handleMode03();

            case 0x04:  // Clear DTCs and MIL
                return handleMode04();

            case 0x07:  // Show pending DTCs
                return handleMode07();

            case 0x09:  // Vehicle information
                // Parse PID (if present)
                if (request.length() >= 4) {
                    uint8_t pid = strtol(request.substring(2, 4).c_str(), NULL, 16);
                    return handleMode09(pid);
                }
                return "?\r\r>";

            default:
                return "NO DATA\r\r>";
        }
    }
};

#endif // PID_HANDLER_H
