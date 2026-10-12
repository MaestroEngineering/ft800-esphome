#include "ft800_widget.h"
#include "ft800_display.h"

namespace esphome {
namespace ft800 {

// ---------------------------------------------------------------------------
// FT800TextWidget
// ---------------------------------------------------------------------------

void FT800TextWidget::render(FT800Display &disp) {
  if (has_color_) disp.color(color_);
  disp.text(x_, y_, font_, options_, text_);
}

// ---------------------------------------------------------------------------
// FT800ButtonWidget
// ---------------------------------------------------------------------------

void FT800ButtonWidget::render(FT800Display &disp) {
  uint16_t opts = options_;
  if (pressed_) opts |= OPT_FLAT;  // pressed appearance
  disp.button(x_, y_, w_, h_, font_, opts, label_);
}

// ---------------------------------------------------------------------------
// FT800ToggleWidget
// ---------------------------------------------------------------------------

void FT800ToggleWidget::render(FT800Display &disp) {
  disp.toggle(x_, y_, w_, font_, 0, state_ ? 65535 : 0, labels_);
}

// ---------------------------------------------------------------------------
// FT800SliderWidget
// ---------------------------------------------------------------------------

void FT800SliderWidget::render(FT800Display &disp) {
  disp.slider(x_, y_, w_, h_, 0, value_, range_);
}

// ---------------------------------------------------------------------------
// FT800ProgressWidget
// ---------------------------------------------------------------------------

void FT800ProgressWidget::render(FT800Display &disp) {
  disp.progress(x_, y_, w_, h_, 0, value_, range_);
}

// ---------------------------------------------------------------------------
// FT800GaugeWidget
// ---------------------------------------------------------------------------

void FT800GaugeWidget::render(FT800Display &disp) {
  // Gauge centre is (x_, y_); r_ is the radius stored in w_/2
  disp.gauge(x_, y_, r_, 0, major_, minor_, value_, range_);
}

// ---------------------------------------------------------------------------
// FT800NumberWidget
// ---------------------------------------------------------------------------

void FT800NumberWidget::render(FT800Display &disp) {
  if (has_color_) disp.color(color_);
  disp.number(x_, y_, font_, options_, value_);
}

} // namespace ft800
} // namespace esphome
