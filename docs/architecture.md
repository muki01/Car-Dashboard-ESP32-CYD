# Architecture

## Layers

```
┌─────────────────────────────────────────────────────────────────┐
│ CarCYD.ino  →  app::setup() / app::loop()                        │
├─────────────────────────────────────────────────────────────────┤
│ app/        Composition root: creates the drivers, data source   │
│             and storage and wires them to core and ui            │
├───────────────────────────────┬─────────────────────────────────┤
│ ui/   LVGL user interface     │ hal/  Hardware drivers            │
│  (depends only on core +      │  (Arduino, LovyanGFX, NVS)        │
│   LVGL)                       │                                   │
├───────────────────────────────┴─────────────────────────────────┤
│ core/  Platform independent business logic (no Arduino.h)        │
│        platform.h: everything the code expects from the hardware  │
└─────────────────────────────────────────────────────────────────┘
```

**Rules**
- `core/` includes no hardware or LVGL headers → unit testing and simulation on a PC are possible.
- `ui/` reaches the hardware only through `core/platform.h` (sounds, brightness information, raw touch
  samples, system information, restart). This is why the same UI code runs in `tools/simulator`.
- `hal/` only drives hardware; it contains no business rules.
- `app/` wires everything together; no other layer depends on `app/`.

## Execution model

Everything runs cooperatively in the Arduino `loop()` task. LVGL is not thread safe, so there are no extra
tasks or locks. The main loop:

```
link.poll()            data source (demo: fixed-step model at 20 Hz)
alerts.update()        limit evaluation, debouncing, hysteresis
trip.update()          distance / time / fuel integration, 0-100 timer
alert sounds, LED      new or escalated alerts, shift light
backlight (20 Hz)      LDR filter → target brightness → smooth fade
BOOT button, buzzer and LED sequencers
settings.service()     NVS save 1.5 s after the last change
trip save (60 s)
lv_timer_handler()     UI timer 40 ms (25 Hz) + LVGL refresh (max. 50 Hz)
```

No step blocks: speaker melodies, LED patterns and backlight fades all advance with timestamps.

## Data flow

```
DataLink (SimLink / NullLink / later: ESP32 link)
   │ set(SignalId, value, now)      DTC read/clear results, link state, clock
   ▼
VehicleState ──► AlertManager ──► banner / sound / LED
   │        └──► TripComputer ──► persistent storage
   ▼
UI pages (only the visible page is updated)
```

- Every signal value carries a timestamp; a value not refreshed for 2.5 s is **stale** and shown as "--".
- Limits are defined in one place (`core/signals.cpp` → `limits()`): value colouring and alerts use the same
  function.
- DTC operations are asynchronous: `link::request_dtc_read()` → `DtcState.operation` → result in
  `DtcState.last_result` plus an incremented `revision`. The UI watches the revision number.

## Adding a data source (the ESP32 vehicle link)

1. Write a class derived from `core::DataLink` (e.g. `hal/esp32_link.cpp`, UART or ESP-NOW).
2. In `poll()`, decode incoming frames and call `state.set(SignalId::Rpm, value, now)`; keep
   `state.link_state` up to date (including a connection timeout).
3. Forward `request_dtc_read/clear()` to the interface and write the result into `state.dtc`
   (`operation` → `Idle`, `last_result`, `revision++`).
4. In `app.cpp` → `select_data_source()`, use the new class instead of `NullLink` when `demo_mode == false`.

The UI, the alerts and the trip computer need no changes. Connector CN1 (IO22 / IO27) is reserved for a UART.

## Adding a signal

1. `core/types.h` → add it to `SignalId` (the order is the order of the Live Data list).
2. `core/signals.cpp` → metadata row (and a `limits()` entry if needed).
3. `core/i18n.h` → full and short name in every language.
4. `ui/signal_format.cpp` → icon.
The signal then automatically appears in Live Data, the value picker and the chart view.

## Adding a page

Derive from `ui::Page` (`create`, `title`, `update`, optionally `on_back`, `save_state/restore_state`), add it to
`make_page()` and `PageId` in `ui.cpp` and give it an icon on the navigation rail.

## Persistent data

| Key | Content | Written |
|---|---|---|
| `settings` | `core::Settings` | 1.5 s after the last change and before a restart |
| `touch_cal` | affine correction + display orientation id | at the end of a calibration |
| `trip` | `core::TripData` | every 60 s while driving, on reset and before a restart |

Format: `[magic][version][size][CRC32][payload]` (`core/persist.h`). Structures are extended **by appending
fields only**; a shorter blob written by older firmware is loaded as a prefix and new fields keep their
defaults. Corrupt data (CRC) falls back to the defaults. Every settings field is range checked after loading.

## Memory and performance

| Resource | Value |
|---|---|
| Flash (firmware) | ≈ 770 KB of 1.25 MB (default partition scheme, OTA ready) |
| Static RAM | ≈ 26 KB |
| LVGL draw buffers | 2 × 320×40×2 B = 50 KB, DMA capable internal RAM |
| LVGL objects | pages are created on first visit, sub-pages are deleted when closed |
| Display transfer | `RGB565_SWAPPED` → DMA without conversion; LVGL renders into one buffer while the other is sent |

Techniques: only the visible page is updated; labels are redrawn only when their text changes
(`ui::set_text`); the ring gauge invalidates only the changed arc segment; no shadows or complex gradients.

## Localization

The `I18N_STRINGS` X-macro in `core/i18n.h` defines every string once, in all languages, so the tables stay in
sync at compile time. The fonts contain the required glyphs (see `tools/fonts`). Adding a language means adding a
column to the macro and a `Language` value.

## Rebuilding the UI

When the language or the accent colour changes, the whole UI is rebuilt at a safe point (`lv_async_call`); the
open page and sub-page are preserved (`save_state/restore_state`).
