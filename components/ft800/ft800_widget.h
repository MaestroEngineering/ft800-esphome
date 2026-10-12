#pragma once
// FT800 widget descriptors.
// A widget encapsulates a named, tagged UI component: its geometry, state,
// and how to emit the co-processor commands that render it.

#include <cstdint>
#include <cstring>
#include "esphome/core/color.h"
#include "ft800_commands.h"

namespace esphome {
namespace ft800 {

class FT800Display;  // forward declaration

// ---------------------------------------------------------------------------
// Widget base class
// ---------------------------------------------------------------------------
class FT800Widget {
 public:
  FT800Widget(const char *name, uint8_t tag_id, uint8_t page_id,
              int16_t x, int16_t y, int16_t w, int16_t h)
      : name_(name), tag_id_(tag_id), page_id_(page_id),
        x_(x), y_(y), w_(w), h_(h) {}

  virtual ~FT800Widget() = default;

  // ---- Metadata -----------------------------------------------------------
  const char *name()    const { return name_; }
  uint8_t  tag_id()     const { return tag_id_; }
  uint8_t  page_id()    const { return page_id_; }
  int16_t  x()          const { return x_; }
  int16_t  y()          const { return y_; }
  int16_t  width()      const { return w_; }
  int16_t  height()     const { return h_; }
  bool     is_visible() const { return visible_; }
  bool     is_trackable() const { return trackable_; }

  // ---- State setters (mark parent dirty on change) -----------------------
  void set_visible(bool v)       { if (visible_ != v) { visible_ = v; dirty_ = true; } }
  void set_position(int16_t x, int16_t y) { x_ = x; y_ = y; dirty_ = true; }

  // ---- Rendering ----------------------------------------------------------
  virtual void render(FT800Display &disp) = 0;

  bool is_dirty() const { return dirty_; }
  void clear_dirty()     { dirty_ = false; }

 protected:
  const char *name_;
  uint8_t     tag_id_;
  uint8_t     page_id_;
  int16_t     x_, y_, w_, h_;
  bool        visible_{true};
  bool        dirty_{true};
  bool        trackable_{false};
};

// ---------------------------------------------------------------------------
// Text widget
// ---------------------------------------------------------------------------
class FT800TextWidget : public FT800Widget {
 public:
  FT800TextWidget(const char *name, uint8_t tag_id, uint8_t page_id,
                  int16_t x, int16_t y, uint8_t font, uint16_t options)
      : FT800Widget(name, tag_id, page_id, x, y, 0, 0),
        font_(font), options_(options) {
    text_[0] = '\0';
  }

  void set_text(const char *t) {
    strncpy(text_, t, sizeof(text_) - 1);
    text_[sizeof(text_) - 1] = '\0';
    dirty_ = true;
  }

  void set_color(Color c) { color_ = c; has_color_ = true; dirty_ = true; }

  void render(FT800Display &disp) override;

 protected:
  char    text_[128]{};
  uint8_t font_;
  uint16_t options_;
  Color   color_{255, 255, 255, 0};
  bool    has_color_{false};
};

// ---------------------------------------------------------------------------
// Button widget
// ---------------------------------------------------------------------------
class FT800ButtonWidget : public FT800Widget {
 public:
  FT800ButtonWidget(const char *name, uint8_t tag_id, uint8_t page_id,
                    int16_t x, int16_t y, int16_t w, int16_t h,
                    uint8_t font, uint16_t options)
      : FT800Widget(name, tag_id, page_id, x, y, w, h),
        font_(font), options_(options) {
    label_[0] = '\0';
  }

  void set_label(const char *l) {
    strncpy(label_, l, sizeof(label_) - 1);
    label_[sizeof(label_) - 1] = '\0';
    dirty_ = true;
  }

  void set_pressed(bool p) { pressed_ = p; dirty_ = true; }

  void render(FT800Display &disp) override;

 protected:
  char    label_[64]{};
  uint8_t font_;
  uint16_t options_;
  bool    pressed_{false};
};

// ---------------------------------------------------------------------------
// Toggle widget
// ---------------------------------------------------------------------------
class FT800ToggleWidget : public FT800Widget {
 public:
  FT800ToggleWidget(const char *name, uint8_t tag_id, uint8_t page_id,
                    int16_t x, int16_t y, int16_t w,
                    uint8_t font, const char *off_label, const char *on_label)
      : FT800Widget(name, tag_id, page_id, x, y, w, 0), font_(font) {
    trackable_ = true;
    snprintf(labels_, sizeof(labels_), "%s\xFF%s", off_label, on_label);
  }

  void set_state(bool on) { state_ = on; dirty_ = true; }
  bool state() const { return state_; }

  void render(FT800Display &disp) override;

 protected:
  char    labels_[64]{};
  uint8_t font_;
  bool    state_{false};
};

// ---------------------------------------------------------------------------
// Slider widget
// ---------------------------------------------------------------------------
class FT800SliderWidget : public FT800Widget {
 public:
  FT800SliderWidget(const char *name, uint8_t tag_id, uint8_t page_id,
                    int16_t x, int16_t y, int16_t w, int16_t h,
                    uint16_t range)
      : FT800Widget(name, tag_id, page_id, x, y, w, h), range_(range) {
    trackable_ = true;
  }

  void set_value(uint16_t v) { value_ = v; dirty_ = true; }
  uint16_t value() const { return value_; }
  uint16_t range() const { return range_; }

  void render(FT800Display &disp) override;

 protected:
  uint16_t value_{0};
  uint16_t range_;
};

// ---------------------------------------------------------------------------
// Progress widget
// ---------------------------------------------------------------------------
class FT800ProgressWidget : public FT800Widget {
 public:
  FT800ProgressWidget(const char *name, uint8_t tag_id, uint8_t page_id,
                      int16_t x, int16_t y, int16_t w, int16_t h,
                      uint16_t range)
      : FT800Widget(name, tag_id, page_id, x, y, w, h), range_(range) {}

  void set_value(uint16_t v) { value_ = v; dirty_ = true; }

  void render(FT800Display &disp) override;

 protected:
  uint16_t value_{0};
  uint16_t range_;
};

// ---------------------------------------------------------------------------
// Gauge widget
// ---------------------------------------------------------------------------
class FT800GaugeWidget : public FT800Widget {
 public:
  FT800GaugeWidget(const char *name, uint8_t tag_id, uint8_t page_id,
                   int16_t cx, int16_t cy, int16_t r,
                   uint16_t major, uint16_t minor, uint16_t range)
      : FT800Widget(name, tag_id, page_id, cx, cy, r * 2, r * 2),
        r_(r), major_(major), minor_(minor), range_(range) {}

  void set_value(uint16_t v) { value_ = v; dirty_ = true; }

  void render(FT800Display &disp) override;

 protected:
  int16_t  r_;
  uint16_t major_, minor_;
  uint16_t value_{0};
  uint16_t range_;
};

// ---------------------------------------------------------------------------
// Number widget (integer display)
// ---------------------------------------------------------------------------
class FT800NumberWidget : public FT800Widget {
 public:
  FT800NumberWidget(const char *name, uint8_t tag_id, uint8_t page_id,
                    int16_t x, int16_t y, uint8_t font, uint16_t options)
      : FT800Widget(name, tag_id, page_id, x, y, 0, 0),
        font_(font), options_(options) {}

  void set_value(int32_t v) { value_ = v; dirty_ = true; }
  void set_color(Color c)   { color_ = c; has_color_ = true; dirty_ = true; }

  void render(FT800Display &disp) override;

 protected:
  int32_t  value_{0};
  uint8_t  font_;
  uint16_t options_;
  Color    color_{255, 255, 255, 0};
  bool     has_color_{false};
};

} // namespace ft800
} // namespace esphome
