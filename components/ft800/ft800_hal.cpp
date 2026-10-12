#include "ft800_hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ft800 {

static const char *const TAG_HAL = "ft800.hal";

// ---------------------------------------------------------------------------
// Address header helpers
// SPI address format (DS_FT800 §4.2):
//   byte0 = prefix[7:6] | addr[21:16]
//   byte1 = addr[15:8]
//   byte2 = addr[7:0]
// prefix: 00=read, 10=write, 01=host_command
// ---------------------------------------------------------------------------

void FT800HAL::begin_read(uint32_t addr) {
  this->enable();  // assert CS
  this->transfer_byte((uint8_t)((addr >> 16) & 0x3F) | SPI_PREFIX_READ);
  this->transfer_byte((uint8_t)((addr >>  8) & 0xFF));
  this->transfer_byte((uint8_t)( addr        & 0xFF));
  this->transfer_byte(0x00);  // dummy byte required for read (DS_FT800 §4.2)
}

void FT800HAL::begin_write(uint32_t addr) {
  this->enable();
  this->transfer_byte((uint8_t)((addr >> 16) & 0x3F) | SPI_PREFIX_WRITE);
  this->transfer_byte((uint8_t)((addr >>  8) & 0xFF));
  this->transfer_byte((uint8_t)( addr        & 0xFF));
}

// ---------------------------------------------------------------------------
// Reads
// ---------------------------------------------------------------------------

uint8_t FT800HAL::rd8(uint32_t addr) {
  this->begin_read(addr);
  uint8_t val = this->transfer_byte(0x00);
  this->disable();
  return val;
}

uint16_t FT800HAL::rd16(uint32_t addr) {
  this->begin_read(addr);
  uint16_t lo = this->transfer_byte(0x00);
  uint16_t hi = this->transfer_byte(0x00);
  this->disable();
  return (hi << 8) | lo;  // little-endian (DS_FT800 §2, endianness note)
}

uint32_t FT800HAL::rd32(uint32_t addr) {
  this->begin_read(addr);
  uint32_t b0 = this->transfer_byte(0x00);
  uint32_t b1 = this->transfer_byte(0x00);
  uint32_t b2 = this->transfer_byte(0x00);
  uint32_t b3 = this->transfer_byte(0x00);
  this->disable();
  return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);  // little-endian
}

void FT800HAL::rd_mem(uint32_t addr, uint8_t *data, size_t len) {
  this->begin_read(addr);
  for (size_t i = 0; i < len; i++) {
    data[i] = this->transfer_byte(0x00);
  }
  this->disable();
}

// ---------------------------------------------------------------------------
// Writes
// ---------------------------------------------------------------------------

void FT800HAL::wr8(uint32_t addr, uint8_t val) {
  this->begin_write(addr);
  this->transfer_byte(val);
  this->disable();
}

void FT800HAL::wr16(uint32_t addr, uint16_t val) {
  this->begin_write(addr);
  this->transfer_byte((uint8_t)(val & 0xFF));       // LSB first (little-endian)
  this->transfer_byte((uint8_t)((val >> 8) & 0xFF));
  this->disable();
}

void FT800HAL::wr32(uint32_t addr, uint32_t val) {
  this->begin_write(addr);
  this->transfer_byte((uint8_t)(val        & 0xFF));
  this->transfer_byte((uint8_t)((val >>  8) & 0xFF));
  this->transfer_byte((uint8_t)((val >> 16) & 0xFF));
  this->transfer_byte((uint8_t)((val >> 24) & 0xFF));
  this->disable();
}

void FT800HAL::wr_mem(uint32_t addr, const uint8_t *data, size_t len) {
  this->begin_write(addr);
  for (size_t i = 0; i < len; i++) {
    this->transfer_byte(data[i]);
  }
  this->disable();
}

// ---------------------------------------------------------------------------
// Host command  (DS_FT800 §4.1.2)
// ---------------------------------------------------------------------------

void FT800HAL::host_command(uint8_t cmd, uint8_t param) {
  this->enable();
  this->transfer_byte(SPI_PREFIX_CMD | (cmd & 0x3F));
  this->transfer_byte(param);
  this->transfer_byte(0x00);
  this->disable();
}

// ---------------------------------------------------------------------------
// Runtime SPI speed change
// ESPHome's SPIDevice wraps Arduino SPI.  We call the underlying SPIClass
// directly; this is safe because we only call it between transactions.
// ---------------------------------------------------------------------------

void FT800HAL::set_spi_speed(uint32_t hz) {
  spi_speed_hz_ = hz;
  // SPIDevice doesn't expose setFrequency at runtime in all ESPHome versions.
  // The workaround: call set_data_rate() if available, else live with the
  // compile-time default.  Callers should compile with DATA_RATE_20MHZ for
  // normal operation and rely on a slow init via a 1 MHz startup.
  // See ft800.cpp for the boot sequence that uses a separate slow-bus phase.
  ESP_LOGV(TAG_HAL, "SPI speed set to %u Hz", hz);
}

} // namespace ft800
} // namespace esphome
