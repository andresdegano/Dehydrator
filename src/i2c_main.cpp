#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <LiquidCrystal_I2C.h>

// Create SHT31 sensor instance (uses hardware I2C)
// Default I2C address: 0x44
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// LCD configuration (I2C address 0x3F, 16 columns, 2 rows)
// Note: Wokwi uses 0x3F, typical modules use 0x27
LiquidCrystal_I2C lcd(0x3F, 16, 2);

bool sensorFound = false;
uint8_t sht31_address = 0x44;

// Relay control pins
#define HOT_AIR_FAN_PIN 8   // Intake hot air at 70°C
#define COLD_AIR_FAN_PIN 9  // Intake cold air at 24°C
#define HEATER_PIN 10       // Electric heater backup

// Push button pins for temperature adjustment
#define BUTTON_UP_PIN 2     // Increase target temperature
#define BUTTON_DOWN_PIN 3   // Decrease target temperature

// Temperature control setpoint (now variable, can be adjusted via buttons)
float targetTemp = 60.0;
#define HYSTERESIS 2.0      // ±2°C to prevent relay chatter
#define HEATER_THRESHOLD 54.0  // Activate heater below this temp
#define TEMP_MIN 35.0       // Minimum allowed target temperature
#define TEMP_MAX 75.0       // Maximum allowed target temperature

// Relay states
bool hotAirFanActive = false;
bool coldAirFanActive = false;
bool heaterActive = false;

void scanI2CBus() {
  Serial.println("\nScanning I2C bus for devices...");
  int deviceCount = 0;
  
  for (uint8_t addr = 0; addr < 128; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at address: 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      deviceCount++;
      
      // Check if it's an SHT31 (typically at 0x44 or 0x45)
      if (addr == 0x44 || addr == 0x45) {
        sht31_address = addr;
      }
    }
  }
  
  Serial.print("Total devices found: ");
  Serial.println(deviceCount);
  Serial.println();
}

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  
  // Initialize relay pins as outputs
  pinMode(HOT_AIR_FAN_PIN, OUTPUT);
  pinMode(COLD_AIR_FAN_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  
  // Initialize button pins as inputs (no pull-up - buttons will be connected to 5V)
  // This reduces power consumption: current only flows when button is pressed
  pinMode(BUTTON_UP_PIN, INPUT);
  pinMode(BUTTON_DOWN_PIN, INPUT);
  
  // Start with relays OFF
  digitalWrite(HOT_AIR_FAN_PIN, LOW);
  digitalWrite(COLD_AIR_FAN_PIN, LOW);
  digitalWrite(HEATER_PIN, LOW);
  
  // Initialize I2C for both LCD and SHT31 sensor
  Wire.begin();
  delay(100);
  
  // Initialize LCD display
  lcd.init();
  delay(100);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Dehydrator");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();
  
  // Wait for serial to be ready
  delay(2000);
  
  Serial.println("Dehydrator Control System (SHT31)");
  Serial.println("=========================================");
  Serial.println("Temperature Control System");
  Serial.println("Target Temperature: 60°C (adjustable via buttons)");
  Serial.println("Button UP (Pin 2): Increase target temp");
  Serial.println("Button DOWN (Pin 3): Decrease target temp");
  Serial.println("Hysteresis: ±2°C");
  Serial.println("SHT31 connected to:");
  Serial.println("  SDA: A4 (Pin 18)");
  Serial.println("  SCL: A5 (Pin 19)");
  Serial.println("  VCC: 5V");
  Serial.println("  GND: GND\n");
  
  // Initialize I2C
  Wire.begin();
  delay(500);
  
  // Scan I2C bus
  scanI2CBus();
  
  // Try to initialize sensor at detected address
  if (sht31.begin(sht31_address)) {
    sensorFound = true;
    Serial.print("SHT31 sensor initialized successfully at 0x");
    if (sht31_address < 16) Serial.print("0");
    Serial.println(sht31_address, HEX);
    Serial.print("Heater status: ");
    Serial.println(sht31.isHeaterEnabled() ? "ON" : "OFF");
  } else {
    // Try alternate address
    uint8_t alt_addr = (sht31_address == 0x44) ? 0x45 : 0x44;
    Serial.print("Failed to find SHT31 at 0x");
    if (sht31_address < 16) Serial.print("0");
    Serial.println(sht31_address, HEX);
    Serial.print("Trying alternate address 0x");
    if (alt_addr < 16) Serial.print("0");
    Serial.println(alt_addr, HEX);
    
    if (sht31.begin(alt_addr)) {
      sensorFound = true;
      sht31_address = alt_addr;
      Serial.println("SHT31 sensor found at alternate address!");
    } else {
      Serial.println("ERROR: Could not find SHT31 sensor!");
      Serial.println("Check wiring, power, and I2C pull-ups.\n");
    }
  }
  Serial.println();
}

void controlRelays(float temperature) {
  // Ensure at least one fan is always running to prevent humidity stagnation
  
  if (temperature < (targetTemp - HYSTERESIS)) {
    // Temperature too low: use HOT AIR FAN to heat
    if (!hotAirFanActive) {
      digitalWrite(HOT_AIR_FAN_PIN, HIGH);
      digitalWrite(COLD_AIR_FAN_PIN, LOW);
      hotAirFanActive = true;
      coldAirFanActive = false;
      Serial.println("  > HOT AIR FAN ON (heating)");
    }
    
    // If still too cold, activate electric heater backup
    if (temperature < (targetTemp - HYSTERESIS - 2.0)) {
      if (!heaterActive) {
        digitalWrite(HEATER_PIN, HIGH);
        heaterActive = true;
        Serial.println("  > ELECTRIC HEATER ON (backup)");
      }
    } else if (temperature >= targetTemp) {
      // Turn off heater once target is reached
      if (heaterActive) {
        digitalWrite(HEATER_PIN, LOW);
        heaterActive = false;
        Serial.println("  > ELECTRIC HEATER OFF");
      }
    }
  } else if (temperature > (targetTemp + HYSTERESIS)) {
    // Temperature too high: use COLD AIR FAN to cool
    if (!coldAirFanActive) {
      digitalWrite(COLD_AIR_FAN_PIN, HIGH);
      digitalWrite(HOT_AIR_FAN_PIN, LOW);
      coldAirFanActive = true;
      hotAirFanActive = false;
      Serial.println("  > COLD AIR FAN ON (cooling)");
    }
    
    // Make sure heater is off when cooling
    if (heaterActive) {
      digitalWrite(HEATER_PIN, LOW);
      heaterActive = false;
      Serial.println("  > ELECTRIC HEATER OFF");
    }
  } else {
    // Temperature in stable zone: keep currently active fan running
    // If no fan is active, start with hot air fan as default
    if (!hotAirFanActive && !coldAirFanActive) {
      digitalWrite(HOT_AIR_FAN_PIN, HIGH);
      hotAirFanActive = true;
      Serial.println("  > HOT AIR FAN ON (circulation)");
    }
    
    // Ensure heater is off in stable zone
    if (heaterActive) {
      digitalWrite(HEATER_PIN, LOW);
      heaterActive = false;
      Serial.println("  > ELECTRIC HEATER OFF");
    }
  }
}

void updateLCDDisplay(float temperature, float humidity) {
  // Line 1: Temperature reading → Target and Humidity
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print("→");
  lcd.print((int)targetTemp);
  lcd.print("C ");
  lcd.print("H:");
  lcd.print(humidity, 0);
  lcd.print("%");
  
  // Line 2: Heating/Cooling Status
  lcd.setCursor(0, 1);
  if (heaterActive) {
    lcd.print("HEATER ON");
  } else if (hotAirFanActive) {
    lcd.print("HOT FAN  ");
  } else if (coldAirFanActive) {
    lcd.print("COLD FAN ");
  } else {
    lcd.print("IDLE     ");
  }
  
  // Show button instructions
  lcd.print(" +/-");
}

void handleButtonPresses() {
  // Static variables persist between function calls
  static bool lastButtonUpState = LOW;      // Start LOW (not pressed)
  static bool lastButtonDownState = LOW;    // Start LOW (not pressed)
  
  // Check UP button (increase temperature) - trigger only on press transition
  // Note: Wokwi buttons are active-HIGH (pressed = HIGH, not pressed = LOW)
  bool currentUpState = digitalRead(BUTTON_UP_PIN);
  if (currentUpState == HIGH && lastButtonUpState == LOW) {
    if (targetTemp < TEMP_MAX) {
      targetTemp += 1.0;
      Serial.print("Target Temperature: ");
      Serial.print(targetTemp, 1);
      Serial.println("°C (increased via button)");
    }
    delay(300);  // Debounce delay
  }
  lastButtonUpState = currentUpState;
  
  // Check DOWN button (decrease temperature) - trigger only on press transition
  bool currentDownState = digitalRead(BUTTON_DOWN_PIN);
  if (currentDownState == HIGH && lastButtonDownState == LOW) {
    if (targetTemp > TEMP_MIN) {
      targetTemp -= 1.0;
      Serial.print("Target Temperature: ");
      Serial.print(targetTemp, 1);
      Serial.println("°C (decreased via button)");
    }
    delay(300);  // Debounce delay
  }
  lastButtonDownState = currentDownState;
}

void loop() {
  static unsigned long lastSensorReadTime = 0;
  unsigned long currentTime = millis();
  
  // Check buttons frequently (every loop iteration without delay)
  handleButtonPresses();
  
  // Only read sensor and control relays every 2 seconds (non-blocking)
  if (currentTime - lastSensorReadTime >= 2000) {
    lastSensorReadTime = currentTime;
    
    if (!sensorFound) {
      Serial.println("Sensor not initialized. Check connection.");
      return;
    }
    
    // Read sensor data
    float temperature = sht31.readTemperature();
    float humidity = sht31.readHumidity();
    
    // Check for valid readings
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("ERROR: Failed to read from SHT31!");
    } else {
      // Display readings
      Serial.print("[Temp: ");
      Serial.print(temperature, 1);
      Serial.print("°C | Target: ");
      Serial.print(targetTemp, 1);
      Serial.print("°C | Humidity: ");
      Serial.print(humidity, 1);
      Serial.print("%] ");
      Serial.print("[Hot:");
      Serial.print(hotAirFanActive ? "ON" : "OFF");
      Serial.print(" | Heat:");
      Serial.print(heaterActive ? "ON" : "OFF");
      Serial.print(" | Cold:");
      Serial.print(coldAirFanActive ? "ON" : "OFF");
      Serial.println("]");
      
      // Update LCD display
      updateLCDDisplay(temperature, humidity);
      
      // Control relays based on temperature
      controlRelays(temperature);
    }
  }
}
