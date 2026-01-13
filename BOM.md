# MockStang Bill of Materials (BOM)

This document lists all hardware components needed to build MockStang. There are two build options: ESP-01S (WiFi only) or ESP32-S3 Feather (WiFi + BLE + Display).

---

## ESP-01S Build (WiFi Only)

### Required Components

| Item | Description | Quantity | Approximate Cost | Link |
|------|-------------|----------|------------------|------|
| **ESP-01S Module** | ESP8266-based WiFi module, 1MB flash | 1 | $3-5 | [Amazon Link] |
| **USB to TTL Serial Adapter** | For programming ESP-01S (FTDI or CH340G) | 1 | $8-12 | [Amazon Link] |
| **3.3V Power Supply** | Regulated 3.3V, minimum 500mA | 1 | $6-10 | [Amazon Link] |
| **Jumper Wires** | Female-to-female for programming connections | 6-8 | $5 | [Amazon Link] |

### Optional Components

| Item | Description | Quantity | Approximate Cost | Link |
|------|-------------|----------|------------------|------|
| **ESP-01S Adapter Board** | Breadboard-friendly adapter with voltage regulator | 1 | $3-5 | [Amazon Link] |
| **Push Button Switch** | For GPIO0 programming mode | 1 | $2 | [Amazon Link] |
| **Enclosure** | Small project box for ESP-01S | 1 | $5-10 | [Amazon Link] |
| **2-Pin JST Connector** | For easy power connection | 1 | $3 | [Amazon Link] |

**Estimated Total (ESP-01S Build):** $25-35 (required only)

---

## ESP32-S3 Feather Build (WiFi + BLE + Display)

### Required Components

| Item | Description | Quantity | Approximate Cost | Link |
|------|-------------|----------|------------------|------|
| **Adafruit ESP32-S3 Reverse TFT Feather** | ESP32-S3 with 240x135 color TFT display, 8MB flash, 2MB PSRAM | 1 | $25-30 | [Adafruit Link] |
| **USB-C Cable** | For programming and power | 1 | $5-8 | [Amazon Link] |

### Optional Components

| Item | Description | Quantity | Approximate Cost | Link |
|------|-------------|----------|------------------|------|
| **LiPo Battery** | 3.7V 500-2000mAh for portable operation | 1 | $10-15 | [Adafruit Link] |
| **FeatherWing Enclosure** | Protective case for Feather boards | 1 | $8-12 | [Adafruit Link] |
| **Header Pins** | For prototyping or adding peripherals | 1 set | $3-5 | [Amazon Link] |
| **USB-C Power Adapter** | 5V 2A wall adapter | 1 | $8-10 | [Amazon Link] |

**Estimated Total (ESP32-S3 Build):** $30-40 (required only)

---

## Power Supply Options

### ESP-01S Power Requirements
- **Voltage:** 3.3V (regulated)
- **Current:** 250mA typical, 500mA peak during WiFi transmission
- **Important:** DO NOT use 5V directly - ESP-01S is NOT 5V tolerant!

### Recommended Power Supplies for ESP-01S

| Option | Description | Pros | Cons | Link |
|--------|-------------|------|------|------|
| **AMS1117-3.3 Regulator** | Linear regulator module | Cheap, simple | Requires 5V input, generates heat | [Amazon Link] |
| **USB to ESP-01S Adapter** | Integrated programmer + power | All-in-one solution | More expensive | [Amazon Link] |
| **Buck Converter (adjustable)** | Switching regulator module | Efficient, works with 5-24V input | Requires adjustment | [Amazon Link] |
| **Dedicated 3.3V PSU** | Wall adapter with 3.3V output | Clean power, no regulation needed | Less common, harder to find | [Amazon Link] |

### ESP32-S3 Power Requirements
- **Voltage:** 5V via USB-C
- **Current:** 500mA typical, 1A peak
- **Power:** Built-in USB-C port handles all power management
- **Battery:** Optional LiPo battery (3.7V) with built-in charging

---

## Programming Setup

### ESP-01S Programming

**Connections Required:**

| ESP-01S Pin | USB-TTL Adapter | Notes |
|-------------|-----------------|-------|
| VCC | 3.3V | **NOT 5V!** |
| GND | GND | Common ground |
| TX | RX | Cross TX/RX |
| RX | TX | Cross TX/RX |
| CH_PD (EN) | 3.3V | Chip enable (always high) |
| GPIO0 | GND (during upload) | Pull low to enter programming mode |
| GPIO2 | (floating) | Leave disconnected |
| RST | (optional button to GND) | For manual reset |

**Programming Procedure:**
1. Connect GPIO0 to GND
2. Power on ESP-01S (or press reset if already powered)
3. Upload firmware via PlatformIO or Arduino IDE
4. Disconnect GPIO0 from GND
5. Reset ESP-01S to run uploaded firmware

### ESP32-S3 Programming

**Connections Required:**
- USB-C cable only (no external programmer needed)

**Programming Procedure:**
1. Connect USB-C cable to computer
2. Upload firmware via PlatformIO or Arduino IDE
3. Board automatically enters programming mode via USB

---

## Tools & Accessories

### Required Tools
- Computer with USB port (Windows, Mac, or Linux)
- PlatformIO IDE or Arduino IDE installed
- Serial monitor software (built into IDEs)

### Recommended Tools

| Tool | Purpose | Link |
|------|---------|------|
| **Multimeter** | Verify power supply voltage, check connections | [Amazon Link] |
| **Helping Hands** | Hold wires during soldering/assembly | [Amazon Link] |
| **Soldering Iron Kit** | If soldering headers or connections | [Amazon Link] |
| **Heat Shrink Tubing** | Protect exposed connections | [Amazon Link] |
| **Cable Ties** | Organize wiring | [Amazon Link] |

---

## Assembly Notes

### ESP-01S Assembly Tips
1. **Verify 3.3V:** Always check voltage with multimeter before connecting ESP-01S
2. **Stable Power:** ESP-01S draws current spikes during WiFi - use quality power supply
3. **Programming Adapter:** Consider buying an ESP-01S programmer board for easier uploads
4. **GPIO0 Button:** Add a push button between GPIO0 and GND for easy programming mode
5. **Breadboard Adapter:** ESP-01S has 2x4 pin header - adapter makes breadboard use easier

### ESP32-S3 Assembly Tips
1. **Plug and Play:** ESP32-S3 Feather is ready to use out of the box
2. **Display Protection:** Consider a case to protect the TFT screen
3. **Battery Usage:** For portable operation, use genuine LiPo batteries only
4. **USB-C Quality:** Use a good quality USB-C cable for reliable programming
5. **Expansion:** Header pins allow adding sensors or other FeatherWings

---

## Where to Buy

### ESP-01S Components
- **Amazon:** ESP-01S modules, USB-TTL adapters, power supplies, cables
- **AliExpress:** Lower cost but longer shipping times
- **eBay:** Wide selection of ESP-01S adapters and accessories
- **Adafruit:** Quality USB-TTL adapters and tools
- **SparkFun:** Educational resources and reliable components

### ESP32-S3 Components
- **Adafruit:** Official ESP32-S3 Feather boards (recommended)
- **Amazon:** USB-C cables, cases, accessories
- **Adafruit:** LiPo batteries, FeatherWing accessories

---

## Cost Comparison

| Build Option | Required Components | With Optional Items | Notes |
|--------------|---------------------|---------------------|-------|
| **ESP-01S** | $25-35 | $40-60 | Budget-friendly, WiFi only |
| **ESP32-S3** | $30-40 | $50-80 | Premium features, includes display |

---

## Safety Notes

⚠️ **Important Safety Information:**

1. **ESP-01S Voltage:** NEVER apply 5V directly to ESP-01S - it operates at 3.3V only
2. **Polarity:** Double-check power supply polarity before connecting
3. **Current Rating:** Ensure power supply can deliver at least 500mA for stable operation
4. **LiPo Batteries:** Use proper LiPo battery handling procedures (ESP32-S3 only)
5. **Heat:** Power regulators can get hot - ensure adequate ventilation
6. **ESD Protection:** Handle boards with anti-static precautions

---

## Vendor Links (To Be Filled In)

### ESP-01S
- [ ] ESP-01S Module: [Your Amazon Link Here]
- [ ] USB to TTL Serial Adapter: [Your Amazon Link Here]
- [ ] 3.3V Power Supply: [Your Amazon Link Here]
- [ ] ESP-01S Programmer Board: [Your Amazon Link Here]

### ESP32-S3
- [ ] Adafruit ESP32-S3 Reverse TFT Feather: [Adafruit Product Link]
- [ ] USB-C Cable: [Your Amazon Link Here]
- [ ] LiPo Battery (optional): [Adafruit Product Link]

### Common Accessories
- [ ] Jumper Wires: [Your Amazon Link Here]
- [ ] Multimeter: [Your Amazon Link Here]
- [ ] Breadboard (optional): [Your Amazon Link Here]

---

**Last Updated:** 2026-01-13
**Project:** MockStang WiFi/BLE OBD-II Emulator

**Note:** Prices are approximate and may vary by region and vendor. Always verify component specifications before purchasing.
