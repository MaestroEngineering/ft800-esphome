#pragma once
// FT800 core driver for ESPHome.
// Implements the boot sequence, co-processor FIFO management, touch ISR,
// and rendering lifecycle.  All hardware addresses are cited from:
//   DS_FT800_Embedded_Video_Engine v1.4
//   FT800_Series_Programmer_Guide v2.1

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/preferences.h"
#include "ft800_hal.h"
#include "ft800_registers.h"
#include "ft800_commands.h"

namespace esphome {
namespace ft800 {

// DisplayProfile and PROFILE_WQVGA / PROFILE_QVGA are in ft800_registers.h
// (plain struct, no ESPHome types — safe for host-side test inclusion)

// ---------------------------------------------------------------------------
// Touch calibration: six s15.16 fixed-point transform coefficients
// Stored in NVS / ESPHome preferences after CMD_CALIBRATE  (DS_FT800 §4.3)
// ---------------------------------------------------------------------------
struct TouchCalibration {
  uint32_t a, b, c, d, e, f;  // REG_TOUCH_TRANSFORM_A through F
  bool valid;
};

// ---------------------------------------------------------------------------
// Touch event dispatched to registered sub-components
// ---------------------------------------------------------------------------
struct TouchEvent {
  uint8_t  tag;    // FT800 tag ID (0 = no touch)
  int16_t  x;      // calibrated x coordinate
  int16_t  y;      // calibrated y coordinate
  uint16_t track;  // REG_TRACKER value (for sliders/dials)
};

enum class TouchEventType { PRESS, RELEASE, HOLD, TRACK_CHANGE };

using TouchCallback = std::function<void(const TouchEvent &, TouchEventType)>;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
class FT800Display;

// ---------------------------------------------------------------------------
// FT800Component — the main ESPHome component
// ---------------------------------------------------------------------------
class FT800Component : public Component, public FT800HAL {
 public:
  // ---- Configuration setters (called from Python codegen) ----------------
  void set_pd_pin(GPIOPin *pd)    { pd_pin_ = pd; }
  void set_int_pin(GPIOPin *intr) { int_pin_ = intr; }
  void set_profile(const DisplayProfile &p) { profile_ = p; }
  void set_brightness(uint8_t pct);  // 0–100 %

  // ---- ESPHome lifecycle --------------------------------------------------
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  // ---- Co-processor FIFO API (used by FT800Display and widgets) ----------
  // All cmd_* methods write into a local RAM buffer.
  // flush() sends the buffer to the chip and triggers execution.

  void cmd(uint32_t word);               // append one 32-bit command
  void cmd_string(const char *s);        // append null-terminated string (4-byte padded)
  void cmd_buf(const void *data, size_t len); // append raw bytes (4-byte padded)
  bool flush();                          // send pending commands → REG_CMD_WRITE
  bool await_idle(uint32_t timeout_ms = 200); // wait for co-proc to finish

  // ---- Frame rendering entry point (called by FT800Display) --------------
  void begin_frame();    // CMD_DLSTART
  void end_frame();      // DISPLAY() + CMD_SWAP + flush

  // ---- Runtime display control -------------------------------------------
  void set_backlight(uint8_t duty);      // 0–128 direct REG_PWM_DUTY
  void sleep();
  void wake();

  // ---- Touch calibration -------------------------------------------------
  bool run_calibration();                // runs CMD_CALIBRATE, saves to NVS
  bool load_calibration();               // load from NVS, write to HW registers
  void clear_calibration();             // erase NVS entry

  // ---- Touch listener registration ---------------------------------------
  void add_touch_listener(TouchCallback cb) { touch_listeners_.push_back(cb); }

  // ---- Accessors used by display sub-component ---------------------------
  uint16_t width()  const { return profile_.hsize; }
  uint16_t height() const { return profile_.vsize; }
  bool is_failed()  const { return failed_; }

  // ---- Dirty flag — set when display content must be rebuilt -------------
  void mark_dirty() { dirty_ = true; }

 protected:
  // ---- Boot sequence  (Programmer's Guide §2.2 Code Snippet 1) -----------
  bool boot_sequence_();

  // ---- FIFO helpers -------------------------------------------------------
  uint16_t fifo_free_();                // bytes available in RAM_CMD FIFO
  void handle_copro_fault_();           // reset co-proc engine on fault

  // ---- ISR & touch processing --------------------------------------------
  static void IRAM_ATTR touch_isr_(FT800Component *self);
  void process_touch_();
  void dispatch_touch_event_(const TouchEvent &ev, TouchEventType type);

  // ---- Pins ---------------------------------------------------------------
  GPIOPin *pd_pin_  {nullptr};
  GPIOPin *int_pin_ {nullptr};

  // ---- Display profile / state -------------------------------------------
  DisplayProfile profile_{PROFILE_WQVGA};
  bool failed_{false};
  bool initialized_{false};
  bool sleeping_{false};
  bool dirty_{true};

  // ---- FIFO state ---------------------------------------------------------
  // Local staging buffer: hold commands until flush() is called.
  // Max one frame worth = typically 1–2 kB; 3 kB headroom.
  static constexpr size_t FIFO_STAGE_SIZE = 3072;
  std::array<uint8_t, FIFO_STAGE_SIZE> fifo_stage_;
  size_t fifo_stage_len_{0};
  uint16_t fifo_wr_{0};  // mirrors REG_CMD_WRITE (local copy)

  // ---- Touch state --------------------------------------------------------
  volatile bool touch_pending_{false};
  uint8_t  last_tag_{0};
  int16_t  last_x_{0};
  int16_t  last_y_{0};
  uint16_t last_track_{0};
  uint32_t press_start_ms_{0};
  std::vector<TouchCallback> touch_listeners_;

  // ---- Preferences (NVS) for calibration ----------------------------------
  ESPPreferenceObject cal_pref_;
};

} // namespace ft800
} // namespace esphome
