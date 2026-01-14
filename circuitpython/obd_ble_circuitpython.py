"""
OBD-II BLE Connection for CircuitPython

Simplified BLE client for Vgate iCar2 adapter using CircuitPython's _bleio.
Designed for ESP32-S3 and other CircuitPython boards with BLE support.

Based on protocol analysis from MockStang project.
"""

import time
import _bleio

# BLE UUIDs for Vgate iCar2 adapter
VGATE_SERVICE_UUID = 0x18F0
VGATE_WRITE_UUID = 0x2AF1      # Send OBD commands here
VGATE_NOTIFY_UUID = 0x2AF0     # Receive responses here

# Device names to scan for
VGATE_DEVICE_NAMES = ["IOS-Vlink", "IOS-Vgate", "Vlinker", "Vgate"]

# Timeouts
SCAN_TIMEOUT = 10.0
CONNECT_TIMEOUT = 5.0
RESPONSE_TIMEOUT = 3.0

# Common OBD PIDs
PID_ENGINE_RPM = "010C"
PID_VEHICLE_SPEED = "010D"
PID_THROTTLE_POS = "0111"
PID_ENGINE_LOAD = "0104"
PID_COOLANT_TEMP = "0105"
PID_INTAKE_TEMP = "010F"
PID_MAF = "0110"


class OBDBLEConnection:
    """BLE connection to Vgate OBD-II adapter"""

    def __init__(self):
        self._adapter = _bleio.adapter
        self._connection = None
        self._write_char = None
        self._notify_char = None
        self._response_buffer = ""
        self._echo_enabled = True
        self._connected = False

    def scan(self, timeout=SCAN_TIMEOUT):
        """Scan for Vgate OBD-II adapter"""
        print(f"[OBD BLE] Scanning for Vgate adapter ({timeout}s)...")

        scan_start = time.monotonic()

        for entry in self._adapter.start_scan(timeout=timeout):
            if time.monotonic() - scan_start > timeout:
                break

            # Check device name
            if entry.complete_name:
                for device_name in VGATE_DEVICE_NAMES:
                    if device_name in entry.complete_name:
                        print(f"[OBD BLE] Found: {entry.complete_name}")
                        self._adapter.stop_scan()
                        return entry

            # Check advertised service UUID (fallback)
            if entry.advertisement_bytes:
                adv_data = entry.advertisement_bytes
                if b'\xF0\x18' in adv_data or b'\x18\xF0' in adv_data:
                    print(f"[OBD BLE] Found via service UUID")
                    self._adapter.stop_scan()
                    return entry

        self._adapter.stop_scan()
        print("[OBD BLE] No adapter found")
        return None

    def connect(self, scan_timeout=SCAN_TIMEOUT):
        """Connect to Vgate OBD-II adapter"""
        try:
            # Scan for device
            device = self.scan(timeout=scan_timeout)
            if not device:
                return False

            # Connect to device
            print(f"[OBD BLE] Connecting...")
            self._connection = self._adapter.connect(device.address, timeout=CONNECT_TIMEOUT)

            # Discover services
            print("[OBD BLE] Discovering services...")
            for service in self._connection:
                # Look for custom service 0x18F0
                if service.uuid.uuid16 == VGATE_SERVICE_UUID:
                    print(f"[OBD BLE] Found service: {hex(service.uuid.uuid16)}")

                    # Find characteristics
                    for char in service.characteristics:
                        if char.uuid.uuid16 == VGATE_WRITE_UUID:
                            self._write_char = char
                            print(f"[OBD BLE] Found write char: {hex(char.uuid.uuid16)}")

                        elif char.uuid.uuid16 == VGATE_NOTIFY_UUID:
                            self._notify_char = char
                            print(f"[OBD BLE] Found notify char: {hex(char.uuid.uuid16)}")

            if not self._write_char or not self._notify_char:
                print("[OBD BLE] ERROR: Required characteristics not found")
                self.disconnect()
                return False

            # Subscribe to notifications
            self._notify_char.set_cccd(notify=True)
            print("[OBD BLE] Subscribed to notifications")

            # Initialize adapter
            time.sleep(0.5)
            self._send_command("ATZ")  # Reset
            time.sleep(1.0)

            # Disable echo for cleaner parsing
            response = self._send_command("ATE0")
            if "OK" in response:
                self._echo_enabled = False
                print("[OBD BLE] Echo disabled")

            self._connected = True
            print("[OBD BLE] ✓ Connected and initialized")
            return True

        except Exception as e:
            print(f"[OBD BLE] Connection error: {e}")
            self.disconnect()
            return False

    def disconnect(self):
        """Disconnect from adapter"""
        if self._connection:
            try:
                self._connection.disconnect()
            except:
                pass
            self._connection = None

        self._write_char = None
        self._notify_char = None
        self._connected = False
        print("[OBD BLE] Disconnected")

    def is_connected(self):
        """Check if connected to adapter"""
        return self._connected and self._connection is not None

    def _send_command(self, command):
        """Send OBD command and wait for response"""
        if not self._write_char or not self._notify_char:
            raise Exception("Not connected")

        # Clear buffer
        self._response_buffer = ""

        # Send command with carriage return
        cmd_bytes = (command + "\r").encode('ascii')
        self._write_char.value = cmd_bytes

        # Wait for response with timeout
        start_time = time.monotonic()
        while time.monotonic() - start_time < RESPONSE_TIMEOUT:
            # Read notification data
            if self._notify_char.value:
                data = self._notify_char.value
                if data:
                    self._response_buffer += data.decode('ascii', errors='ignore')

                    # Check for prompt (end of response)
                    if '>' in self._response_buffer:
                        return self._response_buffer

            time.sleep(0.05)

        # Timeout
        return self._response_buffer

    def _parse_response(self, response, expected_pid=None):
        """Parse OBD response and extract data"""
        # Remove echo if present
        lines = response.split('\r')

        # Find the data line
        for line in lines:
            line = line.strip()

            # Skip empty lines and prompt
            if not line or line == '>':
                continue

            # Check for errors
            if 'NO DATA' in line or 'ERROR' in line or '?' in line:
                return None

            # Check if this is a response line
            if expected_pid and expected_pid in line:
                return line
            elif line.startswith('4'):  # Response mode
                return line

        return None

    def _decode_pid(self, response, pid):
        """Decode PID response into numeric value"""
        if not response:
            return None

        # Extract hex bytes from response
        parts = response.split()
        if len(parts) < 3:
            return None

        try:
            if pid == "010C":  # RPM
                if len(parts) >= 4:
                    a = int(parts[2], 16)
                    b = int(parts[3], 16)
                    return ((a * 256) + b) / 4.0

            elif pid == "010D":  # Speed (km/h)
                if len(parts) >= 3:
                    return float(int(parts[2], 16))

            elif pid == "0111":  # Throttle position (%)
                if len(parts) >= 3:
                    a = int(parts[2], 16)
                    return (a * 100.0) / 255.0

            elif pid == "0104":  # Engine load (%)
                if len(parts) >= 3:
                    a = int(parts[2], 16)
                    return (a * 100.0) / 255.0

            elif pid == "0105":  # Coolant temp (°C)
                if len(parts) >= 3:
                    return float(int(parts[2], 16) - 40)

            elif pid == "010F":  # Intake air temp (°C)
                if len(parts) >= 3:
                    return float(int(parts[2], 16) - 40)

            elif pid == "0110":  # MAF (g/s)
                if len(parts) >= 4:
                    a = int(parts[2], 16)
                    b = int(parts[3], 16)
                    return ((a * 256) + b) / 100.0

        except (ValueError, IndexError):
            pass

        return None

    def query_pid(self, pid):
        """Query a PID and return decoded value"""
        if not self.is_connected():
            return None

        try:
            # Send command
            response = self._send_command(pid)

            # Parse response
            expected = "41 " + pid[2:4]
            parsed = self._parse_response(response, expected)

            # Decode value
            return self._decode_pid(parsed, pid)

        except Exception as e:
            print(f"[OBD BLE] Query error: {e}")
            return None

    # Convenience methods for common PIDs

    def get_rpm(self):
        """Get engine RPM"""
        return self.query_pid(PID_ENGINE_RPM)

    def get_speed(self):
        """Get vehicle speed (km/h)"""
        return self.query_pid(PID_VEHICLE_SPEED)

    def get_throttle(self):
        """Get throttle position (%)"""
        return self.query_pid(PID_THROTTLE_POS)

    def get_load(self):
        """Get engine load (%)"""
        return self.query_pid(PID_ENGINE_LOAD)

    def get_coolant_temp(self):
        """Get coolant temperature (°C)"""
        return self.query_pid(PID_COOLANT_TEMP)

    def get_intake_temp(self):
        """Get intake air temperature (°C)"""
        return self.query_pid(PID_INTAKE_TEMP)

    def get_maf(self):
        """Get MAF air flow rate (g/s)"""
        return self.query_pid(PID_MAF)

    def read_all(self):
        """Read all common PIDs"""
        data = {}
        data['rpm'] = self.get_rpm()
        data['speed'] = self.get_speed()
        data['throttle'] = self.get_throttle()
        data['load'] = self.get_load()
        data['coolant_temp'] = self.get_coolant_temp()
        data['intake_temp'] = self.get_intake_temp()
        data['maf'] = self.get_maf()
        return data
