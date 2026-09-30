#include "lcd.h"

#include "../config/board_config.h"

namespace hal {
namespace {

// Panel rotation offsets (LovyanGFX: bits 0-1 = 90 degree steps, bit 2 = mirror).
#if CYD_PANEL == CYD_PANEL_ST7789
using PanelType = lgfx::Panel_ST7789;
constexpr const char* kPanelName = "ST7789 320x240";
constexpr uint8_t kDefaultRotationOffset = 0;
#elif CYD_PANEL == CYD_PANEL_ILI9341_2
using PanelType = lgfx::Panel_ILI9341_2;
constexpr const char* kPanelName = "ILI9341 320x240";
// The ILI9341_2 init sequence clears GS (gate scan direction, B6h = 08 82 27),
// which mirrors the long axis compared to Panel_ILI9341 (B6h = 08 C2 27).
// The mirror bit restores the MADCTL the panel needs (MV only in landscape).
constexpr uint8_t kDefaultRotationOffset = 4;
#else
using PanelType = lgfx::Panel_ILI9341;
constexpr const char* kPanelName = "ILI9341 320x240";
constexpr uint8_t kDefaultRotationOffset = 2;
#endif

#ifdef CYD_PANEL_ROTATION_OFFSET
constexpr uint8_t kPanelRotationOffset = (CYD_PANEL_ROTATION_OFFSET) & 7;
#else
constexpr uint8_t kPanelRotationOffset = kDefaultRotationOffset;
#endif

// The touch sheet is mounted the same way on every CYD: in our landscape mode
// (setRotation(1)) LovyanGFX must map it with effective rotation 3, whatever
// offset the panel needs. LovyanGFX combines the offsets as
//   internal = ((r + panel) & 3) | ((r & 4) ^ (panel & 4))
//   touch    = ((internal + t) & 3) | ((internal & 4) ^ (t & 4))
// which gives touch == 3 for r == 1 when t = ((2 - panel) & 3) | (panel & 4).
constexpr uint8_t kTouchRotationOffset =
    static_cast<uint8_t>(((2 - kPanelRotationOffset) & 3) | (kPanelRotationOffset & 4));

/** LovyanGFX's rotation/offset combination (Panel_LCD::setRotation, Panel_Device::convertRawXY). */
constexpr uint8_t combine_rotation(uint8_t r, uint8_t offset) {
  return static_cast<uint8_t>(((r + offset) & 3) | ((r & 4) ^ (offset & 4)));
}
static_assert(combine_rotation(combine_rotation(1, kPanelRotationOffset), kTouchRotationOffset) == 3,
              "touch must be mapped with rotation 3 in landscape");
static_assert(combine_rotation(combine_rotation(3, kPanelRotationOffset), kTouchRotationOffset) == 1,
              "touch must be mapped with rotation 1 in flipped landscape");

class CydDevice final : public lgfx::LGFX_Device {
 public:
  CydDevice() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = SPI2_HOST;  // HSPI, native IO_MUX pins
      cfg.spi_mode = 0;
      cfg.freq_write = CYD_LCD_SPI_HZ;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.pin_sclk = board::kLcdSclk;
      cfg.pin_mosi = board::kLcdMosi;
      cfg.pin_miso = board::kLcdMiso;
      cfg.pin_dc = board::kLcdDc;
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }
    {
      auto cfg = panel_.config();
      cfg.pin_cs = board::kLcdCs;
      cfg.pin_rst = board::kLcdRst;
      cfg.pin_busy = -1;
      cfg.panel_width = board::kLcdWidth;
      cfg.panel_height = board::kLcdHeight;
      cfg.memory_width = board::kLcdWidth;
      cfg.memory_height = board::kLcdHeight;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = kPanelRotationOffset;
      cfg.readable = true;
      cfg.invert = CYD_PANEL_INVERT;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      panel_.config(cfg);
    }
    {
      auto cfg = touch_.config();
      cfg.x_min = 300;  // nominal raw range of the CYD touch sheet
      cfg.x_max = 3900;
      cfg.y_min = 3700;
      cfg.y_max = 200;
      cfg.pin_int = -1;
      cfg.bus_shared = false;
      cfg.offset_rotation = kTouchRotationOffset;
      cfg.spi_host = -1;  // software SPI on the dedicated touch pins
      cfg.freq = 1000000;
      cfg.pin_sclk = board::kTouchClk;
      cfg.pin_mosi = board::kTouchMosi;
      cfg.pin_miso = board::kTouchMiso;
      cfg.pin_cs = board::kTouchCs;
      touch_.config(cfg);
      panel_.setTouch(&touch_);
    }
    setPanel(&panel_);
  }

 private:
  lgfx::Bus_SPI bus_;
  PanelType panel_;
  lgfx::Touch_XPT2046 touch_;
};

CydDevice g_device;

}  // namespace

lgfx::LGFX_Device& lcd() { return g_device; }

const char* lcd_panel_name() { return kPanelName; }

uint32_t lcd_orientation_id() {
  return (static_cast<uint32_t>(CYD_PANEL) << 16) | (static_cast<uint32_t>(kPanelRotationOffset) << 8) |
         kTouchRotationOffset;
}

}  // namespace hal
