# Arduino Dehydrator Project

An Arduino-based environmental monitoring system for a solar-powered dehydrator with electric heating backup. This project reads temperature and humidity from sensors and can be used to control heating elements and monitor drying conditions.

## How It Works

### System Overview
The dehydrator controller maintains optimal drying conditions by monitoring temperature and humidity using one of two available sensor types (DHT22 or SHT31). The system automatically controls a 3-relay heating/cooling system based on sensor readings, displays real-time data on an LCD, and allows runtime temperature adjustment via push buttons.

**Key Features:**
- Real-time temperature and humidity monitoring
- Automatic heating/cooling control to maintain target temperature
- LCD display showing current readings and active relay status
- Push button interface for runtime temperature adjustment (35-75°C)
- Serial logging of all readings and relay state changes
- Non-blocking control loop (sensor reads every 2 seconds, buttons checked every loop)

### Main Control Loop
The `loop()` function operates on two timescales:

**Fast Loop (Every iteration, < 1ms):**
1. Check for button presses (temperature adjustment)
2. Update button state tracking for debouncing

**Slow Loop (Every 2 seconds, non-blocking):**
1. Read temperature and humidity from sensor
2. Validate sensor readings
3. Update LCD display with current values and relay status
4. Execute relay control logic based on temperature
5. Log readings to serial monitor

This hybrid approach ensures responsive button input while avoiding sensor read delays.

### Relay Control System

The system uses **3 relays** to automatically maintain optimal drying conditions through temperature feedback:

**Configuration:**
- **Target Temperature**: 60°C (default, adjustable 35-75°C via buttons)
- **Hysteresis**: ±2°C (prevents relay chatter and rapid switching)
- **Sensor Read Interval**: 2 seconds

**Temperature Control Logic:**

The `controlRelays()` function implements a three-zone control strategy:

| Temperature Range | Action | Rationale |
|---|---|---|
| **T < 58°C** (below target - HYSTERESIS) | Activate HOT AIR FAN | Primary heating using solar-preheated air |
| **T < 54°C** (far below target) | ALSO activate ELECTRIC HEATER | Backup heating for cloudy days/cold nights |
| **58°C ≤ T ≤ 62°C** (target zone) | Idle (keep fan running if on) | Prevent frequent switching; maintain circulation |
| **T > 62°C** (above target + HYSTERESIS) | Activate COLD AIR FAN, turn off heater | Prevent overheating; protect food quality |

**Relay Pin Assignment:**
1. **HOT AIR FAN (Pin 8)** - Primary heating
   - Draws warm ambient or solar-heated air through chamber
   - Active when temperature is below target - HYSTERESIS
   - Automatically turns off when temperature reaches stable zone

2. **ELECTRIC HEATER (Pin 10)** - Backup heating
   - Only activates when hot air fan alone insufficient (T < 54°C)
   - Turns off once target temperature reached
   - Ensures system reaches desired temp even on cold/cloudy days

3. **COLD AIR FAN (Pin 9)** - Cooling
   - Activates when temperature exceeds target + HYSTERESIS
   - Draws cooler ambient air to prevent overheating
   - Protects temperature-sensitive foods

**Safety & Stability Features:**
- Hysteresis (±2°C) prevents relay chatter and extends hardware lifespan
- At least one fan always runs in stable zone to maintain air circulation and prevent humidity stagnation
- Heater is always OFF when cooling (mutually exclusive operation)
- LCD display shows active relay status in real-time for monitoring
- Serial output logs all relay state changes with timestamps for debugging
- Button debounce delay (300ms) prevents accidental multiple presses

### Temperature Control Interface

**Push Button Control:**
- **Button UP (Pin 2)**: Increase target temperature by 1°C
- **Button DOWN (Pin 3)**: Decrease target temperature by 1°C
- **Valid Range**: 35°C to 75°C (enforced in code)
- **Debounce Delay**: 300ms after each button press
- **Detection Method**: Edge-triggered on button press (LOW→HIGH transition)

**LCD Display (I2C address 0x3F):**

Line 1: Temperature reading, target, and humidity
```
T:24.5→60C H:45%
```
- T: Current temperature from sensor
- →: Arrow pointing to target temperature
- Target: User-set target temperature (integer)
- H: Current humidity percentage

Line 2: Relay status and button controls
```
HOT FAN   +/-
HEATER ON +/-
COLD FAN  +/-
IDLE      +/-
```
- Shows which relay is currently active
- "+/-" prompt reminds user buttons are available for adjustment

**Serial Monitor Output:**

On startup:
```
Dehydrator Control System (DHT22)
=========================================
Temperature Control System
Target Temperature: 60°C (adjustable via buttons)
Button UP (Pin 2): Increase target temp
Button DOWN (Pin 3): Decrease target temp
Hysteresis: ±2°C
=========================================
```

During operation:
```
[Temp: 24.5°C | Target: 60.0°C | Humidity: 45.3%] [Hot:ON | Heat:OFF | Cold:OFF]
  > HOT AIR FAN ON (heating)
```

Button press:
```
Target Temperature: 61.0°C (increased via button)
```

Relay state changes:
```
  > HOT AIR FAN ON (heating)
  > ELECTRIC HEATER ON (backup)
  > ELECTRIC HEATER OFF
  > COLD AIR FAN ON (cooling)
```

### Code Architecture

**File Structure:**
- `src/dht_main.cpp`: Main program using DHT22 sensor (single-wire digital)
- `src/i2c_main.cpp`: Alternate program using SHT31 sensor (I2C protocol)

**Key Functions:**

| Function | File(s) | Purpose |
|---|---|---|
| `setup()` | Both | Initialize pins, sensors, LCD, serial communication |
| `loop()` | Both | Main control loop (buttons checked every iteration, sensors every 2s) |
| `handleButtonPresses()` | Both | Detect button presses and update target temperature |
| `controlRelays()` | Both | Implement temperature control logic and activate/deactivate relays |
| `updateLCDDisplay()` | Both | Format and display readings and relay status on LCD |
| `scanI2CBus()` | i2c_main.cpp only | Detect SHT31 sensor on I2C bus at startup |

**Global Variables:**
```cpp
float targetTemp = 60.0;        // User-adjustable target temperature
bool hotAirFanActive;           // Tracks HOT AIR FAN relay state
bool coldAirFanActive;          // Tracks COLD AIR FAN relay state
bool heaterActive;              // Tracks ELECTRIC HEATER relay state
```

**Pin Configuration:**
```cpp
#define DHT_PIN 7               // DHT22 data pin (dht_main.cpp only)
#define HOT_AIR_FAN_PIN 8       // Hot air fan relay
#define COLD_AIR_FAN_PIN 9      // Cold air fan relay
#define HEATER_PIN 10           // Electric heater relay
#define BUTTON_UP_PIN 2         // Temperature up button
#define BUTTON_DOWN_PIN 3       // Temperature down button
// LCD address: 0x3F (I2C)
// SHT31 address: 0x44 or 0x45 (I2C, auto-detected)
```

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