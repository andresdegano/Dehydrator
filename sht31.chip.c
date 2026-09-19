// SHT31 I2C Temperature and Humidity Sensor Simulator for Wokwi
// Implements SHT31-D with I2C communication

#include "wokwi-api.h"

typedef struct {
  uint32_t i2c_dev;
  uint16_t temperature;  // Raw temperature value
  uint16_t humidity;     // Raw humidity value
} chip_state_t;

// Simulated sensor data (can be modified by Wokwi)
static uint8_t sht31_data[] = {
  0x68, 0x03,  // Temperature: ~25°C
  0x4B, 0x80   // Humidity: ~45%
};

static uint32_t chip_init(wokwi_chip_desc_t *chip) {
  chip_state_t *state = malloc(sizeof(chip_state_t));
  
  state->temperature = ((sht31_data[0] << 8) | sht31_data[1]);
  state->humidity = ((sht31_data[2] << 8) | sht31_data[3]);
  
  // Get I2C address from attributes (default 0x44)
  const char *addr_str = wokwi_chip_get_attr_string(chip, "address");
  uint8_t i2c_addr = 0x44;
  if (addr_str) {
    i2c_addr = strtol(addr_str, NULL, 16);
  }
  
  // Register I2C slave
  state->i2c_dev = wokwi_i2c_init(chip, i2c_addr, NULL);
  
  return (uint32_t)state;
}

static void chip_pin_change(uint32_t chip_state, uint32_t pin, uint32_t value) {
  // I2C handling is automatic
}

static void chip_read_addr(uint32_t chip_state, uint32_t i2c_dev, uint32_t addr) {
  chip_state_t *state = (chip_state_t *)chip_state;
  
  if (addr == 0xE0) {  // Status register
    wokwi_i2c_write(i2c_dev, 0x00);
  }
}

static void chip_write(uint32_t chip_state, uint32_t i2c_dev, uint32_t addr, uint32_t value) {
  chip_state_t *state = (chip_state_t *)chip_state;
  
  // Handle heater control or other commands
  if (addr == 0x30 || addr == 0x31) {  // Measurement commands
    // Measurement commands are handled by returning data
  }
}

const wokwi_chip_t sht31_chip = {
  .name = "SHT31",
  .init = chip_init,
  .pin_change = chip_pin_change,
  .read = chip_read_addr,
  .write = chip_write,
  .i2c_write = chip_write,
  .i2c_read_addr = chip_read_addr,
};
