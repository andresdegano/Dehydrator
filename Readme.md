# Arduino Dehydrator Project

An Arduino-based environmental monitoring system for a solar-powered dehydrator with electric heating backup. This project reads temperature and humidity from sensors and can be used to control heating elements and monitor drying conditions.

## How It Works

### System Overview
The dehydrator controller monitors ambient temperature and humidity using one of two available sensor types. Based on sensor readings, the system can:
- Monitor drying chamber conditions in real-time
- Automatically control heating/cooling via 3-relay system to maintain optimal temperature
- Log environmental data via serial output and LCD display
- Support both emulated testing (Wokwi) and real hardware deployment

### Sensor Reading Flow
1. **Initialization**: On startup, the Arduino initializes the selected sensor (DHT22 or SHT31)
2. **Reading**: Sensor is queried at regular intervals (typically every few seconds)
3. **Data Processing**: Temperature and humidity values are read and validated
4. **Relay Control**: Based on temperature, the system activates/deactivates heating and cooling
5. **Display Update**: Current readings and relay status shown on LCD
6. **Output**: Data is printed to serial monitor for monitoring/logging
7. **Control Loop**: System repeats indefinitely

### Relay Control System

The system uses **3 relays** to automatically maintain optimal drying conditions:

**Temperature Control Logic:**
- **Target Temperature**: 60°C (default, adjustable via push buttons)
- **Hysteresis**: ±2°C (prevents relay chatter/rapid switching)

**Relay Operations:**

1. **Hot Air Fan (Pin 8)** - Activates when T < 58°C
   - Draws warm ambient air through drying chamber
   - Primary heating method using solar-heated air
   - Stays on until target temperature is reached

2. **Electric Heater (Pin 10)** - Activates when T < 54°C
   - Backup heating for cloudy days or cold nights
   - Only turns on when hot air fan alone can't reach target
   - Automatically turns off once T ≥ 60°C

3. **Cold Air Fan (Pin 9)** - Activates when T > 62°C
   - Draws in cooler ambient air to prevent overheating
   - Protects sensitive foods from temperature damage
   - Ensures even drying without product degradation

**Relay Behavior:**
```
Temp < 54°C   → Hot Air Fan + Electric Heater (full power)
54°C ≤ Temp < 58°C → Hot Air Fan + Electric Heater
58°C ≤ Temp ≤ 62°C → Idle (all relays off)
Temp > 62°C   → Cold Air Fan (cooling)
```

**Safety Features:**
- Hysteresis prevents relay chatter and extends hardware life
- At least one fan always runs to prevent humidity stagnation
- LCD display shows active relay status in real-time
- Serial output logs all relay state changes for debugging
- Push buttons allow safe runtime temperature adjustment (35-75°C)

## Quick Start

### Prerequisites
- PlatformIO installed (`pip install platformio`)
- Arduino IDE or VS Code + PlatformIO extension
- Arduino Uno microcontroller (for hardware deployment)
- Selected sensor (DHT22 or SHT31)
- 2x push buttons (momentary switches, normally open)
- 3x relay module (5V 3-channel)
- 16x2 LCD I2C display

### Test in Emulator (Recommended)
1. **Build for DHT22 emulation:**
   ```bash
   pio run -e uno-wokwi
   ```

2. **Start simulator:**
   - Open VS Code command palette (⌘+Shift+P)
   - Search for "Wokwi: Start Simulator"
   - Watch serial output for temperature/humidity readings

### Deploy to Real Hardware
1. **Wire your sensor** (see wiring diagrams below)
2. **Build and upload:**
   ```bash
   # For DHT22 sensor
   pio run -e uno-dht22 -t upload
   
   # For SHT31 I2C sensor
   pio run -e uno-sht31 -t upload
   ```
3. **Monitor output:**
   ```bash
   pio device monitor -b 9600
   ```

### Available Build Environments

The project supports multiple build environments via `platformio.ini`. Choose based on your sensor type and target platform:

**Wokwi Simulator (Testing):**
```bash
pio run -e uno-wokwi    # DHT22 sensor simulation
pio run -e i2c-wokwi    # SHT31 sensor simulation
```

**Physical Arduino Hardware (Deployment):**
```bash
pio run -e uno-dht22    # Arduino Uno with DHT22 (Pin 7)
pio run -e uno-sht31    # Arduino Uno with SHT31 (I2C: A4/A5)
```

**Environment Details:**

| Environment | Platform | Sensor | Use Case |
|---|---|---|---|
| `uno-wokwi` | Wokwi Emulator | DHT22 | Testing with web simulator |
| `i2c-wokwi` | Wokwi Emulator | SHT31 | Testing with I2C sensor |
| `uno-dht22` | Arduino Uno | DHT22 | Hardware deployment, single-wire sensor |
| `uno-sht31` | Arduino Uno | SHT31 | Hardware deployment, I2C sensor |

**Upload and Monitor:**
```bash
# Compile only
pio run -e uno-dht22

# Compile and upload
pio run -e uno-dht22 -t upload

# Compile, upload, and monitor serial output
pio run -e uno-dht22 -t upload && pio device monitor -b 9600
```

## Upload to Physical Arduino

### 1. **Connect Arduino to Computer**
- Plug the Arduino Uno into your computer via USB cable
- The device will appear as `/dev/ttyUSB0`, `/dev/ttyACM0`, or `/dev/cu.usbserial-*` (depending on OS/driver)

### 2. **Choose Your Sensor Environment**

**For DHT22 sensor:**
```bash
cd /Users/andresdegano/projects/Arduino/Dehydrator
pio run -e uno-dht22 -t upload
```

**For SHT31 sensor (I2C):**
```bash
cd /Users/andresdegano/projects/Arduino/Dehydrator
pio run -e uno-sht31 -t upload
```

### 3. **Verify Successful Upload**
You should see output like:
```
Uploading .pio/build/uno-dht22/firmware.hex
Uploading [████████████████████████████] 100% Done
========================= [SUCCESS] =========================
```

### 4. **View Serial Output**
After successful upload, monitor the serial output:
```bash
# Auto-detect port and monitor
pio device monitor -b 9600

# Or specify port manually
pio device monitor -p /dev/ttyUSB0 -b 9600
```

You should see readings like:
```
[Temp: 24.5°C | Target: 60.0°C | Humidity: 45.3%] [Hot:ON | Heat:OFF | Cold:OFF]
```

### 5. **Troubleshooting**

**If upload fails with "port not found":**
```bash
# List available COM ports
pio device list

# Then specify the port
pio run -e uno-dht22 -t upload --upload-port /dev/ttyUSB0
```

**If you get permission denied errors (Linux/Mac):**
```bash
# Give user permission to access serial port
sudo usermod -a -G dialout $USER
# Then restart your terminal or log out and back in
```

**If IDE/upload is slow:**
- Check USB cable quality
- Try a different USB port
- Verify baud rate matches (9600 in our config)

### 6. **Complete Workflow**

Quick one-liner to build, upload, and monitor:
```bash
# DHT22
pio run -e uno-dht22 -t upload && pio device monitor -b 9600

# SHT31
pio run -e uno-sht31 -t upload && pio device monitor -b 9600
```

### 7. **Hardware Wiring Reminder**

Make sure your Arduino is properly wired before uploading:
- **DHT22**: Data pin → Pin 7, VCC → 5V, GND → GND
- **SHT31**: SDA → A4, SCL → A5, VCC → 5V, GND → GND
- **Relays**: Pins 8, 9, 10 to relay module IN pins
- **Buttons**: Pins 2, 3 to buttons with pull-down resistors
- **LCD I2C**: SDA → A4, SCL → A5

## Sensor Versions

## Sensor Versions

### 1. DHT22 - Single Pin Digital Sensor (Recommended for Start)

**File:** `src/dht_main.cpp`

**Specifications:**
- Protocol: Single-wire digital (SimpleDHT22 library)
- Pin: GPIO 7 (Digital Pin 7)
- Temperature range: -40°C to 125°C
- Humidity range: 0-100% RH
- Accuracy: ±0.5°C, ±2% RH
- Emulator Support: ✅ **Fully supported in Wokwi**

**Wiring (Real Hardware):**
```
DHT22 Pin 1 (VCC)  → Arduino 5V
DHT22 Pin 2 (DATA) → Arduino Pin 7
DHT22 Pin 3 (GND)  → Arduino GND
(Pin 4 is not used)
```

**Usage:**
```bash
# Simulate
pio run -e uno-wokwi

# Deploy to real hardware
pio run -e uno -t upload
```

### 2. SHT31 - I2C Sensor (More Accurate, Hardware Only)

**File:** `src/i2c_main.cpp`

**Specifications:**
- Protocol: Hardware I2C (Adafruit SHT31 library)
- Temperature range: -40°C to 125°C
- Humidity range: 0-100% RH
- Accuracy: ±0.3°C, ±2% RH (better than DHT22)
- Default I2C Address: 0x44
- Emulator Support: ⚠️ **Limited** (I2C not properly simulated in Wokwi)

**Wiring (Real Hardware):**
```
SHT31 VCC   → Arduino 5V
SHT31 GND   → Arduino GND
SHT31 SDA   → Arduino A4 (Pin 18)
SHT31 SCL   → Arduino A5 (Pin 19)
```

**Usage (Real Hardware Only):**
```bash
pio run -e uno -t upload
pio device monitor
```

## Build & Deployment

### Hardware Wiring (All Components)

**Push Button Connections:**
```
Button UP (Increase Temp)   → Arduino Pin 2 (other leg to 5V)
Button DOWN (Decrease Temp) → Arduino Pin 3 (other leg to 5V)
```
*Note: Buttons connected to 5V for low power consumption - current only flows when pressed*

**Relay Module Connections (5V 3-Channel Relay Board):**
```
Relay VCC  → Arduino 5V
Relay GND  → Arduino GND

Relay IN1 (Hot Air Fan)   → Arduino Pin 8
Relay IN2 (Cold Air Fan)  → Arduino Pin 9
Relay IN3 (Electric Heater) → Arduino Pin 10
```

**AC Appliance Connections (for actual devices):**
```
Relay 1 Common → Hot Air Fan Motor (AC 110/220V)
Relay 2 Common → Cold Air Fan Motor (AC 110/220V)
Relay 3 Common → Electric Heater Element (AC 110/220V)
```

**LCD Display (I2C 1602 Display):**
```
LCD VCC → Arduino 5V
LCD GND → Arduino GND
LCD SDA → Arduino A4 (Pin 18)
LCD SCL → Arduino A5 (Pin 19)
```

**Complete Wiring Diagram (with DHT22):**
```
Arduino Uno
├── Pin 2  → Button UP (Increase Temperature)
├── Pin 3  → Button DOWN (Decrease Temperature)
├── Pin 7  → DHT22 Data Pin
├── Pin 8  → Relay 1 (Hot Air Fan)
├── Pin 9  → Relay 2 (Cold Air Fan)
├── Pin 10 → Relay 3 (Electric Heater)
├── A4 (SDA) → LCD & SHT31 (if used)
└── A5 (SCL) → LCD & SHT31 (if used)

5V Rail
├── Relay Module VCC
├── DHT22 VCC
├── LCD VCC
└── Button leads (other side to GND)

GND Rail
├── Relay Module GND
├── DHT22 GND
├── LCD GND
└── Button leads (from both buttons)
```

## Project Structure

```
src/
  dht_main.cpp      - DHT22 sensor implementation (recommended start)
  i2c_main.cpp      - SHT31 I2C sensor implementation

diagram.json        - Wokwi circuit diagram (visual representation)
wokwi.toml          - Wokwi emulator configuration
platformio.ini      - PlatformIO build & environment configuration
.gitignore          - Git ignore rules (build artifacts, sensitive data)
Readme.md           - This file
```

## Understanding the Code

### DHT22 Implementation (`src/dht_main.cpp`)
- Initializes SimpleDHT22 library on pin 7
- Reads temperature and humidity every 2 seconds
- **Button Handling**: Pins 2 (UP) and 3 (DOWN) adjust `targetTemp` variable
- Controls 3 relays based on dynamic temperature logic
- Updates 16x2 LCD I2C display with real-time status
- Outputs formatted data to serial (9600 baud)
- Handles sensor errors gracefully
- Example output: 
  ```
  [Temp: 60.5°C | Target: 60.0°C | Humidity: 65%] [Hot:ON | Heat:OFF | Cold:OFF]
  ```

### SHT31 Implementation (`src/i2c_main.cpp`)
- Initializes I2C communication on Arduino pins A4/A5
- Detects SHT31 sensor on I2C bus (address 0x44 or 0x45)
- **Button Handling**: Pins 2 (UP) and 3 (DOWN) adjust `targetTemp` variable
- Reads temperature and humidity with high accuracy
- Controls 3 relays with same dynamic logic
- Updates LCD display with enhanced heater control info
- Provides heater element cycle management
- Example output: 
  ```
  [Temp: 60.42°C | Target: 60.0°C | Humidity: 64.53%] [Hot:ON | Heat:OFF | Cold:OFF]
  ```

### Key Functions

**`handleButtonPresses()`** - New function for temperature adjustment
- Reads pins 2 and 3 for button presses
- Implements debounce logic (300ms default) to prevent accidental changes
- Validates temperature is within TEMP_MIN (35°C) and TEMP_MAX (75°C)
- Updates `targetTemp` variable and logs changes to serial

**`controlRelays(float temperature)`** - Relay logic
- Uses dynamic `targetTemp` instead of fixed #define
- Implements ±2°C hysteresis for stable control
- Manages 3 relay states based on temperature thresholds
- Logs all state changes to serial monitor

## Configuration

### Adjust Temperature at Runtime (Easiest)
Use the push buttons on pins 2 and 3:
- **Button UP (Pin 2)**: Increases target temperature by 1°C
- **Button DOWN (Pin 3)**: Decreases target temperature by 1°C
- Range: 35°C to 75°C
- Changes are **immediate** and reflected on LCD display

### Change Default Starting Temperature (in Code)
Edit the relevant source file (`dht_main.cpp` or `i2c_main.cpp`):

```cpp
float targetTemp = 60.0;  // Change this value (35-75°C range)
```

Then rebuild and redeploy:
```bash
pio run --target clean
pio run -e uno -t upload
```

### Fine-Tune Control Parameters (Advanced)
Edit the same file to adjust these parameters:

```cpp
#define HYSTERESIS 2.0          // ±2°C - reduce for tighter control, increase for stability
#define HEATER_THRESHOLD 54.0   // Temperature at which backup heater activates
#define TEMP_MIN 35.0           // Lowest allowed target temperature via buttons
#define TEMP_MAX 75.0           // Highest allowed target temperature via buttons
#define DEBOUNCE_DELAY 300      // milliseconds - increase if buttons are too sensitive
```

**Common Configurations:**

For **herbs and leafy greens** (delicate):
```cpp
float targetTemp = 40.0;
#define HEATER_THRESHOLD 35.0
```

For **fruits and vegetables** (standard):
```cpp
float targetTemp = 60.0;       // Current default
#define HEATER_THRESHOLD 54.0
```

For **meats and jerky** (aggressive):
```cpp
float targetTemp = 70.0;
#define HEATER_THRESHOLD 65.0
```

For **tight temperature control** (±0.5°C):
```cpp
#define HYSTERESIS 0.5
#define DEBOUNCE_DELAY 500      // Prevent accidental changes
```

For **loose temperature control** (±3°C, energy efficient):
```cpp
#define HYSTERESIS 3.0
#define DEBOUNCE_DELAY 200      // Responsive buttons
```

After modifying, rebuild and redeploy:
```bash
pio run --target clean
pio run -e uno -t upload
```

### Change Active Sensor
Edit `platformio.ini` and set the build environment:

```ini
[env:uno]
build_flags = -DUSE_DHT22  # or -DUSE_SHT31
```

Then rebuild:
```bash
pio run --target clean
pio run
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Sensor not detected | Check wiring, verify I2C address (SHT31 default: 0x44) |
| Serial output not showing | Verify baud rate (9600 for DHT22, adjust in code if needed), try device monitor |
| Wokwi simulator not responding | Rebuild with `pio run --target clean` first |
| DHT22 reading errors | Ensure 10kΩ pull-up resistor on data line |
| Relays not activating | Check relay module power (5V), verify pin connections (8, 9, 10) |
| Relays switching erratically | Reduce HYSTERESIS value in code, check sensor accuracy |
| LCD display blank | Check I2C address (0x3F for Wokwi, 0x27 for typical modules) |
| Heater not turning on | Verify HEATER_THRESHOLD value (54°C by default), check wiring |
| Temperature not reaching target | Check if all fans/heater are wired correctly, verify relay switching |
| Buttons not working | Verify pins 2 and 3 wiring to GND, check for 300ms debounce delay |
| Temperature jumps randomly | Reduce DEBOUNCE_DELAY to 200ms, check button contacts for noise |
| Button changes are too slow | Increase DEBOUNCE_DELAY if accidental changes occur, reduce if needed |