// Host-side unit tests for FT800 command encoding.
// Run without hardware: just validates the bit packing in ft800_commands.h
// and ft800_registers.h against the datasheet tables.
//
// Build: g++ -std=c++17 -I../components/ft800 test_commands.cpp -o test_commands
// Run:   ./test_commands

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>

// Stub out ESPHome types so we can compile the headers standalone
namespace esphome { namespace ft800 {} }

#include "../components/ft800/ft800_registers.h"
#include "../components/ft800/ft800_commands.h"
// ft800.h is NOT included here — it requires ESPHome headers.
// DisplayProfile / PROFILE_* are now in ft800_registers.h (plain structs).

using namespace esphome::ft800;

static int pass_count = 0;
static int fail_count = 0;

#define EXPECT_EQ(a, b, msg) do { \
  if ((a) == (b)) { pass_count++; } \
  else { printf("FAIL: %s  (got 0x%08X, expected 0x%08X)\n", msg, (unsigned)(a), (unsigned)(b)); fail_count++; } \
} while(0)

#define EXPECT_NE(a, b, msg) do { \
  if ((a) != (b)) { pass_count++; } \
  else { printf("FAIL: %s  (both 0x%08X)\n", msg, (unsigned)(a)); fail_count++; } \
} while(0)


// ---------------------------------------------------------------------------
// Memory map addresses  (DS_FT800 Table 5-1)
// ---------------------------------------------------------------------------
static void test_memory_map() {
  EXPECT_EQ(RAM_G,          0x000000UL, "RAM_G start");
  EXPECT_EQ(ROM_CHIPID,     0x0C0000UL, "ROM_CHIPID");
  EXPECT_EQ(RAM_DL,         0x100000UL, "RAM_DL");
  EXPECT_EQ(RAM_PAL,        0x102000UL, "RAM_PAL");
  EXPECT_EQ(RAM_CMD,        0x108000UL, "RAM_CMD");
  EXPECT_EQ(RAM_DL_SIZE,    8192U,      "RAM_DL size = 8kB");
  EXPECT_EQ(RAM_CMD_SIZE,   4096U,      "RAM_CMD size = 4kB");
  EXPECT_EQ(RAM_G_SIZE,     262144U,    "RAM_G size = 256kB");
}

// ---------------------------------------------------------------------------
// Key register addresses  (DS_FT800 Table 5-2)
// ---------------------------------------------------------------------------
static void test_registers() {
  EXPECT_EQ(REG_ID,               0x102400UL, "REG_ID");
  EXPECT_EQ(REG_CPURESET,         0x10241CUL, "REG_CPURESET");
  EXPECT_EQ(REG_HSIZE,            0x102430UL, "REG_HSIZE");
  EXPECT_EQ(REG_VSIZE,            0x102444UL, "REG_VSIZE");
  EXPECT_EQ(REG_PCLK,             0x10246CUL, "REG_PCLK");
  EXPECT_EQ(REG_DLSWAP,           0x102450UL, "REG_DLSWAP");
  EXPECT_EQ(REG_GPIO,             0x102490UL, "REG_GPIO");
  EXPECT_EQ(REG_GPIO_DIR,         0x10248CUL, "REG_GPIO_DIR");
  EXPECT_EQ(REG_CMD_READ,         0x1024E4UL, "REG_CMD_READ");
  EXPECT_EQ(REG_CMD_WRITE,        0x1024E8UL, "REG_CMD_WRITE");
  EXPECT_EQ(REG_TOUCH_TAG,        0x102518UL, "REG_TOUCH_TAG");
  EXPECT_EQ(REG_TOUCH_SCREEN_XY,  0x102510UL, "REG_TOUCH_SCREEN_XY");
  EXPECT_EQ(REG_TOUCH_TRANSFORM_A,0x10251CUL, "REG_TOUCH_TRANSFORM_A");
  EXPECT_EQ(REG_TOUCH_TRANSFORM_F,0x102530UL, "REG_TOUCH_TRANSFORM_F");
  EXPECT_EQ(REG_TRACKER,          0x109000UL, "REG_TRACKER");
  EXPECT_EQ(REG_PWM_DUTY,         0x1024C4UL, "REG_PWM_DUTY");
  EXPECT_EQ(FT800_REG_ID_VALUE,   0x7CU,      "REG_ID sentinel value");
}

// ---------------------------------------------------------------------------
// CLEAR_COLOR_RGB encoding  (Programmer's Guide §4.5)
// Opcode = 0x02, bits[23:16]=R, [15:8]=G, [7:0]=B
// ---------------------------------------------------------------------------
static void test_clear_color_rgb() {
  uint32_t cmd = CLEAR_COLOR_RGB(0xFF, 0x80, 0x00);
  EXPECT_EQ((cmd >> 24) & 0xFF, 0x02U, "CLEAR_COLOR_RGB opcode");
  EXPECT_EQ((cmd >> 16) & 0xFF, 0xFFU, "CLEAR_COLOR_RGB R");
  EXPECT_EQ((cmd >>  8) & 0xFF, 0x80U, "CLEAR_COLOR_RGB G");
  EXPECT_EQ((cmd      ) & 0xFF, 0x00U, "CLEAR_COLOR_RGB B");
}

// ---------------------------------------------------------------------------
// CLEAR encoding  (Programmer's Guide §4.6)
// Opcode = 0x26, bits[2]=C, [1]=S, [0]=T
// ---------------------------------------------------------------------------
static void test_clear() {
  EXPECT_EQ(CLEAR(1, 1, 1), 0x26000007UL, "CLEAR(1,1,1)");
  EXPECT_EQ(CLEAR(1, 0, 0), 0x26000004UL, "CLEAR(1,0,0)");
  EXPECT_EQ(CLEAR(0, 0, 0), 0x26000000UL, "CLEAR(0,0,0)");
}

// ---------------------------------------------------------------------------
// COLOR_RGB encoding  (Programmer's Guide §4.8)
// Opcode = 0x04
// ---------------------------------------------------------------------------
static void test_color_rgb() {
  uint32_t cmd = COLOR_RGB(10, 20, 30);
  EXPECT_EQ((cmd >> 24) & 0xFF, 0x04U, "COLOR_RGB opcode");
  EXPECT_EQ((cmd >> 16) & 0xFF, 10U,   "COLOR_RGB R");
  EXPECT_EQ((cmd >>  8) & 0xFF, 20U,   "COLOR_RGB G");
  EXPECT_EQ((cmd      ) & 0xFF, 30U,   "COLOR_RGB B");
}

// ---------------------------------------------------------------------------
// TAG encoding  (Programmer's Guide §4.42)
// Opcode = 0x03, bits[7:0] = tag value
// ---------------------------------------------------------------------------
static void test_tag() {
  EXPECT_EQ(TAG(0),   0x03000000UL, "TAG(0)");
  EXPECT_EQ(TAG(1),   0x03000001UL, "TAG(1)");
  EXPECT_EQ(TAG(255), 0x030000FFUL, "TAG(255)");
}

// ---------------------------------------------------------------------------
// DISPLAY()  (Programmer's Guide §4.10)
// Must be exactly 0x00000000
// ---------------------------------------------------------------------------
static void test_display() {
  EXPECT_EQ(DISPLAY(), 0x00000000UL, "DISPLAY()");
}

// ---------------------------------------------------------------------------
// VERTEX2F encoding  (Programmer's Guide §4.44)
// bits[31:30]=01, [29:15]=x (1/16px), [14:0]=y (1/16px)
// ---------------------------------------------------------------------------
static void test_vertex2f() {
  // Pixel (10, 20) = subpixel (160, 320)
  uint32_t v = VERTEX2F(160, 320);
  EXPECT_EQ((v >> 30) & 0x3, 0x01U, "VERTEX2F prefix bits");
  EXPECT_EQ((v >> 15) & 0x7FFF, 160U,  "VERTEX2F x");
  EXPECT_EQ( v        & 0x7FFF, 320U,  "VERTEX2F y");
}

// ---------------------------------------------------------------------------
// Co-processor opcodes (all should have upper byte = 0xFF)
// ---------------------------------------------------------------------------
static void test_coproc_opcodes() {
  // All CMD_* opcodes have bits[31:24] = 0xFF
  const uint32_t opcodes[] = {
    CMD_DLSTART, CMD_SWAP, CMD_COLDSTART, CMD_BUTTON, CMD_TEXT,
    CMD_NUMBER, CMD_TOGGLE, CMD_SLIDER, CMD_PROGRESS, CMD_GAUGE,
    CMD_DIAL, CMD_CLOCK, CMD_KEYS, CMD_TRACK, CMD_CALIBRATE,
  };
  for (uint32_t op : opcodes) {
    EXPECT_EQ((op >> 24) & 0xFF, 0xFFU, "CMD opcode upper byte");
  }
  // Spot-check specific values
  EXPECT_EQ(CMD_DLSTART, 0xFFFFFF00UL, "CMD_DLSTART");
  EXPECT_EQ(CMD_SWAP,    0xFFFFFF01UL, "CMD_SWAP");
  EXPECT_EQ(CMD_TEXT,    0xFFFFFF0CUL, "CMD_TEXT");
  EXPECT_EQ(CMD_BUTTON,  0xFFFFFF0DUL, "CMD_BUTTON");
  EXPECT_EQ(CMD_TRACK,   0xFFFFFF2CUL, "CMD_TRACK");
  EXPECT_EQ(CMD_CALIBRATE,0xFFFFFF15UL,"CMD_CALIBRATE");
}

// ---------------------------------------------------------------------------
// Host command opcodes  (DS_FT800 §4.1.2)
// ---------------------------------------------------------------------------
static void test_host_commands() {
  EXPECT_EQ(HOSTCMD_ACTIVE,  0x00U, "HOSTCMD_ACTIVE");
  EXPECT_EQ(HOSTCMD_CLKEXT,  0x44U, "HOSTCMD_CLKEXT");
  EXPECT_EQ(HOSTCMD_SLEEP,   0x42U, "HOSTCMD_SLEEP");
  EXPECT_EQ(HOSTCMD_PWRDOWN, 0x43U, "HOSTCMD_PWRDOWN");
  EXPECT_EQ(HOSTCMD_CORERST, 0x68U, "HOSTCMD_CORERST");
  EXPECT_EQ(SPI_PREFIX_READ,  0x00U, "SPI_PREFIX_READ");
  EXPECT_EQ(SPI_PREFIX_WRITE, 0x80U, "SPI_PREFIX_WRITE");
  EXPECT_EQ(SPI_PREFIX_CMD,   0x40U, "SPI_PREFIX_CMD");
}

// ---------------------------------------------------------------------------
// Display profiles: check 480×272 timing values
// ---------------------------------------------------------------------------
static void test_profiles() {
  EXPECT_EQ(PROFILE_WQVGA.hsize,  480U, "WQVGA hsize");
  EXPECT_EQ(PROFILE_WQVGA.vsize,  272U, "WQVGA vsize");
  EXPECT_EQ(PROFILE_WQVGA.hcycle, 548U, "WQVGA hcycle");
  EXPECT_EQ(PROFILE_WQVGA.vcycle, 292U, "WQVGA vcycle");
  EXPECT_EQ(PROFILE_WQVGA.pclk,     5U, "WQVGA pclk = 48/5 = 9.6MHz");
  EXPECT_EQ(PROFILE_QVGA.hsize,   320U, "QVGA hsize");
  EXPECT_EQ(PROFILE_QVGA.vsize,   240U, "QVGA vsize");
  EXPECT_EQ(PROFILE_QVGA.pclk,      8U, "QVGA pclk = 48/8 = 6MHz");
}

// ---------------------------------------------------------------------------
// FIFO fault sentinel value
// ---------------------------------------------------------------------------
static void test_fifo_fault() {
  EXPECT_EQ(FIFO_FAULT_VALUE, 0x0FFFU, "FIFO fault sentinel = 4095");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
  printf("FT800 command encoding tests\n");
  printf("==============================\n");

  test_memory_map();
  test_registers();
  test_clear_color_rgb();
  test_clear();
  test_color_rgb();
  test_tag();
  test_display();
  test_vertex2f();
  test_coproc_opcodes();
  test_host_commands();
  test_profiles();
  test_fifo_fault();

  printf("------------------------------\n");
  printf("PASSED: %d  FAILED: %d\n", pass_count, fail_count);
  return (fail_count == 0) ? 0 : 1;
}
