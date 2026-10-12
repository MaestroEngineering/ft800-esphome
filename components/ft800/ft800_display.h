#pragma once
// FT800 display component for ESPHome.
// Provides the it.* drawing API inside a YAML lambda and drives the
// co-processor rendering cycle each update period.

#include <functional>
#include "esphome/core/component.h"
#include "esphome/core/color.h"
#include "ft800.h"
#include "ft800_widget.h"

namespace esphome {
namespace ft800 {

class FT800Display : public PollingComponent {
 public:
  explicit FT800Display(FT800Component *parent) : parent_(parent) {}

  // ---- ESPHome lifecycle --------------------------------------------------
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

  // ---- Configuration (set from Python codegen) ---------------------------
  void set_lambda(std::function<void(FT800Display &)> &&fn) { writer_ = std::move(fn); }
  void set_page(uint8_t page_id);

  // ---- Widget tree management --------------------------------------------
  void add_widget(FT800Widget *w);
  FT800Widget *get_widget_by_tag(uint8_t tag);
  FT800Widget *get_widget_by_name(const char *name);

  // ---- Dimensions (delegates to parent) ----------------------------------
  uint16_t width()  const { return parent_->width(); }
  uint16_t height() const { return parent_->height(); }

  // =========================================================================
  // Drawing API — the it.* interface available inside lambda:
  // All co-processor commands are documented in Programmer's Guide §4.
  // =========================================================================

  // ---- Color helpers ------------------------------------------------------
  // Set foreground / background colour for subsequent co-proc widgets
  void fg_color(Color c);
  void bg_color(Color c);
  void grad_color(Color c);

  // ---- Text & numbers -----------------------------------------------------
  // CMD_TEXT(x, y, font, options, string)  (Prog. Guide §4.43)
  void text(int16_t x, int16_t y, uint8_t font, uint16_t options, const char *str);
  // Convenience: centred text
  void text_centered(int16_t x, int16_t y, uint8_t font, const char *str);

  // CMD_NUMBER(x, y, font, options, n)  (Prog. Guide §4.29)
  void number(int16_t x, int16_t y, uint8_t font, uint16_t options, int32_t n);

  // ---- Widgets ------------------------------------------------------------
  // CMD_BUTTON(x, y, w, h, font, options, label)  (Prog. Guide §4.5)
  void button(int16_t x, int16_t y, int16_t w, int16_t h,
              uint8_t font, uint16_t options, const char *label);

  // CMD_TOGGLE(x, y, w, font, options, state, labels)  (Prog. Guide §4.44)
  // labels: "OFF\xFFON" — the 0xFF byte is the separator
  void toggle(int16_t x, int16_t y, int16_t w,
              uint8_t font, uint16_t options, uint16_t state, const char *labels);

  // CMD_SLIDER(x, y, w, h, options, val, range)  (Prog. Guide §4.38)
  void slider(int16_t x, int16_t y, int16_t w, int16_t h,
              uint16_t options, uint16_t val, uint16_t range);

  // CMD_PROGRESS(x, y, w, h, options, val, range)  (Prog. Guide §4.31)
  void progress(int16_t x, int16_t y, int16_t w, int16_t h,
                uint16_t options, uint16_t val, uint16_t range);

  // CMD_GAUGE(x, y, r, options, major, minor, val, range)  (Prog. Guide §4.11)
  void gauge(int16_t x, int16_t y, int16_t r,
             uint16_t options, uint16_t major, uint16_t minor,
             uint16_t val, uint16_t range);

  // CMD_DIAL(x, y, r, options, val)  (Prog. Guide §4.8)
  void dial(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t val);

  // CMD_CLOCK(x, y, r, options, h, m, s, ms)  (Prog. Guide §4.7)
  void clock(int16_t x, int16_t y, int16_t r,
             uint16_t options, uint16_t h, uint16_t m, uint16_t s, uint16_t ms);

  // CMD_KEYS(x, y, w, h, font, options, string)  (Prog. Guide §4.17)
  void keys(int16_t x, int16_t y, int16_t w, int16_t h,
            uint8_t font, uint16_t options, const char *key_chars);

  // CMD_SPINNER(x, y, style, scale)  (Prog. Guide §4.40)
  void spinner(int16_t x, int16_t y, uint16_t style, uint16_t scale);

  // CMD_GRADIENT(x0, y0, rgb0, x1, y1, rgb1)  (Prog. Guide §4.12)
  void gradient(int16_t x0, int16_t y0, uint32_t rgb0,
                int16_t x1, int16_t y1, uint32_t rgb1);

  // ---- Raw display list primitives ----------------------------------------
  // Emit a single raw DL word; use for things CMD_* doesn't cover
  void dl(uint32_t word);

  // Set TAG for following primitives (touch tag assignment)
  void tag(uint8_t t);

  // BEGIN / END primitives
  void begin(uint8_t primitive);   // PRIM_* constants from ft800_commands.h
  void end_prim();

  // Raw vertex helpers (coordinate in pixels; VERTEX2F uses 1/16 pixel)
  void vertex(int16_t x, int16_t y);           // VERTEX2F (16x sub-pixel)
  void vertex_px(uint16_t x, uint16_t y, uint8_t handle = 0, uint8_t cell = 0); // VERTEX2II

  // Rectangle shortcut using RECTS primitive
  void rect(int16_t x, int16_t y, int16_t w, int16_t h, Color c);

  // Line shortcut
  void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t width_px, Color c);

  // Circle (point primitive)
  void circle(int16_t cx, int16_t cy, int16_t radius, Color c);

  // ---- Color state helpers ------------------------------------------------
  void color(Color c);
  void color_rgb(uint8_t r, uint8_t g, uint8_t b);
  void clear_color(Color c);
  void clear_color_rgb(uint8_t r, uint8_t g, uint8_t b);
  void clear_screen();    // clear with current clear colour

  // ---- CMD_TRACK — register a region for slider/dial tracking ------------
  void track(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t tag_id);

 protected:
  FT800Component *parent_;
  std::function<void(FT800Display &)> writer_;
  std::vector<FT800Widget *> widgets_;
  uint8_t active_page_{0};

  // Render all visible widgets for the active page, then call the user lambda
  void render_frame_();
};

} // namespace ft800
} // namespace esphome
