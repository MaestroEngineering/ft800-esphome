#pragma once
// FT800 touch sub-components for ESPHome.
// FT800BinarySensor: fires on press/release of a specific TAG.
// FT800Sensor:       reports the tracker value (0.0–1.0) from a slider/dial TAG.

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "ft800.h"

namespace esphome {
namespace ft800 {

// ---------------------------------------------------------------------------
// Binary sensor: press/release on a named TAG
// ---------------------------------------------------------------------------
class FT800BinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  FT800BinarySensor(FT800Component *parent, uint8_t tag_id)
      : parent_(parent), tag_id_(tag_id) {}

  void setup() override {
    parent_->add_touch_listener([this](const TouchEvent &ev, TouchEventType type) {
      if (ev.tag != tag_id_) return;
      if (type == TouchEventType::PRESS)   publish_state(true);
      if (type == TouchEventType::RELEASE) publish_state(false);
    });
  }

  void dump_config() override {
    LOG_BINARY_SENSOR("", "FT800 Binary Sensor", this);
    ESP_LOGCONFIG("ft800.touch", "  TAG: %u", tag_id_);
  }

  uint8_t tag_id() const { return tag_id_; }

 protected:
  FT800Component *parent_;
  uint8_t tag_id_;
};

// ---------------------------------------------------------------------------
// Sensor: tracker value from slider/dial (normalised 0.0–1.0)
// ---------------------------------------------------------------------------
class FT800Sensor : public sensor::Sensor, public Component {
 public:
  FT800Sensor(FT800Component *parent, uint8_t tag_id)
      : parent_(parent), tag_id_(tag_id) {}

  void setup() override {
    parent_->add_touch_listener([this](const TouchEvent &ev, TouchEventType type) {
      if (ev.tag != tag_id_) return;
      if (type != TouchEventType::TRACK_CHANGE) return;
      // REG_TRACKER range is 0–65535; normalise to 0.0–1.0
      float normalised = (float)ev.track / 65535.0f;
      publish_state(normalised);
    });
  }

  void dump_config() override {
    LOG_SENSOR("", "FT800 Sensor", this);
    ESP_LOGCONFIG("ft800.touch", "  TAG: %u (tracker)", tag_id_);
  }

  uint8_t tag_id() const { return tag_id_; }

 protected:
  FT800Component *parent_;
  uint8_t tag_id_;
};

} // namespace ft800
} // namespace esphome
