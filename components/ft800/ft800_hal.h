#pragma once
// FT800 Hardware Abstraction Layer — thin wrapper around ESPHome's SPIDevice.
// This file has no dependencies on any FT800 widget or display logic.
// Swapping this file for an STM32 SPI HAL is sufficient for a bare-metal port.
//
// SPI protocol (DS_FT800 §4.2):
//   Read:  [addr_hi | 0x00][addr_mid][addr_lo][dummy_byte] [data...]
//   Write: [addr_hi | 0x80][addr_mid][addr_lo]              [data...]
//   Host:  [0x40 | cmd][param][0x00]

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "ft800_registers.h"

namespace esphome {
namespace ft800 {

class FT800HAL : public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST,
                                       spi::CLOCK_POLARITY_LOW,
                                       spi::CLOCK_PHASE_LEADING,
                                       spi::DATA_RATE_1MHZ> {
 public:
  // ---------------------------------------------------------------------------
  // Single-value reads
  // ---------------------------------------------------------------------------
  uint8_t  rd8 (uint32_t addr);
  uint16_t rd16(uint32_t addr);
  uint32_t rd32(uint32_t addr);

  // ---------------------------------------------------------------------------
  // Single-value writes
  // ---------------------------------------------------------------------------
  void wr8 (uint32_t addr, uint8_t  val);
  void wr16(uint32_t addr, uint16_t val);
  void wr32(uint32_t addr, uint32_t val);

  // ---------------------------------------------------------------------------
  // Burst write — write `len` bytes from `data` starting at `addr`.
  // Keeps CS low for the entire transfer; wraps at RAM_CMD boundary if needed.
  // ---------------------------------------------------------------------------
  void wr_mem(uint32_t addr, const uint8_t *data, size_t len);

  // ---------------------------------------------------------------------------
  // Burst read — read `len` bytes into `data` starting at `addr`.
  // ---------------------------------------------------------------------------
  void rd_mem(uint32_t addr, uint8_t *data, size_t len);

  // ---------------------------------------------------------------------------
  // Host command — 3-byte SPI transaction with 01 prefix  (DS_FT800 §4.1.2)
  // cmd: one of HOSTCMD_* constants; param: command parameter (often 0)
  // ---------------------------------------------------------------------------
  void host_command(uint8_t cmd, uint8_t param = 0x00);

  // ---------------------------------------------------------------------------
  // Change SPI clock rate at runtime.
  // FT800 init must use ≤ 11 MHz; after boot can use up to 30 MHz.
  // ESPHome SPIDevice does not expose runtime rate changes cleanly on all
  // targets, so we implement it by re-configuring the bus if the platform
  // supports it.  On platforms that don't, the compile-time rate is used.
  // ---------------------------------------------------------------------------
  void set_spi_speed(uint32_t hz);

 protected:
  // Build and send the 3-byte address header for a read or write transaction.
  // Returns with CS asserted (low); caller must call spi_->end_transaction().
  void begin_read (uint32_t addr);
  void begin_write(uint32_t addr);

  uint32_t spi_speed_hz_{1000000};  // current effective speed
};

} // namespace ft800
} // namespace esphome
