#include "display.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

#include "../config/app_config.h"
#include "../core/log.h"
#include "lcd.h"

namespace hal::display {
namespace {

constexpr uint8_t kRotationLandscape = 1;
constexpr uint8_t kRotationLandscapeFlipped = 3;

lv_display_t* g_display = nullptr;
bool g_flipped = false;

void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
  const int32_t w = lv_area_get_width(area);
  const int32_t h = lv_area_get_height(area);
  // LVGL renders LV_COLOR_FORMAT_RGB565_SWAPPED, i.e. exactly the byte order the
  // panel expects: the buffer is sent by DMA without any conversion. LovyanGFX
  // waits for the previous transfer before starting the next one, and LVGL
  // always renders into the other buffer, so we can release this one at once.
  lcd().pushImageDMA(area->x1, area->y1, w, h, reinterpret_cast<const lgfx::swap565_t*>(px_map));
  lv_display_flush_ready(disp);
}

void* alloc_dma(size_t bytes) { return heap_caps_malloc(bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL); }

}  // namespace

lv_display_t* init(bool flipped) {
  lgfx::LGFX_Device& dev = lcd();
  if (!dev.init()) {
    LOG_E("DISP", "panel init failed");
    return nullptr;
  }
  dev.initDMA();
  g_flipped = flipped;
  dev.setRotation(flipped ? kRotationLandscapeFlipped : kRotationLandscape);
  dev.fillScreen(0x0000);
  dev.startWrite();  // the panel owns the HSPI bus: keep the transaction open

  const int32_t w = dev.width();
  const int32_t h = dev.height();
  const size_t buffer_bytes = static_cast<size_t>(w) * APP_DRAW_BUFFER_LINES * sizeof(uint16_t);
  void* buf1 = alloc_dma(buffer_bytes);
  void* buf2 = alloc_dma(buffer_bytes);
  if (buf1 == nullptr || buf2 == nullptr) {
    LOG_E("DISP", "cannot allocate %u B draw buffers", static_cast<unsigned>(2 * buffer_bytes));
    return nullptr;
  }

  g_display = lv_display_create(w, h);
  lv_display_set_color_format(g_display, LV_COLOR_FORMAT_RGB565_SWAPPED);
  lv_display_set_buffers(g_display, buf1, buf2, buffer_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(g_display, flush_cb);
  LOG_I("DISP", "%s %ldx%ld, 2 x %u B DMA buffers", lcd_panel_name(), static_cast<long>(w), static_cast<long>(h),
        static_cast<unsigned>(buffer_bytes));
  return g_display;
}

void set_flipped(bool flipped) {
  if (g_display == nullptr || flipped == g_flipped) return;
  g_flipped = flipped;
  lcd().waitDMA();
  lcd().setRotation(flipped ? kRotationLandscapeFlipped : kRotationLandscape);
  lv_obj_invalidate(lv_screen_active());
  lv_obj_invalidate(lv_layer_top());
}

bool flipped() { return g_flipped; }

}  // namespace hal::display
