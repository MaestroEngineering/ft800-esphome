# DESIGN: ESPHome FT800 EVE Display Component

**Hardware target:** Bridgetek FT800 (resistive touch) on VM800B50A (5.0", 480×272)  
**Host MCU:** ESP32 DevKit 38-pin  
**Reference:** Bridgetek DS_FT800 v1.4, FT800 Programmer's Guide v2.1  
**Status:** Phase 2 — Design

---

## 1. Repository Layout

```
components/ft800/
  __init__.py           Python config schema + codegen
  display.py            Python display platform registration
  touch.py              Python touchscreen platform registration
  ft800_registers.h     Register addresses & memory map (no logic, cite datasheet)
  ft800_commands.h      DL command macros + co-processor command opcodes
  ft800_hal.h/.cpp      Hardware abstraction: SPI read/write/host_command
  ft800.h/.cpp          Core driver: boot, init, FIFO management, IRQ
  ft800_display.h/.cpp  ESPHome Display component (layout + rendering)
  ft800_touch.h/.cpp    ESPHome Touchscreen component (ISR + tag dispatch)
  ft800_widget.h/.cpp   Widget descriptors (text, number, button, slider, …)

examples/
  minimal.yaml          Boot + static text, no HA
  dashboard.yaml        Live sensor values from HA
  touch_control.yaml    Toggle light, slider brightness, page navigation

tools/
  convert_image.py      PNG → raw RGB565 for RAM_G upload
  convert_font.py       TTF → FT800 custom font format

tests/
  test_commands.cpp     Host-side unit tests for DL/FIFO encoding
  test_config.py        Python schema validation tests

README.md
DESIGN.md               (this file)
```

---

## 2. SPI Wiring Defaults (ESP32 VSPI)

| Signal | ESP32 GPIO | Notes |
|--------|-----------|-------|
| SCK    | 18        | VSPI default |
| MOSI   | 23        | VSPI default |
| MISO   | 19        | VSPI default |
| CS     | 5         | Active low |
| PD_N   | 4         | Active low: drive low ≥ 20 ms to hard-reset chip |
| INT_N  | 2         | Active low; falling edge = touch/fault interrupt |

All pins are YAML-configurable. PD_N and INT_N are optional in the schema but recommended.

---

## 3. Layer Architecture

```
┌──────────────────────────────────────────────────────────────┐
│  ESPHome YAML / Home Assistant automations                   │
├───────────────────────┬──────────────────────────────────────┤
│  ft800_display.h/cpp  │  ft800_touch.h/cpp                   │
│  Display component    │  Touchscreen component               │
│  (ESPHome display::   │  (ESPHome touchscreen:: base)        │
│   DisplayBuffer base) │  binary_sensor per TAG               │
├───────────────────────┴──────────────────────────────────────┤
│  ft800_widget.h/cpp                                          │
│  Widget tree: page, text, button, slider, …                  │
│  Assigns TAG IDs, owns dirty flag, formats CMD_ args         │
├──────────────────────────────────────────────────────────────┤
│  ft800.h/cpp   (FT800 core driver)                          │
│  Boot sequence, co-processor FIFO send/await, IRQ handler    │
├──────────────────────────────────────────────────────────────┤
│  ft800_hal.h/cpp   (HAL)                                     │
│  Thin wrapper around ESPHome SPIDevice                       │
│  rd8/rd16/rd32, wr8/wr16/wr32, host_command, burst_write    │
├──────────────────────────────────────────────────────────────┤
│  ESP32 Arduino SPI / ESPHome SPIComponent                    │
└──────────────────────────────────────────────────────────────┘
```

The EVE command layer (`ft800.h/.cpp` + `ft800_hal.h/.cpp`) is kept free of
ESPHome-specific includes so it can be ported to bare STM32 later.

---

## 4. ESPHome Component Integration

### YAML schema (sketch)

```yaml
spi:
  clk_pin: GPIO18
  mosi_pin: GPIO23
  miso_pin: GPIO19

ft800:
  id: my_eve
  cs_pin: GPIO5
  pd_pin: GPIO4           # optional but strongly recommended
  int_pin: GPIO2          # optional; enables interrupt-driven touch
  brightness: 80          # 0-100, maps to REG_PWM_DUTY 0-128
  display_size: 5in       # 5in | 4.3in | 3.5in (selects timing constants)

display:
  - platform: ft800
    ft800_id: my_eve
    lambda: |-
      // direct lambda access for custom DL commands
      it.text(10, 10, "Hello!");

touchscreen:
  - platform: ft800
    ft800_id: my_eve
    calibration_data: !secret ft800_cal   # hex string stored in secrets.yaml

binary_sensor:
  - platform: ft800
    ft800_id: my_eve
    tag: 1
    name: "Button 1 pressed"
    on_press:
      - light.toggle: living_room_light

sensor:
  - platform: ft800
    ft800_id: my_eve
    tag: 2                # slider/dial tracking
    name: "Brightness slider"
    on_value:
      - light.control:
          id: living_room_light
          brightness: !lambda return x / 100.0;
```

### Python codegen responsibilities (`__init__.py`)

- Validate chip is reachable (register `REG_ID == 0x7C`) — ESPHome validates at flash time
- Validate tag IDs 1–255, detect duplicates, emit `static_assert` if exceeded
- `display_size:` maps to a `DisplayProfile` struct with all 12 timing constants
- Emit C++ `FT800Component::set_timing_profile(...)` calls in generated code
- Schema error messages name the FT800 register being configured

---

## 5. Nextion Parity Map

| Nextion feature | FT800 equivalent | Gap / notes |
|---|---|---|
| **Pages** | Widget list filtered by `page_id`; `CMD_DLSTART` → build page → `CMD_SWAP` | Full rebuild per page change |
| **Named components** | TAG IDs 1–255 (YAML assigns at compile time); C++ symbol = name | Max 255 touchable components per session |
| **Set text** | Update `std::array<char, N>` buffer in widget; set dirty flag | Rebuilt on next `update()` |
| **Set value / number** | Update int field; set dirty flag | Same |
| **Set colour** | Update `Color` struct; set dirty flag | Same |
| **Set visibility** | `visible_` bool on widget; omit from DL when false | Same |
| **Set position** | Update x/y; set dirty | Same |
| **Text component** | `CMD_TEXT(x, y, font, options, str)` | ROM fonts 16–34 built-in; custom via `CMD_SETFONT` |
| **Number component** | `CMD_NUMBER(x, y, font, options, n)` | Integer; float formatted via lambda |
| **Button** | `CMD_BUTTON(x, y, w, h, font, options, str)` + `TAG(id)` | |
| **Toggle / switch** | `CMD_TOGGLE(x, y, w, font, options, state, labels)` | Labels = "OFF\xFFON" |
| **Slider** | `CMD_SLIDER(x, y, w, h, options, val, range)` + `CMD_TRACK` | Use REG_TRACKER for live drag |
| **Progress bar** | `CMD_PROGRESS(x, y, w, h, options, val, range)` | |
| **Gauge / dial** | `CMD_GAUGE` / `CMD_DIAL` | |
| **Clock face** | No built-in; draw with `CMD_DIAL` + text overlay | Not exact Nextion match |
| **Keyboard / keys** | `CMD_KEYS(x, y, w, h, font, options, keys)` | |
| **Scrollbar** | No built-in; approximate with `CMD_SLIDER` | |
| **Rectangle** | DL: `TAG(id)` + `BEGIN(RECTS)` + `VERTEX2F(...)` × 2 | |
| **Line / point** | DL: `BEGIN(LINES)` / `BEGIN(POINTS)` | |
| **Bitmap / image** | Load to RAM_G; `CMD_LOADIMAGE` for JPEG/PNG; `CMD_SETBITMAP` | 256 kB RAM_G total; convert offline |
| **Custom font** | Load font bitmap to RAM_G; `CMD_SETFONT(handle, addr)` | Offline conversion step needed |
| **Touch press/release** | TAG transition: 0→N = press, N→0 = release | Single touch only (resistive) |
| **Touch hold** | Software timer on press start | Configurable threshold in YAML |
| **Slider / toggle change** | `REG_TRACKER` fires on drag; dispatched as `sensor` update | |
| **Page change action** | `ft800.set_page` ESPHome action | |
| **Backlight / brightness** | `REG_PWM_DUTY` (0–128); `ft800.set_brightness` action | |
| **Sleep / wake** | Host commands `SLEEP` / `ACTIVE`; `ft800.sleep` / `ft800.wake` actions | Touch wakes via INT_N |
| **Sound / beep** | `REG_SOUND` + `REG_PLAY`; `ft800.play_sound` action | ROM tone library only unless PCM loaded to RAM_G |
| **Lambda / `it.` API** | Full display object passed to `lambda:`; call `it.cmd_text(...)` etc. | Full DL access |
| **Nextion scripting / timers** | **Not available** | All logic lives in ESPHome automations |
| **Nextion TFT upload** | **Not applicable** | Layout defined at compile time in YAML |
| **Multi-touch** | **Not available** | FT800 is single-touch resistive |

---

## 6. Display-List / FIFO Strategy

### Frame render cycle

```
update() called from ESPHome loop():
  if (!dirty_ && !force_) return;
  wait_for_fifo_idle();            // poll REG_CMD_READ == REG_CMD_WRITE
  cmd(CMD_DLSTART);                // start new DL
  for each widget on active page (sorted by z_order):
    if widget.visible_:
      cmd(TAG(widget.tag_id_));
      widget.render_cmds(*this);   // emits CMD_TEXT / CMD_BUTTON / etc.
  cmd(DISPLAY());                  // DL terminator (raw DL command)
  cmd(CMD_SWAP);                   // swap front/back
  flush_fifo();                    // advance REG_CMD_WRITE
  dirty_ = false;
```

### FIFO management

- Local `uint16_t fifo_wr_` tracks write pointer (mod 4096).
- Before each burst write: check `4096 - ((fifo_wr_ - rd16(REG_CMD_READ)) & 0xFFF) >= needed_bytes`.
- If insufficient space: wait with timeout (5 s watchdog); on timeout declare co-processor fault.
- After writing all commands: `wr16(REG_CMD_WRITE, fifo_wr_)` to trigger execution.
- Wrap-around is automatic: the 4 KB FIFO is ring-buffered in hardware.

### Co-processor fault recovery

```
Fault detected when REG_CMD_READ == 0xFFF (4095).
Recovery:
  1. wr8(REG_CPURESET, 1)    // assert reset on graphics engine (bit 0)
  2. wr8(REG_CPURESET, 0)    // release
  3. wr16(REG_CMD_READ, 0)   // clear FIFO pointers
  4. wr16(REG_CMD_WRITE, 0)
  5. force_update_            // full re-render on next loop()
```

### Display list size

RAM_DL is 8 kB = 2,048 × 32-bit commands.  
Using co-processor FIFO means DL is compiled in hardware — we never write raw DL directly (except the initial boot DL). The co-processor can generate larger DLs from compact FIFO commands.  
Maximum RAM_CMD FIFO payload is 4 kB per flush. Large pages are split into multiple flush cycles by the driver.

---

## 7. Touch Architecture

### Interrupt path

```
INT_N (GPIO2) falling edge  →  IRAM_ATTR ISR sets touch_pending_ = true

loop():
  if (touch_pending_) {
    touch_pending_ = false;
    uint8_t tag  = rd8(REG_TOUCH_TAG);       // 0 = no touch
    uint32_t xy  = rd32(REG_TOUCH_SCREEN_XY);
    int16_t  x   = (int16_t)(xy >> 16);
    int16_t  y   = (int16_t)(xy & 0xFFFF);
    dispatch_touch(tag, x, y);
  }
```

`dispatch_touch()` compares `tag` with `last_tag_`:
- `last_tag_ == 0` and `tag != 0` → **press** event for tag
- `last_tag_ != 0` and `tag == 0` → **release** event for last_tag_
- `tag != 0` and `tag == last_tag_` → **hold** (accumulate time)
- `tag != 0` and `tag != last_tag_` → release old, press new

### Touch calibration

- Factory-default transform matrix: identity (1:1 raw to pixel, uncalibrated).
- On first boot or on `ft800.calibrate` action: run `CMD_CALIBRATE`, store 6 transform registers to ESP32 NVS (or `preferences`).
- Calibration data exposed as a hex string for `secrets.yaml` persistence.
- REG_TOUCH_TRANSFORM_A–F are s15.16 fixed-point; store/restore as 6 × uint32.

### Tracker (slider / toggle)

- After building the DL, call `CMD_TRACK(x, y, w, h, tag)` for each slider/toggle widget.
- `REG_TRACKER` returns `(track_value << 16) | tag`; poll on each VSYNC/touch interrupt.
- Map track_value (0–65535) to widget range; fire `on_value` trigger.

---

## 8. Asset Handling

### RAM_G budget (256 kB)

| Asset type | Size estimate | Notes |
|---|---|---|
| ROM fonts 16–34 | 0 (in ROM) | Built-in; always available |
| Custom font (one face) | ≈ 4–16 kB | Pre-rendered bitmap strip |
| Small icon (32×32 RGB565) | 2 kB | |
| Full-screen 480×272 RGB565 | 253 kB | Nearly fills RAM_G alone |
| JPEG (decoded to RGB565) | Size of decoded bitmap | Use `CMD_LOADIMAGE` for JPEG decoding |

Recommendation: keep bitmaps small; use vector widgets for UI chrome; JPEG for backgrounds.

### Offline conversion (`tools/`)

- `convert_image.py`: PIL/Pillow → RGB565 raw bytes → C array header or binary file.
- `convert_font.py`: FreeType2 → bitmap strip at requested size + metrics table.
- Both tools emit a `PROGMEM` C array and a `tools/assets_manifest.json` used by `__init__.py` to auto-generate `CMD_LOADIMAGE` / `CMD_SETFONT` calls during setup.

### Upload strategy

Assets in `PROGMEM` (flash) are DMA-copied to RAM_G during `setup()` before first frame, using burst SPI writes. The driver tracks `next_free_ram_g_` to pack assets contiguously.

---

## 9. Failure Handling

| Failure | Detection | Recovery |
|---|---|---|
| Chip not found | `REG_ID != 0x7C` after boot | Log error, mark component failed, skip all updates |
| Wrong chip | `ROM_CHIPID bytes != "0800"` | Log warning (FT801 would read "0801"; still functional if touch path matches) |
| Co-processor fault | `REG_CMD_READ == 0xFFF` | Reset GFX engine, re-render (see §6) |
| SPI bus error | ESPHome `SPIDevice` exception | Handled by ESPHome core |
| Touch calibration missing | NVS read fails | Use identity matrix; auto-run `CMD_CALIBRATE` if `auto_calibrate: true` |
| SPI too fast during init | Wrong data read back | Init at 1 MHz; raise to 20 MHz after `REG_ID` verified |
| FIFO timeout | Watchdog in `wait_for_fifo_idle()` | Log + reset co-processor |

### SPI speed schedule

1. `setup()` phase: **1 MHz** (conservative; FT800 may not have crystal locked yet)
2. After `ACTIVE` + 20 ms delay + `REG_ID == 0x7C` confirmed: raise to **10 MHz**
3. After full boot sequence + `REG_PCLK` written: raise to **20 MHz** (safe maximum for ESP32 VSPI at 3.3V long wires; FT800 supports 30 MHz)

---

## 10. Thread / Timing Model

ESPHome is single-threaded. Rules:

- **No SPI in ISR.** The `INT_N` ISR only sets `volatile bool touch_pending_`.
- **`setup()`**: boot FT800, load assets to RAM_G, render first frame, enable interrupts.
- **`update()`**: called by ESPHome scheduler (default period 50 ms → 20 fps). Rebuilds DL if dirty. Blocked calls not allowed; FIFO waits have a 200 ms timeout before fault recovery.
- **`loop()`**: called every cycle. Checks `touch_pending_`; reads touch registers; fires touch triggers. Also checks for VSYNC-aligned tracker updates.
- **No `delay()`** anywhere in the driver. All waits are bounded loops with `millis()` guards.

---

## 11. C++ Source Layout (clean separation)

```
ft800_hal.h/.cpp
  class FT800HAL              ← pure SPI I/O, no ESPHome types
    void rd_mem(addr, buf, len)
    void wr_mem(addr, buf, len)
    uint8_t  rd8(addr)
    uint16_t rd16(addr)
    uint32_t rd32(addr)
    void wr8(addr, val); wr16; wr32
    void host_command(cmd, param)

ft800.h/.cpp
  class FT800Core : public FT800HAL
    bool boot()                ← full init sequence
    void cmd(uint32_t)         ← write 4 bytes to FIFO
    void cmd_string(const char*) ← FIFO string with padding
    void cmd_buf(const void*, size_t)
    void flush()               ← write REG_CMD_WRITE
    void await_idle()          ← poll + timeout + fault recovery
    uint16_t fifo_free()

ft800_display.h/.cpp
  class FT800Display : public display::DisplayBuffer,
                       public FT800Core
    (ESPHome-facing; includes esphome headers)

ft800_touch.h/.cpp
  class FT800Touchscreen : public touchscreen::Touchscreen
    (ESPHome-facing)
```

The split between `FT800Core` and `FT800Display` keeps the portable HAL/core layer free of ESPHome includes, enabling a future STM32 port by swapping only the HAL SPI calls.

---

## 12. Open Issues / Decisions

| # | Issue | Decision |
|---|---|---|
| 1 | `CMD_CALIBRATE` interactive on-screen | Provide `ft800.calibrate` ESPHome action + separate `calibration.yaml` example |
| 2 | Custom font upload at runtime | Phase 3 MVP: compile-time only; runtime upload deferred |
| 3 | Audio PCM playback | Phase 3 MVP: ROM sounds only (`ft800.play_sound`); PCM deferred |
| 4 | JPEG/PNG decompression | Use `CMD_LOADIMAGE` (co-processor built-in for JPEG); PNG requires offline conversion |
| 5 | Multi-page memory | All pages pre-built in YAML; only active page rendered; no runtime page creation |
| 6 | QSPI | FT800 is SPI-only (QSPI is EVE3+); N/A |
| 7 | I²C alternative | FT800 supports I²C but max 3.4 MHz; too slow for display; SPI only in this driver |

---

*Sources verified against:*  
*[Bridgetek DS_FT800 v1.4](https://brtchip.com/wp-content/uploads/Support/Documentation/Datasheets/ICs/EVE/DS_FT800_Embedded_Video_Engine.pdf)*  
*[FT800 Programmer's Guide v2.1](https://brtchip.com/wp-content/uploads/Support/Documentation/Programming_Guides/ICs/EVE/FT800_Series_Programmer_Guide.pdf)*  
*[Bridgetek/EVE1-Arduino-Dev (MIT)](https://github.com/Bridgetek/EVE1-Arduino-Dev)*
