# Arduino Dehydrator Project

An Arduino-based environmental monitoring system for a solar-powered dehydrator with electric heating backup. This project reads temperature and humidity from sensors and can be used to control heating elements and monitor drying conditions.

## How It Works

### System Overview
The dehydrator controller maintains optimal drying conditions by monitoring temperature and humidity using the SHT31 I2C sensor. The system automatically controls a 3-relay heating/cooling system based on sensor readings, displays real-time data on an LCD, and allows runtime temperature adjustment via push buttons.

**Key Features:**
- Real-time temperature and humidity monitoring via SHT31 I2C sensor
- Automatic heating/cooling control to maintain target temperature
- LCD 2004 display (20x4) over I2C showing current readings and active relay status
- Push button interface for runtime temperature adjustment (20-75°C)
- Non-blocking control loop (sensor reads every 2 seconds, buttons checked every loop)
- Active-LOW button debouncing with 10-read confirmation and 300ms minimum interval

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
- **Valid Range**: 20°C to 75°C (enforced in code)
- **Button Wiring**: Connected to GND (active-LOW) with internal pull-ups
- **Debounce Method**: Counter-based (requires 10 consecutive stable reads)
- **Debounce Delay**: 300ms minimum between button actions

**LCD Display (20x4 Character, I2C Mode):**

The LCD 2004 display connects via I2C (only 2 wires: SDA and SCL) for a cleaner wiring setup.

- **I2C Address**: 0x27 (default, try 0x3F if it doesn't initialize)
- **Connection**: Uses Arduino I2C bus (A4 = SDA, A5 = SCL) shared with SHT31 sensor

Display Layout:
```
Row 0: T:25.1C  H:45%
Row 1: Target: 60C
Row 2: Status: HOT FAN
Row 3: Adjust: (+) UP (-)
```
- Row 0: Current temperature and humidity
- Row 1: Target temperature setpoint
- Row 2: Active relay status (HOT FAN / COLD FAN / HEATER ON / IDLE)
- Row 3: Button instructions for temperature adjustment

**Serial Monitor Output:**

On startup:
```
Dehydrator Control System (SHT31)
=========================================
Temperature Control System
Target Temperature: 60°C (adjustable via buttons)
Button UP (Pin 2): Increase target temp (active-LOW, connected to GND)
Button DOWN (Pin 3): Decrease target temp (active-LOW, connected to GND)
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
- `src/main.cpp`: Main program using SHT31 I2C sensor

**Key Functions:**

| Function | Purpose |
|---|---|
| `setup()` | Initialize pins, sensors, LCD, serial communication |
| `loop()` | Main control loop (buttons checked every iteration, sensors every 2s) |
| `handleButtonPresses()` | Detect button presses and update target temperature |
| `controlRelays()` | Implement temperature control logic and activate/deactivate relays |
| `updateLCDDisplay()` | Format and display readings and relay status on LCD |
| `scanI2CBus()` | Detect SHT31 sensor on I2C bus at startup |

**Global Variables:**
```cpp
float targetTemp = 60.0;        // User-adjustable target temperature
bool hotAirFanActive;           // Tracks HOT AIR FAN relay state
bool coldAirFanActive;          // Tracks COLD AIR FAN relay state
bool heaterActive;              // Tracks ELECTRIC HEATER relay state
```

**Pin Configuration:**
```cpp
#define HOT_AIR_FAN_PIN 8       // Hot air fan relay
#define COLD_AIR_FAN_PIN 9      // Cold air fan relay
#define HEATER_PIN 10           // Electric heater relay
#define BUTTON_UP_PIN 2         // Temperature up button (GND-connected, active-LOW)
#define BUTTON_DOWN_PIN 3       // Temperature down button (GND-connected, active-LOW)
#define LCD_I2C_ADDRESS 0x27    // LCD 2004 I2C address (try 0x3F if not detected)
// I2C Devices (shared bus on A4/A5):
// - SHT31 address: 0x44 or 0x45 (auto-detected)
// - LCD 2004 address: 0x27 (configurable)
// - I2C pins: A4 (SDA), A5 (SCL)
```

## Quick Start

### Prerequisites
- PlatformIO installed (`pip install platformio`)
- Arduino IDE or VS Code + PlatformIO extension
- Arduino Uno microcontroller (for hardware deployment)
- SHT31 I2C temperature/humidity sensor
- 2x push buttons (momentary switches, normally open)
- 3x relay module (5V 3-channel)
- 20x4 LCD display with I2C backpack (I2C address 0x27 or 0x3F)

### Deploy to Real Hardware
1. **Wire your SHT31 sensor** (see wiring diagrams below)
2. **Build and upload:**
   ```bash
   pio run -e uno-sht31 -t upload
   ```
3. **Monitor output:**
   ```bash
   pio device monitor -b 9600
   ```

### Available Build Environments

The project uses the `uno-sht31` environment for the SHT31 I2C sensor on Arduino Uno hardware.

**Build Commands:**
```bash
# Compile only
pio run -e uno-sht31

# Compile and upload to Arduino
pio run -e uno-sht31 -t upload

# Compile, upload, and monitor serial output
pio run -e uno-sht31 -t upload && pio device monitor -b 9600
```

## Upload to Physical Arduino

### 1. **Connect Arduino to Computer**
- Plug the Arduino Uno into your computer via USB cable
- The device will appear as `/dev/ttyUSB0`, `/dev/ttyACM0`, or `/dev/cu.usbserial-*` (depending on OS/driver)

### 2. **Upload to Arduino**

```bash
cd /Users/andresdegano/projects/Arduino/Dehydrator
pio run -e uno-sht31 -t upload
```

### 3. **Verify Successful Upload**
You should see output like:
```
Uploading .pio/build/uno-sht31/firmware.hex
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
pio run -e uno-sht31 -t upload --upload-port /dev/ttyUSB0
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
pio run -e uno-sht31 -t upload && pio device monitor -b 9600
```

### 7. **Hardware Wiring Reminder**

Make sure your Arduino is properly wired before uploading:
- **SHT31**: SDA → A4, SCL → A5, VCC → 5V, GND → GND
- **LCD 2004 I2C**: SDA → A4, SCL → A5, VCC → 5V, GND → GND
- **Relays**: Pins 8, 9, 10 to relay module IN pins
- **Buttons**: Pins 2, 3 to GND (active-LOW with pull-ups)

## Sensor Specifications

### SHT31 - I2C Sensor (Current Implementation)

**File:** `src/main.cpp`

**Specifications:**
- Protocol: Hardware I2C (Adafruit SHT31 library)
- Temperature range: -40°C to 125°C
- Humidity range: 0-100% RH
- Accuracy: ±0.3°C, ±2% RH
- Default I2C Address: 0x44 (auto-detected on 0x45 if needed)

**Wiring (Real Hardware):**
```
SHT31 VCC   → Arduino 5V
SHT31 GND   → Arduino GND
SHT31 SDA   → Arduino A4 (Pin 18)
SHT31 SCL   → Arduino A5 (Pin 19)
```

**Usage:**
```bash
pio run -e uno-sht31 -t upload
pio device monitor -b 9600
```

## Build & Deployment

### Hardware Wiring (All Components)

**Push Button Connections (Active-LOW):**
```
Button UP (Increase Temp)   → Arduino Pin 2 (other leg to GND)
Button DOWN (Decrease Temp) → Arduino Pin 3 (other leg to GND)
```
*Note: Buttons connected to GND using built-in pull-up resistors (INPUT_PULLUP mode)*

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

**LCD Display (20x4, I2C Mode with I2C Backpack):**
```
LCD I2C Backpack connections to Arduino:
LCD SDA (I2C Data)   → Arduino A4
LCD SCL (I2C Clock)  → Arduino A5
LCD VCC              → Arduino 5V
LCD GND              → Arduino GND
```

**Complete Wiring Diagram (SHT31 + LCD 2004 I2C):**
```
Arduino Uno
├── Pin 2  → Button UP (to GND, active-LOW)
├── Pin 3  → Button DOWN (to GND, active-LOW)
├── Pin 8  → Relay 1 (Hot Air Fan)
├── Pin 9  → Relay 2 (Cold Air Fan)
├── Pin 10 → Relay 3 (Electric Heater)
├── A4 (SDA) → SHT31 SDA + LCD I2C SDA (shared I2C bus)
└── A5 (SCL) → SHT31 SCL + LCD I2C SCL (shared I2C bus)

5V Rail
├── Relay Module VCC
├── SHT31 VCC
└── LCD VCC (through I2C backpack)

GND Rail
├── Relay Module GND
├── SHT31 GND
├── LCD GND (through I2C backpack)
├── Button UP pin
└── Button DOWN pin
```

## Project Structure

```
src/
  main.cpp          - SHT31 I2C sensor implementation

diagram.json        - Wokwi circuit diagram (visual representation)
wokwi.toml          - Wokwi emulator configuration
platformio.ini      - PlatformIO build & environment configuration
.gitignore          - Git ignore rules (build artifacts, sensitive data)
Readme.md           - This file
```

## Understanding the Code

### SHT31 Implementation (`src/main.cpp`)
- Initializes I2C communication on Arduino pins A4/A5 for SHT31 sensor and LCD 2004
- Detects SHT31 sensor on I2C bus (auto-detects address 0x44 or 0x45)
- Initializes LCD 2004 display in I2C mode (address 0x27, try 0x3F if not detected)
- **Button Handling**: Pins 2 (UP) and 3 (DOWN) adjust `targetTemp` variable (20-75°C range)
- Active-LOW buttons with counter-based debouncing (10 consecutive reads required)
- Reads temperature and humidity every 2 seconds
- Controls 3 relays based on dynamic temperature logic with ±2°C hysteresis
- Updates 20x4 LCD display with real-time status (4 rows of information)
- Outputs formatted data to serial (9600 baud)
- Handles sensor errors gracefully
- Example output: 
  ```
  [Temp: 60.42°C | Target: 60.0°C | Humidity: 64.53%] [Hot:ON | Heat:OFF | Cold:OFF]
  Target Temperature: 61.0°C (UP)
  ```

### Key Functions

**`handleButtonPresses()`** - Temperature adjustment with debouncing
- Reads pins 2 and 3 for button presses (active-LOW with pull-ups)
- Implements counter-based debounce logic (10 consecutive stable reads required)
- Validates temperature is within TEMP_MIN (20°C) and TEMP_MAX (75°C)
- Enforces 300ms minimum interval between button actions
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
- Range: 20°C to 75°C
- Changes are **immediate** and reflected on LCD display

### Change Default Starting Temperature (in Code)
Edit the source file (`src/main.cpp`):

```cpp
float targetTemp = 60.0;  // Change this value (20-75°C range)
```

Then rebuild and redeploy:
```bash
pio run --target clean
pio run -e uno-sht31 -t upload
```

### Fine-Tune Control Parameters (Advanced)
Edit `src/main.cpp` to adjust these parameters:

```cpp
#define HYSTERESIS 2.0          // ±2°C - reduce for tighter control, increase for stability
#define TEMP_MIN 20.0           // Lowest allowed target temperature via buttons
#define TEMP_MAX 75.0           // Highest allowed target temperature via buttons

// Button debouncing (in handleButtonPresses)
const uint8_t DEBOUNCE_COUNT = 10;       // Consecutive reads required (10 = ~10-20ms)
const unsigned long MIN_ACTION_INTERVAL = 300;  // milliseconds between button actions
```

**Common Configurations:**

For **herbs and leafy greens** (delicate):
```cpp
float targetTemp = 40.0;
#define HYSTERESIS 1.0
```

For **fruits and vegetables** (standard):
```cpp
float targetTemp = 60.0;       // Current default
#define HYSTERESIS 2.0
```

For **meats and jerky** (aggressive):
```cpp
float targetTemp = 70.0;
#define HYSTERESIS 2.0
```

For **tight temperature control** (±0.5°C):
```cpp
#define HYSTERESIS 0.5
// Buttons are always responsive with counter-based debouncing
```

For **loose temperature control** (±3°C, energy efficient):
```cpp
#define HYSTERESIS 3.0
// Buttons are always responsive with counter-based debouncing
```

After modifying, rebuild and redeploy:
```bash
pio run --target clean
pio run -e uno-sht31 -t upload
```

### Build Configuration
The project uses the `uno-sht31` environment for SHT31 sensor support. To rebuild:

```bash
pio run --target clean
pio run -e uno-sht31
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Sensor not detected (ERROR: Could not find SHT31) | Check SDA/SCL wiring (A4/A5), verify power (5V/GND), check I2C address (default 0x44) |
| Serial output not showing | Verify baud rate (9600), check USB connection, try device monitor -b 9600 |
| LCD display blank or not initializing | Check I2C address (default 0x27, try 0x3F), verify SDA/SCL connected to A4/A5, verify power |
| LCD shows garbage or wrong characters | Check I2C address is correct for your LCD backpack, verify I2C communication with both devices |
| Relays not activating | Check relay module power (5V), verify pin connections (8, 9, 10), check wire polarity |
| Relays switching erratically | Verify sensor is reading correctly, check I2C pull-ups (4.7kΩ), reduce HYSTERESIS if needed |
| Temperature not reaching target | Verify all fans/heater relay connections, check if relays are functioning with manual test |
| Buttons not responding | Verify pins 2 and 3 connected to GND (not 5V), check button contacts for debris |
| Buttons trigger too easily | Increase DEBOUNCE_COUNT from 10 to 15-20 in handleButtonPresses() |
| Buttons require hard press | Check button contacts, verify electrical connection to GND, reduce DEBOUNCE_COUNT if needed |
| I2C bus scan shows 0 devices | Check pull-up resistors on SDA/SCL (should be 4.7kΩ to 5V), verify power to sensors, verify both SHT31 and LCD are connected |
| System hangs on startup | Verify SHT31 sensor and LCD wiring before powering on, check for short circuits on I2C bus