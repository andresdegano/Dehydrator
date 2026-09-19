# Arduino Dehydrator Project

Arduino project for Solar powered Dehydrator and electric heater as backup

## Sensor Versions

Two sensor implementations are available:

1. **DHT22 (Digital Sensor - Single Pin)**
   - File: `dht_main.cpp`
   - Protocol: Single-wire digital (SimpleDHT22 library)
   - Pin: GPIO 7
   - Temperature range: -40°C to 125°C
   - ✅ **Fully supported in Wokwi emulator**

2. **SHT31 (I2C Sensor)**
   - File: `i2c_main.cpp`
   - Protocol: Hardware I2C (Adafruit SHT31 library)
   - Pins: A4 (SDA), (Recommended for Wokwi):**
```bash
pio run -e uno-wokwi
```
Then in VS Code: Open command palette (⌘+Shift+P) → "Wokwi: Start Simulator"

The DHT22 sensor works great in Wokwi emulation and lets you test the full sensor reading pipeline.

**SHT31 I2C Version:**
The I2C code (`i2c_main.cpp`) is fully functional and correct for real hardware. However, Wokwi's SHT31 component doesn't properly simulate I2C communication, so testing in emulator will show connection errors.

**To test SHT31 on real hardware:**
```bash
pio run -e i2c-wokwi -t upload
```
This uploads the SHT31 I2C code directly to your Arduino Uno.

**Real Hardware I2C Wiring (SHT31):**
- SDA: A4 (Pin 18)
- SCL: A5 (Pin 19)
- VCC: 5V
- GND: GND
Then in VS Code: Open command palette (⌘+Shift+P) → "Wokwi: Start Simulator"

**I2C Wiring (Hardware I2C on Arduino Uno):**
- SDA: A4 (Pin 18)
- SCL: A5 (Pin 19)
- VCC: 5V
- GND: GND

The simulator shows:
- Arduino Uno board visualization
- Connected sensor component (DHT22 or SHT31)
- Serial monitor output with temperature/humidity readings
- Custom SHT31 chip simulation for I2C communication

### Hardware Upload

**For DHT22:**
```bash
pio run -e uno -t upload
```

**For SHT31 I2C:**
```bash
pio run -e uno -t upload  # Select appropriate environment
```

## Project Structure

```
src/
  dht_main.cpp   - DHT22 sensor implementation
  i2c_main.cpp   - SHT31 I2C sensor implementation
diagram.json     - Wokwi circuit diagram
wokwi.toml       - Wokwi emulator configuration
platformio.ini   - PlatformIO configuration
```

## Temperature Display

The DHT22 implementation supports:
- Temperature range: -40°C to 125°C
- Decimal precision: 0.1°C
- Negative temperature handling for cold environment testing