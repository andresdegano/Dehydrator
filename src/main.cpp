#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <LiquidCrystal_I2C.h>

// Create SHT31 sensor instance (uses hardware I2C)
// Default I2C address: 0x44
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// LCD 2004 configuration (20 columns, 4 rows, I2C address 0x27)
LiquidCrystal_I2C lcd(0x27, 20, 4);

bool sensorFound = false;
uint8_t sht31_address = 0x44;

// Relay control pins
#define HOT_AIR_FAN_PIN 8   // Intake hot air at 70°C
#define COLD_AIR_FAN_PIN 9  // Intake cold air at 24°C
#define HEATER_PIN 10       // Electric heater backup

// Push button pins for temperature adjustment
#define BUTTON_UP_PIN 2     // Increase target temperature
#define BUTTON_DOWN_PIN 3   // Decrease target temperature

// LCD 2004 I2C configuration
#define LCD_I2C_ADDRESS 0x27  // Default I2C address for LCD 2004 (try 0x3F if this doesn't work)

// Temperature control setpoint (now variable, can be adjusted via buttons)
float targetTemp = 60.0;
#define HYSTERESIS 2.0      // ±2°C to prevent relay chatter
#define TEMP_MIN 20.0       // Minimum allowed target temperature
#define TEMP_MAX 75.0       // Maximum allowed target temperature

// Relay states
bool hotAirFanActive = false;
bool coldAirFanActive = false;
bool heaterActive = false;

void scanI2CBus() {
  int deviceCount = 0;
  
  for (uint8_t addr = 0; addr < 128; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      deviceCount++;
      
      // Check if it's an SHT31 (typically at 0x44 or 0x45)
      if (addr == 0x44 || addr == 0x45) {
        sht31_address = addr;
      }
    }
  }
}

void setup() {
  // Initialize serial communication FIRST (critical for diagnostics)
  Serial.begin(9600);
  delay(1000);
  
  // Initialize relay pins as outputs
  pinMode(HOT_AIR_FAN_PIN, OUTPUT);
  pinMode(COLD_AIR_FAN_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  
  // Initialize button pins with pull-ups (buttons wired to ground)
  // Buttons are active-LOW: pressed = LOW, not pressed = HIGH
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  
  // Start with relays OFF
  digitalWrite(HOT_AIR_FAN_PIN, LOW);
  digitalWrite(COLD_AIR_FAN_PIN, LOW);
  digitalWrite(HEATER_PIN, LOW);
  
  // Wait for LCD power to stabilize
  delay(200);
  
  // Initialize I2C LCD display (2004 format: 20x4)
  lcd.init();
  delay(100);
  
  // Enable backlight
  lcd.backlight();
  delay(100);
  
  // Clear display
  lcd.clear();
  delay(100);
  
  // Test LCD by displaying initialization message
  lcd.setCursor(0, 0);
  lcd.print("Dehydrator");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(3000);
  lcd.clear();
  
  // Initialize I2C for SHT31 sensor (after LCD)
  Wire.begin();
  delay(500);
  
  // Wait for serial to be ready
  delay(2000);
  
  // Scan I2C bus
  scanI2CBus();
  
  // Try to initialize sensor at detected address
  if (sht31.begin(sht31_address)) {
    sensorFound = true;
  } else {
    // Try alternate address
    uint8_t alt_addr = (sht31_address == 0x44) ? 0x45 : 0x44;
    
    if (sht31.begin(alt_addr)) {
      sensorFound = true;
      sht31_address = alt_addr;
    }
  }
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
    }
    
    // If still too cold, activate electric heater backup
    if (temperature < (targetTemp - HYSTERESIS - 2.0)) {
      if (!heaterActive) {
        digitalWrite(HEATER_PIN, HIGH);
        heaterActive = true;
      }
    } else if (temperature >= targetTemp) {
      // Turn off heater once target is reached
      if (heaterActive) {
        digitalWrite(HEATER_PIN, LOW);
        heaterActive = false;
      }
    }
  } else if (temperature > (targetTemp + HYSTERESIS)) {
    // Temperature too high: use COLD AIR FAN to cool
    if (!coldAirFanActive) {
      digitalWrite(COLD_AIR_FAN_PIN, HIGH);
      digitalWrite(HOT_AIR_FAN_PIN, LOW);
      coldAirFanActive = true;
      hotAirFanActive = false;
    }
    
    // Make sure heater is off when cooling
    if (heaterActive) {
      digitalWrite(HEATER_PIN, LOW);
      heaterActive = false;
    }
  } else {
    // Temperature in stable zone: keep currently active fan running
    // If no fan is active, start with hot air fan as default
    if (!hotAirFanActive && !coldAirFanActive) {
      digitalWrite(HOT_AIR_FAN_PIN, HIGH);
      hotAirFanActive = true;
    }
    
    // Ensure heater is off in stable zone
    if (heaterActive) {
      digitalWrite(HEATER_PIN, LOW);
      heaterActive = false;
    }
  }
}

void updateLCDDisplay(float temperature, float humidity) {
  char buffer[21];
  char tempStr[10];
  
  // Row 0: Temperature and humidity (exactly 20 chars)
  lcd.setCursor(0, 0);
  dtostrf(temperature, 4, 1, tempStr);
  // Remove leading spaces from temperature string
  char *tempPtr = tempStr;
  while (*tempPtr == ' ') tempPtr++;
  sprintf(buffer, "T:%sC H:%2d%%", tempPtr, (int)humidity);
  int len = strlen(buffer);
  for(int i = len; i < 20; i++) buffer[i] = ' ';
  buffer[20] = '\0';
  lcd.print(buffer);
  
  // Row 1: Target temperature (exactly 20 chars)
  lcd.setCursor(0, 1);
  sprintf(buffer, "Target: %2dC", (int)targetTemp);
  len = strlen(buffer);
  for(int i = len; i < 20; i++) buffer[i] = ' ';
  buffer[20] = '\0';
  lcd.print(buffer);
  
  // Row 2: Heating/Cooling Status (exactly 20 chars)
  lcd.setCursor(0, 2);
  if (heaterActive) {
    sprintf(buffer, "Status: HEATER ON");
  } else if (hotAirFanActive) {
    sprintf(buffer, "Status: HOT FAN");
  } else if (coldAirFanActive) {
    sprintf(buffer, "Status: COLD FAN");
  } else {
    sprintf(buffer, "Status: IDLE");
  }
  len = strlen(buffer);
  for(int i = len; i < 20; i++) buffer[i] = ' ';
  buffer[20] = '\0';
  lcd.print(buffer);
  
  // Row 3: Button instructions (exactly 20 chars)
  lcd.setCursor(0, 3);
  sprintf(buffer, "Adjust: +/-");
  len = strlen(buffer);
  for(int i = len; i < 20; i++) buffer[i] = ' ';
  buffer[20] = '\0';
  lcd.print(buffer);
}

void handleButtonPresses() {
  // Robust counter-based debouncing to filter noise
  static uint8_t buttonUpCounter = 0;      // Counter for debouncing
  static uint8_t buttonDownCounter = 0;    // Counter for debouncing
  static bool buttonUpStable = HIGH;       // Last confirmed stable state (HIGH = not pressed with pull-up)
  static bool buttonDownStable = HIGH;     // Last confirmed stable state (HIGH = not pressed with pull-up)
  static unsigned long lastButtonUpActionTime = 0;
  static unsigned long lastButtonDownActionTime = 0;
  
  const uint8_t DEBOUNCE_COUNT = 10;       // Require 10 consecutive reads to confirm (eliminates noise)
  const unsigned long MIN_ACTION_INTERVAL = 300;  // 300ms minimum between actions
  unsigned long currentTime = millis();
  
  // === UP Button: Accumulate readings (active-LOW) ===
  bool currentUpReading = digitalRead(BUTTON_UP_PIN);
  
  if (currentUpReading == LOW) {
    // Reading is LOW - increment counter (button pressed, saturate at DEBOUNCE_COUNT)
    if (buttonUpCounter < DEBOUNCE_COUNT) {
      buttonUpCounter++;
    }
  } else {
    // Reading is HIGH - decrement counter (button released, saturate at 0)
    if (buttonUpCounter > 0) {
      buttonUpCounter--;
    }
  }
  
  // Once counter reaches threshold, update stable state
  if (buttonUpCounter >= DEBOUNCE_COUNT && buttonUpStable == HIGH) {
    // Transitioned from HIGH to LOW - button pressed!
    buttonUpStable = LOW;
    
    if (currentTime - lastButtonUpActionTime >= MIN_ACTION_INTERVAL) {
      if (targetTemp < TEMP_MAX) {
        targetTemp += 1.0;
      }
      lastButtonUpActionTime = currentTime;
    }
  } else if (buttonUpCounter == 0 && buttonUpStable == LOW) {
    // Transitioned from LOW to HIGH - button released
    buttonUpStable = HIGH;
  }
  
  // === DOWN Button: Accumulate readings (active-LOW) ===
  bool currentDownReading = digitalRead(BUTTON_DOWN_PIN);
  
  if (currentDownReading == LOW) {
    // Reading is LOW - increment counter (button pressed)
    if (buttonDownCounter < DEBOUNCE_COUNT) {
      buttonDownCounter++;
    }
  } else {
    // Reading is HIGH - decrement counter (button released)
    if (buttonDownCounter > 0) {
      buttonDownCounter--;
    }
  }
  
  // Once counter reaches threshold, update stable state
  if (buttonDownCounter >= DEBOUNCE_COUNT && buttonDownStable == HIGH) {
    // Transitioned from HIGH to LOW - button pressed!
    buttonDownStable = LOW;
    
    if (currentTime - lastButtonDownActionTime >= MIN_ACTION_INTERVAL) {
      if (targetTemp > TEMP_MIN) {
        targetTemp -= 1.0;
      }
      lastButtonDownActionTime = currentTime;
    }
  } else if (buttonDownCounter == 0 && buttonDownStable == LOW) {
    // Transitioned from LOW to HIGH - button released
    buttonDownStable = HIGH;
  }
  

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
      return;
    }
    
    // Read sensor data
    float temperature = sht31.readTemperature();
    float humidity = sht31.readHumidity();
    
    // Check for valid readings
    if (!isnan(temperature) && !isnan(humidity)) {
      // Update LCD display
      updateLCDDisplay(temperature, humidity);
      
      // Control relays based on temperature
      controlRelays(temperature);
    }
  }
}
