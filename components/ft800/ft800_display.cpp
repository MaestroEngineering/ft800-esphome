#include "ft800_display.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace ft800 {

static const char *const TAG_DISP = "ft800.display";

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void FT800Display::setup() {
  ESP_LOGCONFIG(TAG_DISP, "FT800Display setup");
  // Parent component must already be initialized before display setup
}

void FT800Display::update() {
  if (parent_->is_failed()) return;
  render_frame_();
}

void FT800Display::dump_config() {
  ESP_LOGCONFIG(TAG_DISP, "FT800Display:");
  ESP_LOGCONFIG(TAG_DISP, "  Active page: %u", active_page_);
  ESP_LOGCONFIG(TAG_DISP, "  Widgets:     %zu", widgets_.size());
}

// ---------------------------------------------------------------------------
// Widget management
// ---------------------------------------------------------------------------

void FT800Display::add_widget(FT800Widget *w) {
  widgets_.push_back(w);
}

FT800Widget *FT800Display::get_widget_by_tag(uint8_t t) {
  for (auto *w : widgets_) {
    if (w->tag_id() == t) return w;
  }
  return nullptr;
}

FT800Widget *FT800Display::get_widget_by_name(const char *name) {
  for (auto *w : widgets_) {
    if (strcmp(w->name(), name) == 0) return w;
  }
  return nullptr;
}

void FT800Display::set_page(uint8_t page_id) {
  if (active_page_ != page_id) {
    active_page_ = page_id;
    parent_->mark_dirty();
  }
}

// ---------------------------------------------------------------------------
// Frame rendering
// ---------------------------------------------------------------------------

void FT800Display::render_frame_() {
  parent_->begin_frame();

  // Clear screen with black background
  parent_->cmd(CLEAR_COLOR_RGB(0, 0, 0));
  parent_->cmd(CLEAR(1, 1, 1));

  // Render all visible widgets on the active page (sorted by registration order)
  for (auto *w : widgets_) {
    if (!w->is_visible() || w->page_id() != active_page_) continue;
    parent_->cmd(TAG(w->tag_id()));
    w->render(*this);
  }

  // Disable further tag updates after widget area  (Prog. Guide §4.42)
  parent_->cmd(TAG(0));

  // Call user lambda for custom drawing on top of widgets
  if (writer_) writer_(*this);

  // Register CMD_TRACK for any interactive widget on this page
  for (auto *w : widgets_) {
    if (!w->is_visible() || w->page_id() != active_page_) continue;
    if (w->is_trackable()) {
      track(w->x(), w->y(), w->width(), w->height(), w->tag_id());
    }
  }

  parent_->end_frame();
}

// ---------------------------------------------------------------------------
// Color helpers — CMD_FGCOLOR / CMD_BGCOLOR  (Programmer's Guide §4.10, 4.9)
// ---------------------------------------------------------------------------

static inline uint32_t color_to_rgb24(Color c) {
  return ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b;
}

void FT800Display::fg_color(Color c) {
  parent_->cmd(CMD_FGCOLOR);
  parent_->cmd(color_to_rgb24(c));
}

void FT800Display::bg_color(Color c) {
  parent_->cmd(CMD_BGCOLOR);
  parent_->cmd(color_to_rgb24(c));
}

void FT800Display::grad_color(Color c) {
  parent_->cmd(CMD_GRADCOLOR);
  parent_->cmd(color_to_rgb24(c));
}

// ---------------------------------------------------------------------------
// Text & numbers  (Programmer's Guide §4.43, §4.29)
// ---------------------------------------------------------------------------

void FT800Display::text(int16_t x, int16_t y, uint8_t font, uint16_t options, const char *str) {
  parent_->cmd(CMD_TEXT);
  parent_->cmd(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
  parent_->cmd(((uint32_t)options << 16) | font);
  parent_->cmd_string(str);
}

void FT800Display::text_centered(int16_t x, int16_t y, uint8_t font, const char *str) {
  text(x, y, font, OPT_CENTER, str);
}

void FT800Display::number(int16_t x, int16_t y, uint8_t font, uint16_t options, int32_t n) {
  parent_->cmd(CMD_NUMBER);
  parent_->cmd(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
  parent_->cmd(((uint32_t)options << 16) | font);
  parent_->cmd((uint32_t)n);
}

// ---------------------------------------------------------------------------
// Widget commands  (Programmer's Guide references in DESIGN.md §5)
// ---------------------------------------------------------------------------

void FT800Display::button(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint8_t font, uint16_t options, const char *label) {
  parent_->cmd(CMD_BUTTON);
  parent_->cmd(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
  parent_->cmd(((uint32_t)(uint16_t)h << 16) | (uint16_t)w);
  parent_->cmd(((uint32_t)options << 16) | font);
  parent_->cmd_string(label);
}

void FT800Display::toggle(int16_t x, int16_t y, int16_t w,
                           uint8_t font, uint16_t options, uint16_t state, const char *labels) {
  parent_->cmd(CMD_TOGGLE);
  parent_->cmd(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
  parent_->cmd(((uint32_t)font << 16) | (uint16_t)w);
  parent_->cmd(((uint32_t)state << 16) | options);
  parent_->cmd_string(labels);
}

void FT800Display::slider(int16_t x, int16_t y, int16_t w, int16_t h,
                           uint16_t options, uint16_t val, uint16_t range) {
  parent_->cmd(CMD_SLIDER);
  parent_->cmd(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
  parent_->cmd(((uint32_t)(uint16_t)h << 16) | (uint16_t)w);
  parent_->cmd(((uint32_t)val << 16) | options);
  parent_->cmd((uint32_t)range);
}

void FT800Display::progress(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint16_t options, uint16_t val, uint16_t range) {
  parent_->cmd(CMD_PROGRESS);
  parent_->cmd(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
  parent_->cmd(((uint32_t)(uint16_t)h << 16) | (uint16_t)w);
  parent_->cmd(((uint32_t)val << 16) | options);
  parent_->cmd((uint32_t)range);
}

void FT800Display::gauge(int16_t x, int16_t y, int16_t r,
                          uint16_t options, uint16_t major, uint16_t minor,
                          uint16_t val, uint16_t range) {
  parent_->cmd(CMD_GAUGE);
  parent_->cmd(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
  parent_->cmd(((uint32_t)options << 16) | (uint16_t)r);
  parent_->cmd(((uint32_t)minor << 16) | major);
  parent_->cmd(((uint32_t)range << 16) | val);
}

void FT800Display::dial(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t val) {
  parent_->cmd(CMD_DIAL);
  parent_->cmd(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
  parent_->cmd(((uint32_t)options << 16) | (uint16_t)r);
  parent_->cmd((uint32_t)val);
}

void FT800Display::clock(int16_t x, int16_t y, int16_t r,
                          uint16_t options, uint16_t h, uint16_t m, uint16_t s, uint16_t ms) {
  parent_->cmd(CMD_CLOCK);
  parent_->cmd(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
  parent_->cmd(((uint32_t)options << 16) | (uint16_t)r);
  parent_->cmd(((uint32_t)m << 16) | h);
  parent_->cmd(((uint32_t)ms << 16) | s);
}

void FT800Display::keys(int16_t x, int16_t y, int16_t w, int16_t h,
                         uint8_t font, uint16_t options, const char *key_chars) {
  parent_->cmd(CMD_KEYS);
  parent_->cmd(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
  parent_->cmd(((uint32_t)(uint16_t)h << 16) | (uint16_t)w);
  parent_->cmd(((uint32_t)options << 16) | font);
  parent_->cmd_string(key_chars);
}

void FT800Display::spinner(int16_t x, int16_t y, uint16_t style, uint16_t scale) {
  parent_->cmd(CMD_SPINNER);
  parent_->cmd(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
  parent_->cmd(((uint32_t)scale << 16) | style);
}

void FT800Display::gradient(int16_t x0, int16_t y0, uint32_t rgb0,
                             int16_t x1, int16_t y1, uint32_t rgb1) {
  parent_->cmd(CMD_GRADIENT);
  parent_->cmd(((uint32_t)(uint16_t)y0 << 16) | (uint16_t)x0);
  parent_->cmd(rgb0 & 0x00FFFFFFUL);
  parent_->cmd(((uint32_t)(uint16_t)y1 << 16) | (uint16_t)x1);
  parent_->cmd(rgb1 & 0x00FFFFFFUL);
}

// ---------------------------------------------------------------------------
// Raw DL primitives
// ---------------------------------------------------------------------------

void FT800Display::dl(uint32_t word) {
  parent_->cmd(word);
}

void FT800Display::tag(uint8_t t) {
  parent_->cmd(TAG(t));
}

void FT800Display::begin(uint8_t primitive) {
  parent_->cmd(BEGIN(primitive));
}

void FT800Display::end_prim() {
  parent_->cmd(END());
}

void FT800Display::vertex(int16_t x, int16_t y) {
  // VERTEX2F uses 1/16 pixel units  (Programmer's Guide §4.44)
  parent_->cmd(VERTEX2F(x * 16, y * 16));
}

void FT800Display::vertex_px(uint16_t x, uint16_t y, uint8_t handle, uint8_t cell) {
  parent_->cmd(VERTEX2II(x, y, handle, cell));
}

// ---------------------------------------------------------------------------
// Convenience shape helpers
// ---------------------------------------------------------------------------

void FT800Display::color(Color c) {
  parent_->cmd(COLOR_RGB(c.r, c.g, c.b));
}

void FT800Display::color_rgb(uint8_t r, uint8_t g, uint8_t b) {
  parent_->cmd(COLOR_RGB(r, g, b));
}

void FT800Display::clear_color(Color c) {
  parent_->cmd(CLEAR_COLOR_RGB(c.r, c.g, c.b));
}

void FT800Display::clear_color_rgb(uint8_t r, uint8_t g, uint8_t b) {
  parent_->cmd(CLEAR_COLOR_RGB(r, g, b));
}

void FT800Display::clear_screen() {
  parent_->cmd(CLEAR(1, 1, 1));
}

void FT800Display::rect(int16_t x, int16_t y, int16_t w, int16_t h, Color c) {
  parent_->cmd(COLOR_RGB(c.r, c.g, c.b));
  parent_->cmd(BEGIN(PRIM_RECTS));
  parent_->cmd(VERTEX2F(x * 16,       y * 16));
  parent_->cmd(VERTEX2F((x + w) * 16, (y + h) * 16));
  parent_->cmd(END());
}

void FT800Display::line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t width_px, Color c) {
  parent_->cmd(COLOR_RGB(c.r, c.g, c.b));
  parent_->cmd(LINE_WIDTH(width_px * 16));  // LINE_WIDTH in 1/16 px units
  parent_->cmd(BEGIN(PRIM_LINES));
  parent_->cmd(VERTEX2F(x0 * 16, y0 * 16));
  parent_->cmd(VERTEX2F(x1 * 16, y1 * 16));
  parent_->cmd(END());
}

void FT800Display::circle(int16_t cx, int16_t cy, int16_t radius, Color c) {
  parent_->cmd(COLOR_RGB(c.r, c.g, c.b));
  parent_->cmd(POINT_SIZE(radius * 16));  // POINT_SIZE in 1/16 px units
  parent_->cmd(BEGIN(PRIM_POINTS));
  parent_->cmd(VERTEX2F(cx * 16, cy * 16));
  parent_->cmd(END());
}

void FT800Display::track(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t tag_id) {
  // CMD_TRACK(x, y, w, h, tag)  (Programmer's Guide §4.46)
  parent_->cmd(CMD_TRACK);
  parent_->cmd(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
  parent_->cmd(((uint32_t)(uint16_t)h << 16) | (uint16_t)w);
  parent_->cmd((uint32_t)tag_id);
}

} // namespace ft800
} // namespace esphome
