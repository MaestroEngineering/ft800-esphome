# FT800 EVE ESPHome Component

ESPHome external component for Bridgetek FT800 / FT801 EVE display dev kits (VM800B family).  
Works like the Nextion integration: declare your UI in YAML, push values from Home Assistant, get touch events back as HA entities.

---

## Hardware Supported

| Board | Chip | Panel | Resolution | Touch |
|---|---|---|---|---|
| VM800B50A | FT800 | 5.0" | 480×272 (WQVGA) | Resistive |
| VM800B43A | FT800 | 4.3" | 480×272 (WQVGA) | Resistive |
| VM800B35A | FT800 | 3.5" | 320×240 (QVGA) | Resistive |

FT801 (capacitive touch) is not yet implemented.

---

## Wiring (ESP32 DevKit 38-pin)

| FT800 Pin | ESP32 GPIO | Notes |
|---|---|---|
| SCK | 18 | VSPI clock |
| MOSI | 23 | VSPI data out |
| MISO | 19 | VSPI data in |
| CS | 5 | Active low chip-select |
| PD_N | 4 | Power-down, active low — required for clean reset |
| INT_N | 2 | Touch interrupt, active low — optional but recommended |
| 3V3 | 3.3V | FT800 is 3.3V logic |
| GND | GND | |

**Important:** The FT800 is 3.3V. Do not connect 5V logic directly.  
The VM800B has its own 5V→3.3V regulator; power it from the USB connector on the board.

---

## Installation

Add to your ESPHome YAML:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/your-username/esphome-ft808
      ref: main
    components: [ft800]
```

Or from a local path during development:

```yaml
external_components:
  - source:
      type: local
      path: /path/to/esphome-ft808/components
```

---

## Minimal Config

```yaml
spi:
  clk_pin: GPIO18
  mosi_pin: GPIO23
  miso_pin: GPIO19

ft800:
  id: eve
  cs_pin: GPIO5
  pd_pin: GPIO4       # optional but strongly recommended
  int_pin: GPIO2      # optional; enables interrupt-driven touch
  display_size: 5in   # 5in | 4.3in | 3.5in
  brightness: 80      # 0-100 %

display:
  - platform: ft800
    ft800_id: eve
    update_interval: 50ms
    lambda: |-
      it.clear_color_rgb(0, 0, 64);
      it.clear_screen();
      it.color_rgb(255, 255, 255);
      it.text_centered(240, 136, 31, "Hello FT800!");
```

---

## YAML Reference

### `ft800:` block

| Key | Type | Default | Description |
|---|---|---|---|
| `id` | ID | required | Component ID |
| `cs_pin` | pin | required | SPI chip-select |
| `pd_pin` | pin | — | Power-down pin (active low) |
| `int_pin` | pin | — | INT_N interrupt pin (active low, falling edge) |
| `display_size` | string | `5in` | `5in`, `4.3in`, or `3.5in` |
| `brightness` | 0–100 | 80 | Backlight brightness (%) |

### `display:` platform `ft800`

| Key | Type | Default | Description |
|---|---|---|---|
| `ft800_id` | ID ref | required | Reference to `ft800:` block |
| `lambda` | lambda | — | Drawing callback; receives `FT800Display &it` |
| `update_interval` | duration | `50ms` | How often to redraw (20 Hz at 50ms) |

### `binary_sensor:` platform `ft800`

Touch press/release on a specific TAG (1–255).

```yaml
binary_sensor:
  - platform: ft800
    ft800_id: eve
    tag: 1
    name: "My Button"
    on_press:
      - light.toggle: my_light
```

### `sensor:` platform `ft800`

Tracker value (slider/dial position) from a specific TAG, normalised 0.0–1.0.

```yaml
sensor:
  - platform: ft800
    ft800_id: eve
    tag: 10
    name: "Brightness Slider"
    on_value:
      - light.control:
          id: my_light
          brightness: !lambda "return x;"
```

---

## Drawing API (`it.*` inside `lambda:`)

### Colors and clear

```cpp
it.clear_color_rgb(r, g, b);   // set clear color
it.clear_screen();             // clear with clear color
it.color_rgb(r, g, b);        // set drawing color
it.fg_color(Color(r,g,b));    // widget foreground
it.bg_color(Color(r,g,b));    // widget background
```

### Text and numbers

```cpp
// font: ROM fonts 16–34 (18=tiny, 21=small, 26=medium, 31=large)
it.text(x, y, font, options, "string");
it.text_centered(x, y, font, "string");  // OPT_CENTER shortcut
it.number(x, y, font, options, int_value);
```

### Widgets

```cpp
it.button(x, y, w, h, font, options, "label");
it.toggle(x, y, w, font, options, state_0_or_65535, "OFF\xFFON");
it.slider(x, y, w, h, options, value, range);
it.progress(x, y, w, h, options, value, range);
it.gauge(x, y, radius, options, major_divs, minor_divs, value, range);
it.dial(x, y, radius, options, value);   // value 0–65535
it.clock(x, y, radius, options, h, m, s, ms);
it.keys(x, y, w, h, font, options, "ABCDEF...");
```

### Shapes

```cpp
it.rect(x, y, w, h, Color(r,g,b));
it.line(x0, y0, x1, y1, width_px, Color(r,g,b));
it.circle(cx, cy, radius, Color(r,g,b));
```

### Touch TAGs and tracking

```cpp
it.tag(id);          // associate subsequent widgets with this touch tag (1–255)
it.tag(0);           // disable tag assignment
it.track(x, y, w, h, tag_id);  // register slider region for REG_TRACKER
```

---

## Pages

Pages are implemented in software — simply branch on a global variable:

```yaml
globals:
  - id: page
    type: int
    initial_value: "0"

display:
  - platform: ft800
    ...
    lambda: |-
      if (id(page) == 0) {
        // page 0 widgets
      } else {
        // page 1 widgets
      }
```

---

## Touch Calibration

Run calibration once after wiring. The result is stored in ESP32 NVS and reloaded on every boot.

1. Add a button in your YAML:
   ```yaml
   binary_sensor:
     - platform: ft800
       tag: 99
       name: "Calibrate"
       on_press:
         - lambda: id(eve).run_calibration();
   ```
2. Press the calibration button. Follow the on-screen targets.
3. The calibration matrix is saved automatically to NVS.

To reset calibration: call `id(eve).clear_calibration()`.

---

## Asset Tools

### Image conversion

```bash
cd tools/
python3 convert_image.py my_image.png my_image.bin        # raw RGB565 binary
python3 convert_image.py my_image.png my_image.h --header  # C PROGMEM header
```

Load in YAML lambda:
```cpp
// After loading binary data to RAM_G at address 0x1000:
it.dl(BITMAP_HANDLE(0));
it.dl(BITMAP_SOURCE(0x1000));
it.dl(BITMAP_LAYOUT(BF_RGB565, width*2, height));
it.dl(BITMAP_SIZE(0, 0, 0, width, height));
it.begin(PRIM_BITMAPS);
it.vertex_px(x, y);
it.end_prim();
```

---

## Bring-Up Checklist

Follow this sequence on first power-up.

1. **Power check:** Measure 3.3V on the FT800 VCC pin before connecting to ESP32.
2. **SPI loopback:** Disconnect FT800. Short MOSI→MISO. Flash `minimal.yaml`. Verify logger shows a byte echo test (remove after confirming).
3. **Boot verification:** Connect FT800. Flash `minimal.yaml`. In ESPHome logs, look for `REG_ID OK (0x7C)`. If you see `REG_ID=0x00`, check CS, MOSI, SCK wiring.
4. **Display visible:** The display should show a dark blue screen with "FT800 EVE" text.
5. **Touch test:** Open `touch_control.yaml`. Tap the screen. Watch logs for `TAG=N` output. If touch doesn't register, check INT_N wiring (or switch to polled mode by removing `int_pin`).
6. **Calibration:** Tap the calibration button. Touch all three targets accurately. Test tap accuracy afterwards.
7. **HA round-trip:** Adopt the device in HA. Create an automation that sets a sensor value; verify the number updates on screen.

---

## Limitations (vs. Nextion)

| Feature | Status |
|---|---|
| Single touch only | FT800 is resistive single-touch by design |
| No on-device scripting | All logic in ESPHome YAML / HA automations |
| No Nextion TFT upload | Layout is YAML-compiled, not a .tft file |
| Custom fonts require offline conversion | Use `tools/convert_font.py` (TODO) |
| No FT801 capacitive support | Not yet implemented |
| RAM_G = 256 kB total | Full 480×272 RGB565 image = 253 kB alone |
| JPEG decoded in co-processor | PNG requires offline pre-conversion |

---

## What Cannot Be Verified Without Hardware

- Actual display output (correct colours, timing, pixel alignment)
- Touch calibration accuracy
- SPI timing at 20 MHz on long wires
- INT_N interrupt latency on ESP32
- Audio ROM sounds via `REG_SOUND`
- `CMD_CALIBRATE` interactive UI

Everything in `tests/test_commands.cpp` runs host-side and is verified.
