#include <Arduino.h>
#include <SimpleDHT.h>
#include <LiquidCrystal_I2C.h>

// DHT22 sensor pin (DHT22 has wider temperature range than DHT11)
#define DHT_PIN 7

// Relay control pins
#define HOT_AIR_FAN_PIN 8   // Intake hot air at 70°C
#define COLD_AIR_FAN_PIN 9  // Intake cold air at 24°C
#define HEATER_PIN 10       // Electric heater backup

// LCD configuration (I2C address 0x3F, 16 columns, 2 rows)
// Note: Wokwi uses 0x3F, typical modules use 0x27
LiquidCrystal_I2C lcd(0x3F, 16, 2);

// Temperature control setpoint
#define TARGET_TEMP 60.0
#define HYSTERESIS 1.0      // ±1°C to prevent relay chatter
#define HEATER_THRESHOLD 55.0  // Activate heater below this temp

// Relay states
bool hotAirFanActive = false;
bool coldAirFanActive = false;
bool heaterActive = false;

// Create DHT22 instance
SimpleDHT22 dht22(DHT_PIN);

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  
  // Initialize relay pins as outputs
  pinMode(HOT_AIR_FAN_PIN, OUTPUT);
  pinMode(COLD_AIR_FAN_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  
  // Start with relays OFF
  digitalWrite(HOT_AIR_FAN_PIN, LOW);
  digitalWrite(COLD_AIR_FAN_PIN, LOW);
  digitalWrite(HEATER_PIN, LOW);
  
  // Initialize I2C for LCD before anything else
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
  
  Serial.println("Dehydrator Control System (DHT22)");
  Serial.println("=========================================");
  Serial.println("Target Temperature: 60°C");
  Serial.println("Hot Air Fan (Pin 8): Activates at <59°C");  Serial.println("Electric Heater (Pin 10): Activates at <55°C");  Serial.println("Cold Air Fan (Pin 9): Activates at >61°C");
  Serial.println("=========================================\n");
}

void updateLCDDisplay(float temperature, float humidity) {
  // Line 1: Temperature and Humidity
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print("C ");
  lcd.print("H:");
  lcd.print(humidity, 0);
  lcd.print("%");
  
  // Line 2: Heating/Cooling Status
  lcd.setCursor(0, 1);
  if (heaterActive) {
    lcd.print("HEATER ON ");
  } else if (hotAirFanActive) {
    lcd.print("HOT FAN   ");
  } else if (coldAirFanActive) {
    lcd.print("COLD FAN  ");
  } else {
    lcd.print("IDLE      ");
  }
  
  // Show target temperature
  lcd.print("T:");
  lcd.print((int)TARGET_TEMP);
  lcd.print("C");
}

void controlRelays(float temperature) {
  // Ensure at least one fan is always running to prevent humidity stagnation
  
  if (temperature < (TARGET_TEMP - HYSTERESIS)) {
    // Temperature too low: use HOT AIR FAN to heat
    if (!hotAirFanActive) {
      digitalWrite(HOT_AIR_FAN_PIN, HIGH);
      digitalWrite(COLD_AIR_FAN_PIN, LOW);
      hotAirFanActive = true;
      coldAirFanActive = false;
      Serial.println("  > HOT AIR FAN ON (heating)");
    }
    
    // If still too cold, activate electric heater backup
    if (temperature < HEATER_THRESHOLD) {
      if (!heaterActive) {
        digitalWrite(HEATER_PIN, HIGH);
        heaterActive = true;
        Serial.println("  > ELECTRIC HEATER ON (backup)");
      }
    } else if (temperature >= TARGET_TEMP) {
      // Turn off heater once target is reached
      if (heaterActive) {
        digitalWrite(HEATER_PIN, LOW);
        heaterActive = false;
        Serial.println("  > ELECTRIC HEATER OFF");
      }
    }
  } else if (temperature > (TARGET_TEMP + HYSTERESIS)) {
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
    // Temperature in stable zone (59-61°C): keep currently active fan running
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

void loop() {
  // Read DHT22 sensor
  float temperature = 0;
  float humidity = 0;
  int err = SimpleDHTErrSuccess;
  
  if ((err = dht22.read2(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("ERROR reading DHT22: ");
    Serial.println(err);
    delay(2000);
    return;
  }
  
  // Display readings
  Serial.print("[Temp: ");
  Serial.print(temperature, 1);
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
  
  // Wait 2 seconds before next reading
  delay(2000);
}
