#pragma once
// FT800 register addresses and memory map constants.
// All values verified against:
//   Bridgetek DS_FT800_Embedded_Video_Engine v1.4, Table 5-1 and Table 5-2
//   Bridgetek FT800_Series_Programmer_Guide v2.1, Appendix C

#include <cstdint>

namespace esphome {
namespace ft800 {

// ---------------------------------------------------------------------------
// Memory map  (DS_FT800 §5, Table 5-1)
// ---------------------------------------------------------------------------
static constexpr uint32_t RAM_G          = 0x000000UL; // 256 kB general-purpose graphics RAM
static constexpr uint32_t ROM_CHIPID     = 0x0C0000UL; // 4 B chip ID: "0800"=FT800, "0801"=FT801
static constexpr uint32_t ROM_FONT       = 0x0BB23CUL; // 275 kB built-in font bitmaps
static constexpr uint32_t ROM_FONT_ADDR  = 0x0FFFFCUL; // 4 B pointer to font table
static constexpr uint32_t RAM_DL         = 0x100000UL; // 8 kB display list RAM
static constexpr uint32_t RAM_PAL        = 0x102000UL; // 1 kB palette RAM
static constexpr uint32_t RAM_CMD        = 0x108000UL; // 4 kB co-processor command FIFO
static constexpr uint32_t RAM_SCREENSHOT = 0x1C2000UL; // 2 kB screenshot readout buffer

static constexpr uint32_t RAM_G_SIZE   = 256 * 1024;
static constexpr uint32_t RAM_DL_SIZE  = 8 * 1024;
static constexpr uint32_t RAM_CMD_SIZE = 4 * 1024;  // FIFO ring size

// REG_ID always reads this value when SPI is working  (DS_FT800 Table 5-2)
static constexpr uint8_t FT800_REG_ID_VALUE = 0x7C;

// ---------------------------------------------------------------------------
// Registers  (DS_FT800 §5.1, Table 5-2)
// ---------------------------------------------------------------------------
static constexpr uint32_t REG_ID                 = 0x102400UL;
static constexpr uint32_t REG_FRAMES             = 0x102404UL;
static constexpr uint32_t REG_CLOCK              = 0x102408UL;
static constexpr uint32_t REG_FREQUENCY          = 0x10240CUL;
static constexpr uint32_t REG_RENDERMODE         = 0x102410UL;
static constexpr uint32_t REG_SNAPY              = 0x102414UL;
static constexpr uint32_t REG_SNAPSHOT           = 0x102418UL;
static constexpr uint32_t REG_CPURESET           = 0x10241CUL; // bit0=gfx, bit1=audio, bit2=touch
static constexpr uint32_t REG_TAP_CRC            = 0x102420UL;
static constexpr uint32_t REG_TAP_MASK           = 0x102424UL;
// Display timing
static constexpr uint32_t REG_HCYCLE             = 0x102428UL;
static constexpr uint32_t REG_HOFFSET            = 0x10242CUL;
static constexpr uint32_t REG_HSIZE              = 0x102430UL;
static constexpr uint32_t REG_HSYNC0             = 0x102434UL;
static constexpr uint32_t REG_HSYNC1             = 0x102438UL;
static constexpr uint32_t REG_VCYCLE             = 0x10243CUL;
static constexpr uint32_t REG_VOFFSET            = 0x102440UL;
static constexpr uint32_t REG_VSIZE              = 0x102444UL;
static constexpr uint32_t REG_VSYNC0             = 0x102448UL;
static constexpr uint32_t REG_VSYNC1             = 0x10244CUL;
static constexpr uint32_t REG_DLSWAP             = 0x102450UL; // write 2 = swap on frame
static constexpr uint32_t REG_ROTATE             = 0x102454UL;
static constexpr uint32_t REG_OUTBITS            = 0x102458UL;
static constexpr uint32_t REG_DITHER             = 0x10245CUL;
static constexpr uint32_t REG_SWIZZLE            = 0x102460UL;
static constexpr uint32_t REG_CSPREAD            = 0x102464UL;
static constexpr uint32_t REG_PCLK_POL          = 0x102468UL; // 0=rising, 1=falling
static constexpr uint32_t REG_PCLK              = 0x10246CUL; // 0=disable; N=48MHz/N
// Tag query
static constexpr uint32_t REG_TAG_X              = 0x102470UL;
static constexpr uint32_t REG_TAG_Y              = 0x102474UL;
static constexpr uint32_t REG_TAG                = 0x102478UL; // tag at REG_TAG_X/Y
// Audio
static constexpr uint32_t REG_VOL_PB             = 0x10247CUL;
static constexpr uint32_t REG_VOL_SOUND          = 0x102480UL;
static constexpr uint32_t REG_SOUND              = 0x102484UL;
static constexpr uint32_t REG_PLAY               = 0x102488UL;
// GPIO
static constexpr uint32_t REG_GPIO_DIR           = 0x10248CUL; // default 0x80 (DISP=output)
static constexpr uint32_t REG_GPIO               = 0x102490UL; // bit7 = DISP enable
// Interrupts
static constexpr uint32_t REG_INT_FLAGS          = 0x102498UL; // r/o, cleared on read
static constexpr uint32_t REG_INT_EN             = 0x10249CUL;
static constexpr uint32_t REG_INT_MASK           = 0x1024A0UL;
// Audio playback
static constexpr uint32_t REG_PLAYBACK_START     = 0x1024A4UL;
static constexpr uint32_t REG_PLAYBACK_LENGTH    = 0x1024A8UL;
static constexpr uint32_t REG_PLAYBACK_READPTR   = 0x1024ACUL;
static constexpr uint32_t REG_PLAYBACK_FREQ      = 0x1024B0UL;
static constexpr uint32_t REG_PLAYBACK_FORMAT    = 0x1024B4UL;
static constexpr uint32_t REG_PLAYBACK_LOOP      = 0x1024B8UL;
static constexpr uint32_t REG_PLAYBACK_PLAY      = 0x1024BCUL;
// Backlight PWM
static constexpr uint32_t REG_PWM_HZ             = 0x1024C0UL; // 250–10000 Hz
static constexpr uint32_t REG_PWM_DUTY           = 0x1024C4UL; // 0=off, 128=full
// Macros
static constexpr uint32_t REG_MACRO_0            = 0x1024C8UL;
static constexpr uint32_t REG_MACRO_1            = 0x1024CCUL;
// Co-processor FIFO pointers
static constexpr uint32_t REG_CMD_READ           = 0x1024E4UL; // 12-bit read ptr
static constexpr uint32_t REG_CMD_WRITE          = 0x1024E8UL; // 12-bit write ptr
static constexpr uint32_t REG_CMD_DL             = 0x1024ECUL;
// Touch engine
static constexpr uint32_t REG_TOUCH_MODE         = 0x1024F0UL; // 0=off,1=oneshot,2=frame,3=cont
static constexpr uint32_t REG_TOUCH_ADC_MODE     = 0x1024F4UL; // 0=single-ended,1=differential
static constexpr uint32_t REG_TOUCH_CHARGE       = 0x1024F8UL;
static constexpr uint32_t REG_TOUCH_SETTLE       = 0x1024FCUL;
static constexpr uint32_t REG_TOUCH_OVERSAMPLE   = 0x102500UL;
static constexpr uint32_t REG_TOUCH_RZTHRESH     = 0x102504UL;
static constexpr uint32_t REG_TOUCH_RAW_XY       = 0x102508UL; // x=MSB16, y=LSB16; raw ADC
static constexpr uint32_t REG_TOUCH_RZ           = 0x10250CUL; // resistance; 32767=no touch
static constexpr uint32_t REG_TOUCH_SCREEN_XY    = 0x102510UL; // calibrated; -32768=no touch
static constexpr uint32_t REG_TOUCH_TAG_XY       = 0x102514UL; // XY used for tag lookup
static constexpr uint32_t REG_TOUCH_TAG          = 0x102518UL; // current tag under touch
static constexpr uint32_t REG_TOUCH_TRANSFORM_A  = 0x10251CUL; // s15.16 fixed-point
static constexpr uint32_t REG_TOUCH_TRANSFORM_B  = 0x102520UL;
static constexpr uint32_t REG_TOUCH_TRANSFORM_C  = 0x102524UL;
static constexpr uint32_t REG_TOUCH_TRANSFORM_D  = 0x102528UL;
static constexpr uint32_t REG_TOUCH_TRANSFORM_E  = 0x10252CUL;
static constexpr uint32_t REG_TOUCH_TRANSFORM_F  = 0x102530UL;
static constexpr uint32_t REG_TOUCH_DIRECT_XY    = 0x102574UL; // raw direct ADC result
static constexpr uint32_t REG_TOUCH_DIRECT_Z1Z2  = 0x102578UL;
// Tracker (for sliders/dials)
static constexpr uint32_t REG_TRACKER            = 0x109000UL; // track_val=MSB16, tag=LSB8

// ---------------------------------------------------------------------------
// REG_DLSWAP values  (Programmer's Guide §2.5.5)
// ---------------------------------------------------------------------------
static constexpr uint8_t DLSWAP_DONE  = 0; // swap complete
static constexpr uint8_t DLSWAP_LINE  = 1; // swap after next scan line
static constexpr uint8_t DLSWAP_FRAME = 2; // swap after next frame (normal use)

// ---------------------------------------------------------------------------
// REG_TOUCH_MODE values
// ---------------------------------------------------------------------------
static constexpr uint8_t TOUCH_MODE_OFF        = 0;
static constexpr uint8_t TOUCH_MODE_ONESHOT    = 1;
static constexpr uint8_t TOUCH_MODE_FRAME      = 2;
static constexpr uint8_t TOUCH_MODE_CONTINUOUS = 3;

// ---------------------------------------------------------------------------
// REG_INT_MASK / REG_INT_FLAGS bits  (DS_FT800 §4.4)
// ---------------------------------------------------------------------------
static constexpr uint8_t INT_SWAP        = (1 << 0); // display list swap
static constexpr uint8_t INT_TOUCH       = (1 << 1); // touch detected
static constexpr uint8_t INT_TAG         = (1 << 2); // touch tag changed
static constexpr uint8_t INT_SOUND       = (1 << 3); // sound effect completed
static constexpr uint8_t INT_PLAYBACK    = (1 << 4); // audio playback completed
static constexpr uint8_t INT_CMDEMPTY    = (1 << 5); // co-proc FIFO empty
static constexpr uint8_t INT_CMDFLAG     = (1 << 6); // co-proc command flag
static constexpr uint8_t INT_CONVCOMPLETE= (1 << 7); // touch ADC conversion complete

// ---------------------------------------------------------------------------
// REG_CPURESET bits  (DS_FT800 §5.1)
// ---------------------------------------------------------------------------
static constexpr uint8_t CPURESET_GFX   = (1 << 0);
static constexpr uint8_t CPURESET_AUDIO = (1 << 1);
static constexpr uint8_t CPURESET_TOUCH = (1 << 2);

// ---------------------------------------------------------------------------
// Host commands  (DS_FT800 §4.1.2 / Programmer's Guide §2.2)
// Sent as: byte0 = 0x40|cmd, byte1 = param, byte2 = 0x00
// ---------------------------------------------------------------------------
static constexpr uint8_t HOSTCMD_ACTIVE  = 0x00; // wake from sleep / activate PLL
static constexpr uint8_t HOSTCMD_STANDBY = 0x41; // standby (PLL running)
static constexpr uint8_t HOSTCMD_SLEEP   = 0x42; // sleep (PLL off)
static constexpr uint8_t HOSTCMD_PWRDOWN = 0x43; // power down
static constexpr uint8_t HOSTCMD_CLKEXT  = 0x44; // use external 12 MHz crystal
static constexpr uint8_t HOSTCMD_CLK36M  = 0x61; // select 36 MHz PLL (internal clock)
static constexpr uint8_t HOSTCMD_CLK48M  = 0x62; // select 48 MHz PLL (internal clock)
static constexpr uint8_t HOSTCMD_CLKINT  = 0x48; // use internal oscillator
static constexpr uint8_t HOSTCMD_CORERST = 0x68; // core reset

// ---------------------------------------------------------------------------
// SPI read/write/command prefix bits  (DS_FT800 §4.2)
// Address is 22 bits; 2 MSBs of the first byte are the prefix.
// ---------------------------------------------------------------------------
static constexpr uint8_t SPI_PREFIX_READ  = 0x00; // bits[23:22] = 00
static constexpr uint8_t SPI_PREFIX_WRITE = 0x80; // bits[23:22] = 10
static constexpr uint8_t SPI_PREFIX_CMD   = 0x40; // bits[23:22] = 01 (host command)

// ---------------------------------------------------------------------------
// Co-processor FIFO sentinel: fault detection  (Programmer's Guide §2.4)
// ---------------------------------------------------------------------------
static constexpr uint16_t FIFO_FAULT_VALUE = 0x0FFF; // REG_CMD_READ == 0xFFF = fault

// ---------------------------------------------------------------------------
// Display timing profile  (Programmer's Guide §2.2, VM800B datasheet)
// Plain struct — no ESPHome types, safe to include in host-side tests.
// ---------------------------------------------------------------------------
struct DisplayProfile {
  uint16_t hcycle;    // total horizontal cycles
  uint16_t hoffset;   // horizontal display start
  uint16_t hsize;     // horizontal pixel count
  uint16_t hsync0;    // hsync fall offset
  uint16_t hsync1;    // hsync rise offset
  uint16_t vcycle;    // total vertical cycles
  uint16_t voffset;   // vertical display start
  uint16_t vsize;     // vertical pixel count
  uint16_t vsync0;    // vsync fall offset
  uint16_t vsync1;    // vsync rise offset
  uint8_t  pclk;      // PCLK divisor (48 MHz / pclk = pixel clock)
  uint8_t  pclk_pol;  // 0 = rising edge, 1 = falling edge
  uint8_t  cspread;   // clock spreading
  uint8_t  swizzle;   // RGB pin arrangement
};

// WQVGA 480×272 — VM800B43A / VM800B50A
// Timing values: Programmer's Guide §2.2 Code Snippet 1
static constexpr DisplayProfile PROFILE_WQVGA = {
  548, 43,  480, 0, 41,
  292, 12,  272, 0, 10,
  5, 1, 1, 0
};

// QVGA 320×240 — VM800B35A
// Timing values: VM800B35 header in Bridgetek Arduino library
static constexpr DisplayProfile PROFILE_QVGA = {
  408, 70,  320, 0, 10,
  263, 13,  240, 0,  2,
  8, 0, 1, 0
};

} // namespace ft800
} // namespace esphome
