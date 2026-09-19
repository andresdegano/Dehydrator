# Arduino Dehydrator - Sensor Versions

This project has two sensor implementations:

## 1. SHT31 Sensor (Software I2C)
**File:** `src/i2c_main.cpp`  
**Pins:** 
- SDA → Pin 4
- SCL → Pin 5
- VCC → 5V
- GND → GND

**Libraries:**
- robtillaart/SHT31_SW@^0.3.2
- stevemarple/SoftWire@^2.0.10

**Features:**
- Uses software I2C (flexible pin selection)
- Reads temperature and humidity
- Displays both Celsius and Fahrenheit
- Consistent readings every 2 seconds

**Run:**
```bash
platformio run --target upload && platformio device monitor --baud 9600
```

---

## 2. DHT11 Sensor (Digital)
**File:** `src/dht11_main.cpp`  
**Pins:**
- DATA → Pin 7
- VCC → 5V
- GND → GND

**Libraries:**
- winlinvip/SimpleDHT@^1.0.15

**Features:**
- Single data pin interface
- Reads temperature (0-50°C) and humidity (20-90%)
- Lower accuracy than SHT31 but simpler wiring
- Readings every 2 seconds

---

## How to Switch Between Versions

Since both files are in `src/`, PlatformIO will try to compile both. To use the DHT11 version:

### Option 1: Use symbolic link to select active version
```bash
cd src/
ln -sf i2c_main.cpp main.cpp      # Use I2C (SHT31)
ln -sf dht11_main.cpp main.cpp    # Use DHT11
```

### Option 2: Use PlatformIO's src_filter
Edit `platformio.ini` to compile only one version:
```ini
[env:uno]
src_filter = +<i2c_main.cpp>      # For I2C version
```
or
```ini
[env:uno]
src_filter = +<dht11_main.cpp>    # For DHT11 version
```

---

## Wiring Comparison

### SHT31 (I2C)
```
       +5V
        |
       10k
        |
SDA ----+---- Arduino Pin 4
        |
    [SHT31]
        |
SCL ----+---- Arduino Pin 5
        |
       10k
        |
       GND
```

### DHT11 (Single Pin)
```
+5V
  |
[DHT11]
  |
GND

DATA Pin → Arduino Pin 7
```

---

## Troubleshooting

**SHT31 not detected:**
- Add 10kΩ pull-up resistors on SDA and SCL
- Verify wiring to pins 4 and 5
- Check sensor address with I2C scanner

**DHT11 reading errors:**
- Ensure data pin is pin 7
- Add 10kΩ pull-up resistor on data line
- DHT11 needs ~1-2 seconds between readings

---

## Library Documentation
- SHT31: https://github.com/RobTillaart/SHT31
- SimpleDHT: https://github.com/siara-cc/SimpleDHT
