#pragma once
// Display list commands and co-processor command opcodes for FT800.
// All opcodes from: Bridgetek FT800_Series_Programmer_Guide v2.1
// All display list commands are 32-bit little-endian values.

#include <cstdint>

namespace esphome {
namespace ft800 {

// ---------------------------------------------------------------------------
// Display list primitives  (Programmer's Guide §2.5)
// Use these to build raw display lists or inside co-processor display lists.
// ---------------------------------------------------------------------------

// CLEAR_COLOR_RGB(r,g,b)  — set clear colour (Prog. Guide §4.5)
inline constexpr uint32_t CLEAR_COLOR_RGB(uint8_t r, uint8_t g, uint8_t b) {
  return (0x02UL << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// CLEAR_COLOR_A(a)  — set clear alpha
inline constexpr uint32_t CLEAR_COLOR_A(uint8_t a) {
  return (0x0FUL << 24) | a;
}

// CLEAR(c,s,t)  — clear colour/stencil/tag buffers  (Prog. Guide §4.6)
inline constexpr uint32_t CLEAR(uint8_t c, uint8_t s, uint8_t t) {
  return (0x26UL << 24) | ((c & 1) << 2) | ((s & 1) << 1) | (t & 1);
}

// COLOR_RGB(r,g,b)  — set drawing colour  (Prog. Guide §4.8)
inline constexpr uint32_t COLOR_RGB(uint8_t r, uint8_t g, uint8_t b) {
  return (0x04UL << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// COLOR_A(a)  — set drawing alpha
inline constexpr uint32_t COLOR_A(uint8_t a) {
  return (0x10UL << 24) | a;
}

// TAG(t)  — associate touch tag t (1–255) with subsequent primitives  (Prog. Guide §4.42)
inline constexpr uint32_t TAG(uint8_t t) {
  return (0x03UL << 24) | t;
}

// TAG_MASK(mask)  — enable/disable tag buffer update
inline constexpr uint32_t TAG_MASK(uint8_t mask) {
  return (0x14UL << 24) | (mask & 1);
}

// BEGIN(prim)  — start drawing primitive group  (Prog. Guide §4.1)
static constexpr uint8_t PRIM_BITMAPS       = 1;
static constexpr uint8_t PRIM_POINTS        = 2;
static constexpr uint8_t PRIM_LINES         = 3;
static constexpr uint8_t PRIM_LINE_STRIP    = 4;
static constexpr uint8_t PRIM_EDGE_STRIP_R  = 5;
static constexpr uint8_t PRIM_EDGE_STRIP_L  = 6;
static constexpr uint8_t PRIM_EDGE_STRIP_A  = 7;
static constexpr uint8_t PRIM_EDGE_STRIP_B  = 8;
static constexpr uint8_t PRIM_RECTS         = 9;

inline constexpr uint32_t BEGIN(uint8_t prim) {
  return (0x1FUL << 24) | (prim & 0x0F);
}

// END()  — end drawing primitive group
inline constexpr uint32_t END() {
  return (0x21UL << 24);
}

// DISPLAY()  — end of display list  (Prog. Guide §4.10)
inline constexpr uint32_t DISPLAY() {
  return 0x00000000UL;
}

// VERTEX2F(x, y)  — sub-pixel precision vertex (1/16 pixel units)  (Prog. Guide §4.44)
inline constexpr uint32_t VERTEX2F(int16_t x, int16_t y) {
  return (0x01UL << 30) | (((uint32_t)(x & 0x7FFF)) << 15) | (y & 0x7FFF);
}

// VERTEX2II(x, y, handle, cell)  — integer pixel vertex with bitmap handle  (Prog. Guide §4.45)
inline constexpr uint32_t VERTEX2II(uint16_t x, uint16_t y, uint8_t handle, uint8_t cell) {
  return (0x02UL << 30) | ((uint32_t)(x & 0x1FF) << 21) | ((uint32_t)(y & 0x1FF) << 12)
         | ((uint32_t)(handle & 0x1F) << 7) | (cell & 0x7F);
}

// POINT_SIZE(r)  — radius in 1/16 pixel units
inline constexpr uint32_t POINT_SIZE(uint16_t r) {
  return (0x0DUL << 24) | (r & 0x1FFF);
}

// LINE_WIDTH(w)  — line half-width in 1/16 pixel units
inline constexpr uint32_t LINE_WIDTH(uint16_t w) {
  return (0x0EUL << 24) | (w & 0x0FFF);
}

// BITMAP_HANDLE(handle)  — select bitmap handle 0–31
inline constexpr uint32_t BITMAP_HANDLE(uint8_t h) {
  return (0x05UL << 24) | (h & 0x1F);
}

// BITMAP_SOURCE(addr)  — RAM_G address of bitmap data
inline constexpr uint32_t BITMAP_SOURCE(uint32_t addr) {
  return (0x01UL << 24) | (addr & 0xFFFFFUL);
}

// BITMAP_LAYOUT(format, linestride, height)
inline constexpr uint32_t BITMAP_LAYOUT(uint8_t fmt, uint16_t stride, uint16_t h) {
  return (0x07UL << 24) | ((uint32_t)(fmt & 0x1F) << 19) | ((uint32_t)(stride & 0x3FF) << 9) | (h & 0x1FF);
}

// Bitmap formats for BITMAP_LAYOUT
static constexpr uint8_t BF_ARGB1555 = 0;
static constexpr uint8_t BF_L1       = 1;
static constexpr uint8_t BF_L4       = 2;
static constexpr uint8_t BF_L8       = 3;
static constexpr uint8_t BF_RGB332   = 4;
static constexpr uint8_t BF_ARGB2    = 5;
static constexpr uint8_t BF_ARGB4    = 6;
static constexpr uint8_t BF_RGB565   = 7;
static constexpr uint8_t BF_PALETTED = 8;
static constexpr uint8_t BF_TEXT8X8  = 9;
static constexpr uint8_t BF_TEXTVGA  = 10;
static constexpr uint8_t BF_BARGRAPH = 11;

// BITMAP_SIZE(filter, wrapx, wrapy, width, height)
inline constexpr uint32_t BITMAP_SIZE(uint8_t filter, uint8_t wx, uint8_t wy, uint16_t w, uint16_t h) {
  return (0x08UL << 24) | ((uint32_t)(filter & 1) << 20) | ((uint32_t)(wx & 1) << 19)
         | ((uint32_t)(wy & 1) << 18) | ((uint32_t)(w & 0x1FF) << 9) | (h & 0x1FF);
}

// SAVE_CONTEXT() / RESTORE_CONTEXT()
inline constexpr uint32_t SAVE_CONTEXT()    { return (0x22UL << 24); }
inline constexpr uint32_t RESTORE_CONTEXT() { return (0x23UL << 24); }

// RETURN() — return from display list call (CMD_CALL / CMD_JUMP)
inline constexpr uint32_t RETURN_DL() { return (0x24UL << 24); }

// MACRO(m)  — execute co-processor macro 0 or 1
inline constexpr uint32_t MACRO(uint8_t m) { return (0x25UL << 24) | (m & 1); }

// ---------------------------------------------------------------------------
// Co-processor command opcodes  (Programmer's Guide §4, Chapter 4)
// These are written into RAM_CMD via the co-processor FIFO.
// ---------------------------------------------------------------------------
static constexpr uint32_t CMD_DLSTART    = 0xFFFFFF00UL; // start new display list
static constexpr uint32_t CMD_SWAP       = 0xFFFFFF01UL; // swap display lists
static constexpr uint32_t CMD_COLDSTART  = 0xFFFFFF32UL; // reset co-proc to default state
static constexpr uint32_t CMD_INTERRUPT  = 0xFFFFFF02UL; // trigger INT_N from co-proc
static constexpr uint32_t CMD_APPEND     = 0xFFFFFF1EUL; // append RAM_G data to DL
static constexpr uint32_t CMD_REGREAD    = 0xFFFFFF19UL; // read register value
static constexpr uint32_t CMD_MEMWRITE   = 0xFFFFFF1AUL; // write data to RAM_G
static constexpr uint32_t CMD_INFLATE    = 0xFFFFFF22UL; // inflate zlib data to RAM_G
static constexpr uint32_t CMD_LOADIMAGE  = 0xFFFFFF24UL; // decode JPEG to RAM_G
static constexpr uint32_t CMD_MEMCRC     = 0xFFFFFF18UL; // CRC of RAM region
static constexpr uint32_t CMD_MEMZERO    = 0xFFFFFF1CUL; // zero-fill RAM region
static constexpr uint32_t CMD_MEMSET     = 0xFFFFFF1BUL; // fill RAM region with value
static constexpr uint32_t CMD_MEMCPY     = 0xFFFFFF1DUL; // copy RAM region
static constexpr uint32_t CMD_BUTTON     = 0xFFFFFF0DUL; // draw button widget
static constexpr uint32_t CMD_CLOCK      = 0xFFFFFF14UL; // draw clock widget
static constexpr uint32_t CMD_FGCOLOR    = 0xFFFFFF0AUL; // set co-proc foreground colour
static constexpr uint32_t CMD_BGCOLOR    = 0xFFFFFF09UL; // set co-proc background colour
static constexpr uint32_t CMD_GRADCOLOR  = 0xFFFFFF34UL; // set co-proc gradient colour
static constexpr uint32_t CMD_GAUGE      = 0xFFFFFF13UL; // draw gauge widget
static constexpr uint32_t CMD_GRADIENT   = 0xFFFFFF0BUL; // draw gradient fill
static constexpr uint32_t CMD_KEYS       = 0xFFFFFF0EUL; // draw keyboard keys
static constexpr uint32_t CMD_PROGRESS   = 0xFFFFFF0FUL; // draw progress bar
static constexpr uint32_t CMD_SCROLLBAR  = 0xFFFFFF11UL; // draw scrollbar
static constexpr uint32_t CMD_SLIDER     = 0xFFFFFF10UL; // draw slider
static constexpr uint32_t CMD_DIAL       = 0xFFFFFF2DUL; // draw dial
static constexpr uint32_t CMD_TOGGLE     = 0xFFFFFF12UL; // draw toggle switch
static constexpr uint32_t CMD_TEXT       = 0xFFFFFF0CUL; // draw text string
static constexpr uint32_t CMD_NUMBER     = 0xFFFFFF2EUL; // draw integer number
static constexpr uint32_t CMD_LOADIDENTITY = 0xFFFFFF26UL; // set matrix to identity
static constexpr uint32_t CMD_SETFONT    = 0xFFFFFF2BUL; // set custom font
static constexpr uint32_t CMD_SETBITMAP  = 0xFFFFFF43UL; // set bitmap properties
static constexpr uint32_t CMD_TRACK      = 0xFFFFFF2CUL; // track touch on component
static constexpr uint32_t CMD_TRANSLATE  = 0xFFFFFF27UL; // translate matrix
static constexpr uint32_t CMD_SCALE      = 0xFFFFFF28UL; // scale matrix
static constexpr uint32_t CMD_ROTATE     = 0xFFFFFF29UL; // rotate matrix
static constexpr uint32_t CMD_SETMATRIX  = 0xFFFFFF2AUL; // write matrix to DL
static constexpr uint32_t CMD_GETMATRIX  = 0xFFFFFF33UL; // read current matrix
static constexpr uint32_t CMD_GETPTR     = 0xFFFFFF23UL; // get end-of-inflate pointer
static constexpr uint32_t CMD_GETPROPS   = 0xFFFFFF25UL; // get image properties
static constexpr uint32_t CMD_CALIBRATE  = 0xFFFFFF15UL; // run touch calibration UI
static constexpr uint32_t CMD_SPINNER    = 0xFFFFFF16UL; // animated spinner
static constexpr uint32_t CMD_STOP       = 0xFFFFFF17UL; // stop animation
static constexpr uint32_t CMD_SCREENSAVER= 0xFFFFFF2FUL; // start screensaver
static constexpr uint32_t CMD_SKETCH     = 0xFFFFFF30UL; // free-hand sketch
static constexpr uint32_t CMD_LOGO       = 0xFFFFFF31UL; // display FTDI logo
static constexpr uint32_t CMD_SNAPSHOT   = 0xFFFFFF1FUL; // capture screenshot

// ---------------------------------------------------------------------------
// Co-processor option flags  (Programmer's Guide §4, option bitmasks)
// Pass as 'options' argument to CMD_TEXT, CMD_BUTTON, etc.
// ---------------------------------------------------------------------------
static constexpr uint16_t OPT_3D        = 0x0000; // default: 3D effect
static constexpr uint16_t OPT_RGB565    = 0x0000;
static constexpr uint16_t OPT_MONO      = 0x0001;
static constexpr uint16_t OPT_NODL      = 0x0002; // do not add generated DL commands
static constexpr uint16_t OPT_FLAT      = 0x0100; // flat (no 3D effect)
static constexpr uint16_t OPT_SIGNED    = 0x0100; // for CMD_NUMBER: treat as signed
static constexpr uint16_t OPT_CENTERX   = 0x0200; // horizontally centred
static constexpr uint16_t OPT_CENTERY   = 0x0400; // vertically centred
static constexpr uint16_t OPT_CENTER    = (OPT_CENTERX | OPT_CENTERY);
static constexpr uint16_t OPT_RIGHTX    = 0x0800; // right-justified
static constexpr uint16_t OPT_NOBACK    = 0x1000; // no background for gauge/dial
static constexpr uint16_t OPT_NOTICKS   = 0x2000; // no ticks on gauge/clock
static constexpr uint16_t OPT_NOHM      = 0x4000; // no hour/minute hands on clock
static constexpr uint16_t OPT_NOPOINTER = 0x4000; // no pointer on gauge
static constexpr uint16_t OPT_NOSECS    = 0x8000; // no seconds hand on clock
static constexpr uint16_t OPT_NOHANDS   = (OPT_NOHM | OPT_NOSECS);

// ---------------------------------------------------------------------------
// Built-in ROM font handles  (Programmer's Guide §4.7, fonts 16–34)
// ---------------------------------------------------------------------------
static constexpr uint8_t FONT_SMALL   = 18; // 8px, good for labels
static constexpr uint8_t FONT_MEDIUM  = 26; // 18px, good for values
static constexpr uint8_t FONT_LARGE   = 31; // 28px, good for headers

// ---------------------------------------------------------------------------
// Screensaver / sound helpers
// ---------------------------------------------------------------------------
// No-touch sentinel value from REG_TOUCH_SCREEN_XY when screen is not touched
static constexpr int16_t TOUCH_NO_TOUCH = -32768;

} // namespace ft800
} // namespace esphome
