# Hardware: ESP32-2432S028R ("Cheap Yellow Display")

## Overview

| Component | Details |
|---|---|
| MCU | ESP32-WROOM-32, 2 × 240 MHz Xtensa LX6, 520 KB SRAM, **no PSRAM** |
| Flash | 4 MB |
| Display | 2.8" TFT, 240×320 (320×240 in landscape), 16-bit colour, SPI |
| Display controller | ILI9341 (single micro-USB board) · ST7789 or ILI9341 (micro-USB + USB-C "CYD2USB") |
| Touch | Resistive, XPT2046 controller |
| Extras | RGB LED, LDR light sensor, speaker amplifier, micro SD slot, BOOT / RESET buttons |
| Power | 5 V (micro-USB, USB-C or the P1 connector) |

## Pin map

| Function | GPIO | Notes |
|---|---|---|
| LCD SCLK / MOSI / MISO | 14 / 13 / 12 | HSPI (SPI2), native IO_MUX pins → 40–80 MHz |
| LCD CS / DC | 15 / 2 | RST is tied to EN (−1) |
| LCD backlight | 21 | PWM (LEDC channel 0, 12 kHz) |
| Touch CLK / MOSI / MISO / CS | 25 / 32 / 39 / 33 | **Software SPI** (see below) |
| Touch IRQ | 36 | Input only; not used by the firmware (pressure measurement is sufficient) |
| RGB LED (R / G / B) | 4 / 16 / 17 | Common anode: LOW = on (LEDC output is inverted) |
| LDR (light sensor) | 34 | ADC1_CH6, input only, 0 dB attenuation |
| Speaker amplifier input | 26 | "SPEAK" JST connector, LEDC tone generation |
| SD card SCK / MISO / MOSI / CS | 18 / 19 / 23 / 5 | VSPI (SPI3) — reserved for data logging |
| BOOT button | 0 | LOW = pressed |
| CN1 connector | 22, 27 | 3.3 V, GND, IO22, IO27 → reserved for the vehicle interface UART |
| P3 connector | 35, 22, 21 | IO35 is input only; IO21 is shared with the backlight! |
| P1 (serial) | 1 / 3 | TX / RX (shared with the USB serial bridge) |

All pins are defined in one place: [`CarCYD/src/config/board_config.h`](../CarCYD/src/config/board_config.h)

### Why software SPI for the touch controller?
The board has three SPI devices (display, touch, SD card) but the ESP32 only offers two usable SPI hosts. The
display uses HSPI. The XPT2046 runs at ≤ 2.5 MHz, so driving it with software SPI costs nothing and **keeps
VSPI free for the SD card**. (LovyanGFX's own CYD board definition uses the same approach.)

## LEDC (PWM) channel plan

With the ESP32 Arduino core 3.x, **channels with the same frequency and resolution share a hardware timer**.
Speaker tones change the frequency, so sharing would make the backlight flicker. Therefore:

| Channel | Pin | Frequency / resolution | Timer |
|---|---|---|---|
| 0 | 21 backlight | 12 kHz / 8 bit (above the audible range) | own |
| 2 | 26 speaker | variable / 10 bit | own (unique configuration) |
| 4, 5, 6 | 4, 16, 17 RGB LED | 5 kHz / 8 bit | shared (the frequency never changes) |

## Board variants

`board_config.h`:

```cpp
#define CYD_PANEL CYD_PANEL_ILI9341     // single micro-USB board
#define CYD_PANEL CYD_PANEL_ILI9341_2   // micro-USB + USB-C "CYD2USB" (use with CYD_PANEL_INVERT 1)
#define CYD_PANEL CYD_PANEL_ST7789      // 2-USB boards fitted with an ST7789 (use with CYD_PANEL_INVERT 0)
#define CYD_PANEL_INVERT 1              // colours look like a photo negative
#define CYD_LCD_SPI_HZ 55000000         // faster display (works on most boards)
#define CYD_PANEL_ROTATION_OFFSET 4     // advanced: orientation / mirror override (normally not needed)
```

### Orientation and mirroring

The picture orientation depends on two things: the **GS** bit (gate scan direction, register B6h) in the
panel's init sequence and the **MADCTL** value from LovyanGFX's rotation table. The firmware selects the correct
offsets for every variant:

| Variant | B6h (GS) | Panel `offset_rotation` | Landscape MADCTL | Touch `offset_rotation` |
|---|---|---|---|---|
| `CYD_PANEL_ILI9341` | `08 C2 27` (GS=1) | 2 | MV · MY | 0 |
| `CYD_PANEL_ILI9341_2` | `08 82 27` (GS=0) | **4** (mirrored) | MV | 6 |
| `CYD_PANEL_ST7789` | — | 0 | MV · MX | 2 |

The ILI9341 and ST7789 values match LovyanGFX's Sunton ESP32-2432S028 board definition. The ILI9341_2 init
sequence clears the GS bit, which scans the long axis in the opposite direction; with the same MADCTL the
picture would be **mirrored left to right**. Offset 4 compensates for it. The touch offset is derived from the
panel offset, so touch input is unaffected (`src/hal/lcd.cpp`, verified at compile time with `static_assert`).

When the panel type or orientation changes, the stored touch calibration is discarded and the calibration
screen appears automatically at the next start.

| Symptom | Fix |
|---|---|
| White or blank screen | Try the other `CYD_PANEL` variants |
| Colours look negative | Toggle `CYD_PANEL_INVERT` |
| Picture mirrored (text reads backwards) | Check `CYD_PANEL`; if it is still mirrored set `CYD_PANEL_ROTATION_OFFSET` to 4 or 6 |
| Picture upside down | *Settings → Display → Rotate screen 180°* (no rebuild needed) |
| Touches are off or mirrored | *Settings → System → Touch calibration*, or hold BOOT while powering up |
| Noise / artefacts on screen | Lower `CYD_LCD_SPI_HZ` to 27 MHz |

## Light sensor (LDR) tuning

The LDR pulls the ADC input towards GND against a 1 MΩ pull-up: **bright light gives low readings, darkness
gives high readings**. Auto-brightness maps the raw value on a logarithmic scale between two references:

```cpp
constexpr uint16_t kLightRawBright = 40;    // bright daylight
constexpr uint16_t kLightRawDark   = 1500;  // dark cabin
```

Boards differ. *Settings → Display → Ambient light* shows `percent · raw value` live; read the values on your
board by day and at night and update the two constants.

## In-car installation

- **Never connect the CYD directly to the 12 V system.** Use an automotive grade (load-dump protected)
  12 V → 5 V converter that also tolerates the drop to 6–7 V while cranking.
- The TN panel has narrow viewing angles; mount the screen facing the driver. For upside-down mounting use
  *Settings → Display → Rotate screen 180°* (touch follows automatically).
- Keep the display out of direct sunlight on the dashboard; the panel heats up quickly.
- The RGB LED is on the back of the board and gives a background glow for the shift light and warnings. All
  warnings are always shown on screen as well; the LED can be disabled in *Settings → Sound & light*.
