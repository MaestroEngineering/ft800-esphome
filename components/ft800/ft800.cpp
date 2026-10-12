#include "ft800.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ft800 {

// Log tag — named FTAG to avoid conflict with TAG() inline function in ft800_commands.h
static const char *const FTAG = "ft800";

// ---------------------------------------------------------------------------
// ESPHome lifecycle
// ---------------------------------------------------------------------------

void FT800Component::setup() {
  ESP_LOGCONFIG(FTAG, "Setting up FT800...");

  cal_pref_ = global_preferences->make_preference<TouchCalibration>(
      fnv1_hash("ft800_cal"), true);

  // PD_N low for ≥ 20 ms performs a hard reset  (DS_FT800 §4.1.1)
  if (pd_pin_ != nullptr) {
    pd_pin_->setup();
    pd_pin_->digital_write(false);
    delay(20);
    pd_pin_->digital_write(true);
    delay(20);
  }

  // SPI must be ≤ 11 MHz during boot  (Programmer's Guide §2.2)
  this->spi_setup();

  if (!boot_sequence_()) {
    ESP_LOGE(FTAG, "FT800 boot failed — check wiring");
    this->mark_failed();
    failed_ = true;
    return;
  }

  if (int_pin_ != nullptr) {
    int_pin_->setup();
    int_pin_->attach_interrupt(
        reinterpret_cast<void (*)(void *)>(FT800Component::touch_isr_),
        this,
        gpio::INTERRUPT_FALLING_EDGE);
    // Enable INT_N on touch/tag events  (DS_FT800 §4.4)
    wr8(REG_INT_EN,   0x01);
    wr8(REG_INT_MASK, INT_TOUCH | INT_TAG);
  }

  if (!load_calibration()) {
    ESP_LOGW(FTAG, "No touch calibration stored; using default transform");
  }

  initialized_ = true;
  dirty_ = true;
  ESP_LOGCONFIG(FTAG, "FT800 ready (%u x %u)", profile_.hsize, profile_.vsize);
}

void FT800Component::loop() {
  if (failed_ || !initialized_) return;
  if (touch_pending_) {
    process_touch_();
  }
}

void FT800Component::dump_config() {
  ESP_LOGCONFIG(FTAG, "FT800:");
  LOG_PIN("  PD_N pin:  ", pd_pin_);
  LOG_PIN("  INT_N pin: ", int_pin_);
  ESP_LOGCONFIG(FTAG, "  Panel:     %u x %u px", profile_.hsize, profile_.vsize);
  ESP_LOGCONFIG(FTAG, "  PCLK:      48MHz/%u = %uMHz", profile_.pclk, 48 / profile_.pclk);
  if (failed_) {
    ESP_LOGE(FTAG, "  Status:    FAILED");
  }
}

// ---------------------------------------------------------------------------
// Boot sequence  (Programmer's Guide §2.2 Code Snippet 1, DS_FT800 §4.1.2)
// ---------------------------------------------------------------------------

bool FT800Component::boot_sequence_() {
  // Send CLKEXT → select external 12 MHz crystal  (Prog. Guide §2.2)
  host_command(HOSTCMD_CLKEXT);
  delayMicroseconds(100);

  // Send ACTIVE → start oscillator + PLL  (Prog. Guide §2.2)
  host_command(HOSTCMD_ACTIVE);
  delay(20);  // PLL lock time ≥ 20 ms

  // Verify REG_ID == 0x7C (up to 100 ms)  (DS_FT800 Table 5-2)
  uint8_t chip_id = 0;
  for (int attempt = 0; attempt < 50; attempt++) {
    chip_id = rd8(REG_ID);
    if (chip_id == FT800_REG_ID_VALUE) break;
    delay(2);
  }
  if (chip_id != FT800_REG_ID_VALUE) {
    ESP_LOGE(FTAG, "REG_ID=0x%02X expected 0x7C — SPI wiring error", chip_id);
    return false;
  }
  ESP_LOGD(FTAG, "REG_ID OK (0x7C)");

  // Read ROM_CHIPID for chip variant  (DS_FT800 §5 Table 5-1)
  uint8_t id_bytes[4];
  rd_mem(ROM_CHIPID, id_bytes, 4);
  ESP_LOGD(FTAG, "ROM_CHIPID: %02X %02X %02X %02X",
           id_bytes[0], id_bytes[1], id_bytes[2], id_bytes[3]);
  if (id_bytes[0] != 0x00 || id_bytes[1] != 0x08) {
    ESP_LOGW(FTAG, "Unexpected chip ID 0x%02X%02X (expected FT800=0x0008)",
             id_bytes[1], id_bytes[0]);
    // Non-fatal: FT801 reads "0x0008 0x0108" which is still usable
  }

  // Release CPU reset on all engines  (DS_FT800 Table 5-2)
  wr8(REG_CPURESET, CPURESET_GFX | CPURESET_AUDIO | CPURESET_TOUCH);
  delayMicroseconds(50);
  wr8(REG_CPURESET, 0x00);
  delay(5);

  // Load display timing (except REG_PCLK which enables video output)
  wr16(REG_HCYCLE,   profile_.hcycle);
  wr16(REG_HOFFSET,  profile_.hoffset);
  wr16(REG_HSIZE,    profile_.hsize);
  wr16(REG_HSYNC0,   profile_.hsync0);
  wr16(REG_HSYNC1,   profile_.hsync1);
  wr16(REG_VCYCLE,   profile_.vcycle);
  wr16(REG_VOFFSET,  profile_.voffset);
  wr16(REG_VSIZE,    profile_.vsize);
  wr16(REG_VSYNC0,   profile_.vsync0);
  wr16(REG_VSYNC1,   profile_.vsync1);
  wr8 (REG_SWIZZLE,  profile_.swizzle);
  wr8 (REG_PCLK_POL, profile_.pclk_pol);
  wr8 (REG_CSPREAD,  profile_.cspread);

  // Write minimal display list before enabling PCLK  (Prog. Guide §2.2)
  wr32(RAM_DL + 0, CLEAR_COLOR_RGB(0, 0, 0));
  wr32(RAM_DL + 4, CLEAR(1, 1, 1));
  wr32(RAM_DL + 8, DISPLAY());
  wr8 (REG_DLSWAP, DLSWAP_FRAME);

  // Enable DISP output via FT800 GPIO bit 7  (Prog. Guide §2.2, DS_FT800 §4.2.4)
  wr8(REG_GPIO_DIR, rd8(REG_GPIO_DIR) | 0x80);
  wr8(REG_GPIO,     rd8(REG_GPIO)     | 0x80);

  // Write REG_PCLK — video output starts now  (Prog. Guide §2.2)
  wr8(REG_PCLK, profile_.pclk);

  // Configure touch engine  (DS_FT800 §4.3)
  wr8(REG_TOUCH_MODE,     TOUCH_MODE_CONTINUOUS);
  wr8(REG_TOUCH_ADC_MODE, 0x01);  // differential (accurate)

  // Default backlight 50 %  (DS_FT800 §4.2.4)
  wr16(REG_PWM_HZ,   250);
  wr8 (REG_PWM_DUTY, 64);

  // Sync local FIFO write pointer  (Prog. Guide §2.4)
  fifo_wr_ = rd16(REG_CMD_WRITE);

  return true;
}

// ---------------------------------------------------------------------------
// Co-processor FIFO  (Programmer's Guide §2.4)
// ---------------------------------------------------------------------------

void FT800Component::cmd(uint32_t word) {
  if (fifo_stage_len_ + 4 > FIFO_STAGE_SIZE) {
    if (!flush()) {
      ESP_LOGW(FTAG, "FIFO flush failed during cmd()");
      return;
    }
  }
  fifo_stage_[fifo_stage_len_++] = (uint8_t)(word         & 0xFF);
  fifo_stage_[fifo_stage_len_++] = (uint8_t)((word >>  8) & 0xFF);
  fifo_stage_[fifo_stage_len_++] = (uint8_t)((word >> 16) & 0xFF);
  fifo_stage_[fifo_stage_len_++] = (uint8_t)((word >> 24) & 0xFF);
}

void FT800Component::cmd_string(const char *s) {
  size_t len = strlen(s) + 1;
  cmd_buf(s, len);
}

void FT800Component::cmd_buf(const void *data, size_t len) {
  const uint8_t *p = static_cast<const uint8_t *>(data);
  size_t padded = (len + 3) & ~3u;
  for (size_t i = 0; i < padded; i++) {
    if (fifo_stage_len_ >= FIFO_STAGE_SIZE) {
      if (!flush()) return;
    }
    fifo_stage_[fifo_stage_len_++] = (i < len) ? p[i] : 0x00;
  }
}

bool FT800Component::flush() {
  if (fifo_stage_len_ == 0) return true;

  size_t remaining = fifo_stage_len_;
  size_t offset    = 0;

  while (remaining > 0) {
    uint32_t deadline = millis() + 200;
    while (fifo_free_() < 4) {
      if (millis() > deadline) {
        ESP_LOGE(FTAG, "FIFO wait timeout");
        handle_copro_fault_();
        fifo_stage_len_ = 0;
        return false;
      }
      yield();
    }

    uint16_t free_bytes = fifo_free_();
    size_t   chunk      = std::min((size_t)free_bytes, remaining);
    uint16_t hw_addr    = fifo_wr_ & (RAM_CMD_SIZE - 1);
    uint16_t to_end     = (uint16_t)(RAM_CMD_SIZE - hw_addr);

    if (chunk <= to_end) {
      wr_mem(RAM_CMD + hw_addr, fifo_stage_.data() + offset, chunk);
    } else {
      wr_mem(RAM_CMD + hw_addr,  fifo_stage_.data() + offset,           to_end);
      wr_mem(RAM_CMD + 0,        fifo_stage_.data() + offset + to_end,  chunk - to_end);
    }

    fifo_wr_ = (uint16_t)((fifo_wr_ + chunk) & (RAM_CMD_SIZE - 1));
    wr16(REG_CMD_WRITE, fifo_wr_);

    offset    += chunk;
    remaining -= chunk;
  }

  fifo_stage_len_ = 0;
  return true;
}

bool FT800Component::await_idle(uint32_t timeout_ms) {
  uint32_t deadline = millis() + timeout_ms;
  while (true) {
    uint16_t rp = rd16(REG_CMD_READ);
    if (rp == FIFO_FAULT_VALUE) {
      ESP_LOGE(FTAG, "Co-processor fault (REG_CMD_READ=0xFFF)");
      handle_copro_fault_();
      return false;
    }
    if (rp == fifo_wr_) return true;
    if (millis() > deadline) {
      ESP_LOGE(FTAG, "Co-processor idle timeout (rp=%u wr=%u)", rp, fifo_wr_);
      handle_copro_fault_();
      return false;
    }
    yield();
  }
}

uint16_t FT800Component::fifo_free_() {
  uint16_t rp = rd16(REG_CMD_READ);
  return (uint16_t)(RAM_CMD_SIZE - 4 - ((fifo_wr_ - rp) & (RAM_CMD_SIZE - 1)));
}

void FT800Component::handle_copro_fault_() {
  ESP_LOGW(FTAG, "Resetting co-processor engine");
  wr8(REG_CPURESET, CPURESET_GFX);
  delayMicroseconds(100);
  wr8(REG_CPURESET, 0x00);
  delay(5);
  wr16(REG_CMD_READ,  0x00);
  wr16(REG_CMD_WRITE, 0x00);
  fifo_wr_        = 0;
  fifo_stage_len_ = 0;
  dirty_ = true;
}

// ---------------------------------------------------------------------------
// Frame entry / exit
// ---------------------------------------------------------------------------

void FT800Component::begin_frame() {
  if (!await_idle()) return;
  cmd(CMD_DLSTART);
}

void FT800Component::end_frame() {
  cmd(DISPLAY());
  cmd(CMD_SWAP);
  flush();
}

// ---------------------------------------------------------------------------
// Display control
// ---------------------------------------------------------------------------

void FT800Component::set_brightness(uint8_t pct) {
  set_backlight((uint8_t)((pct * 128UL) / 100));
}

void FT800Component::set_backlight(uint8_t duty) {
  wr8(REG_PWM_DUTY, duty);
}

void FT800Component::sleep() {
  if (sleeping_) return;
  wr8(REG_PCLK, 0);
  host_command(HOSTCMD_SLEEP);
  sleeping_ = true;
  ESP_LOGD(FTAG, "FT800 sleeping");
}

void FT800Component::wake() {
  if (!sleeping_) return;
  host_command(HOSTCMD_ACTIVE);
  delay(20);
  wr8(REG_PCLK, profile_.pclk);
  sleeping_ = false;
  dirty_ = true;
  ESP_LOGD(FTAG, "FT800 awake");
}

// ---------------------------------------------------------------------------
// Touch calibration  (Programmer's Guide CMD_CALIBRATE §3)
// ---------------------------------------------------------------------------

bool FT800Component::run_calibration() {
  ESP_LOGI(FTAG, "Running touch calibration — follow on-screen instructions");
  begin_frame();
  cmd(CLEAR_COLOR_RGB(64, 64, 64));
  cmd(CLEAR(1, 1, 1));
  cmd(CMD_CALIBRATE);
  cmd(0x00000000);
  end_frame();
  if (!await_idle(30000)) {
    ESP_LOGE(FTAG, "Calibration timed out");
    return false;
  }

  TouchCalibration cal;
  cal.a = rd32(REG_TOUCH_TRANSFORM_A);
  cal.b = rd32(REG_TOUCH_TRANSFORM_B);
  cal.c = rd32(REG_TOUCH_TRANSFORM_C);
  cal.d = rd32(REG_TOUCH_TRANSFORM_D);
  cal.e = rd32(REG_TOUCH_TRANSFORM_E);
  cal.f = rd32(REG_TOUCH_TRANSFORM_F);
  cal.valid = true;
  cal_pref_.save(&cal);
  ESP_LOGI(FTAG, "Calibration saved: A=%08X B=%08X C=%08X D=%08X E=%08X F=%08X",
           cal.a, cal.b, cal.c, cal.d, cal.e, cal.f);
  return true;
}

bool FT800Component::load_calibration() {
  TouchCalibration cal{};
  if (!cal_pref_.load(&cal) || !cal.valid) return false;
  wr32(REG_TOUCH_TRANSFORM_A, cal.a);
  wr32(REG_TOUCH_TRANSFORM_B, cal.b);
  wr32(REG_TOUCH_TRANSFORM_C, cal.c);
  wr32(REG_TOUCH_TRANSFORM_D, cal.d);
  wr32(REG_TOUCH_TRANSFORM_E, cal.e);
  wr32(REG_TOUCH_TRANSFORM_F, cal.f);
  ESP_LOGD(FTAG, "Touch calibration loaded from NVS");
  return true;
}

void FT800Component::clear_calibration() {
  TouchCalibration cal{};
  cal.valid = false;
  cal_pref_.save(&cal);
}

// ---------------------------------------------------------------------------
// Touch ISR and dispatch
// ---------------------------------------------------------------------------

void IRAM_ATTR FT800Component::touch_isr_(FT800Component *self) {
  self->touch_pending_ = true;
}

void FT800Component::process_touch_() {
  touch_pending_ = false;
  (void)rd8(REG_INT_FLAGS);  // clear INT_FLAGS by reading  (DS_FT800 §4.4)

  uint8_t  tag = rd8(REG_TOUCH_TAG);
  uint32_t xy  = rd32(REG_TOUCH_SCREEN_XY);
  int16_t  x   = (int16_t)((xy >> 16) & 0xFFFF);
  int16_t  y   = (int16_t)( xy        & 0xFFFF);

  // REG_TRACKER: track_value[31:16] | tag[7:0]  (DS_FT800 Table 5-2)
  uint32_t tracker  = rd32(REG_TRACKER);
  uint16_t track_val = (uint16_t)(tracker >> 16);

  TouchEvent ev{tag, x, y, track_val};
  uint32_t now = millis();

  if (last_tag_ == 0 && tag != 0) {
    press_start_ms_ = now;
    dispatch_touch_event_(ev, TouchEventType::PRESS);
  } else if (last_tag_ != 0 && tag == 0) {
    TouchEvent rel_ev{last_tag_, last_x_, last_y_, last_track_};
    dispatch_touch_event_(rel_ev, TouchEventType::RELEASE);
  } else if (tag != 0) {
    if (track_val != last_track_) {
      dispatch_touch_event_(ev, TouchEventType::TRACK_CHANGE);
    }
    if (tag == last_tag_ && (now - press_start_ms_) > 500) {
      dispatch_touch_event_(ev, TouchEventType::HOLD);
      press_start_ms_ = now;
    }
  }

  last_tag_   = tag;
  last_x_     = x;
  last_y_     = y;
  last_track_ = track_val;
}

void FT800Component::dispatch_touch_event_(const TouchEvent &ev, TouchEventType type) {
  for (auto &cb : touch_listeners_) {
    cb(ev, type);
  }
}

} // namespace ft800
} // namespace esphome
