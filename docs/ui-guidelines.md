# UI Guidelines (Design System)

All values live in [`CarCYD/src/ui/theme.h`](../CarCYD/src/ui/theme.h); components never hard-code colours or
sizes.

## Principles

1. **Glanceable** — the most important value while driving (speed / rpm) is the largest element; secondary
   values sit in the corners with small, dim captions.
2. **Colour means state** — colours only communicate status: normal is white, cold / info is blue, warning is
   amber, critical is red. The accent colour is reserved for interactive elements and gauges in their normal
   state.
3. **Dark first** — dark background for night driving, contrast even at low brightness, automatic brightness.
4. **Made for resistive touch** — minimum touch target 40 px, frequent controls larger; steppers (−/+) instead
   of sliders for precise values.
5. **Feedback** — every touch gets a pressed state and a click sound; results of actions are confirmed with a
   short notification.
6. **Safe actions** — irreversible actions (clearing codes, factory reset, trip reset) require a confirmation
   dialog; the destructive button is red.

## Layout (320 × 240)

```
 0        56                                   320
 ┌────────┬─────────────────────────────────────┐ 0
 │        │ Status bar (title · status icons)   │
 │  Nav   ├─────────────────────────────────────┤ 28
 │  rail  │                                     │
 │ 5×48px │  Content area 264 × 212             │
 │        │                                     │
 └────────┴─────────────────────────────────────┘ 240
```

- **Navigation rail** on the driver side (left): large touch targets, accent bar on the active page, attention
  badge on Diagnostics.
- **Status bar**: page title / back arrow; on the right check engine, alert, DEMO, link and clock.
- **Grid**: outer padding 8 px, gap between cards 6 px, card radius 10 px, row height 44 px.

## Colours

| Token | Hex | Usage |
|---|---|---|
| `bg` | `#06080C` | App background |
| `surface` | `#0C1016` | Rail, status bar |
| `card` | `#131922` | Cards, list rows |
| `card_hi` | `#1C2430` | Pressed state, secondary button |
| `stroke` | `#232C39` | Dividers, outlines |
| `track` | `#1A212B` | Gauge track |
| `text` | `#E9EEF4` | Primary text |
| `text_dim` | `#8B97A7` | Secondary text, units |
| `text_faint` | `#4F5B6C` | Disabled text, minor ticks |
| `ok` | `#2ED47A` | Success, connected |
| `info` | `#3DA5FF` | Info, cold engine, pending code |
| `warn` | `#FFB020` | Warning, check engine light |
| `crit` | `#FF4B4B` | Critical, red zone, destructive action |

**Accent colours** (user selectable): Cyan `#1FCBF2` (default) · Orange `#FF8A1F` · Red `#F23A4B` ·
Green `#25D38A` · Violet `#9D84FF`

## Typography

The typeface is **Barlow Semi Condensed** (DIN-like and narrow, so more information fits on a small screen); the
speed digits use **Barlow Condensed**. Icons come from **Material Design Icons**.

| Font | Size | Usage |
|---|---|---|
| `ui_font_12` | 12 px Medium | Units, captions, chips |
| `ui_font_14` | 14 px Medium | List text (LVGL default font) |
| `ui_font_16` | 16 px SemiBold | Titles, buttons |
| `ui_font_20` | 20 px SemiBold | Tile values |
| `ui_font_28` | 28 px SemiBold | Large values |
| `ui_font_speed` | 64 px Condensed | Speed (digits only) |
| `ui_icons_24/48` | 24 / 48 px | Rail icons / empty state icons |

The 12–20 px fonts also contain the icons, so `ICON_*` macros can be used inside text. LVGL's internal symbols
(drop-down arrow, check mark, ...) are redrawn from the same icon set.

## Components

| Component | File | Notes |
|---|---|---|
| Button (primary / secondary / danger / ghost) | `widgets.cpp` | 40 px high |
| Card, chip, divider | `widgets.cpp` | |
| Ring gauge | `components/ring_gauge.cpp` | 270°, red zone, gear / up-shift box, needle sweep |
| Mini gauge | `components/mini_gauge.cpp` | 240°, caption inside the open arc segment |
| Corner value | `components/stat_tile.cpp` | long-press opens the value picker |
| Settings rows | `components/setting_rows.cpp` | navigation, switch, slider, segmented, stepper, info, action |
| Dialog / sheet | `components/dialog.cpp` | dimmed backdrop, tap outside to close |
| Notification banner | `components/banner.cpp` | alerts (tap to dismiss) and short confirmations (2.5 s) |

## Alert behaviour

| Severity | Visual | Sound | LED |
|---|---|---|---|
| Info (cold engine etc.) | value in blue | — | — |
| Warning | amber value + amber banner + status icon | two beeps (once) | amber pulse |
| Critical | red value + red banner + blinking icon | alarm, repeated every 15 s until acknowledged | red blink |

An alert is raised only after its condition persists for a while (debouncing) and clears only once the value is
clearly back in range (hysteresis). An acknowledged alert stays silent unless it escalates.

## Text

- Short and action oriented: "Scan", "Clear", "Reset".
- Numbers use fixed decimals; stale data shows "--".
- Long single-line texts end with "…" (`ui::single_line`); descriptions wrap onto several lines.
