# Desktop simulator

Runs the firmware's `core/` and `ui/` code on a PC with the same `lv_conf.h` and fonts as the device. It is
headless: a virtual clock and scripted touch input walk through every screen and save a screenshot of each one.
Use it to review design changes without flashing the board and as a UI smoke test. The screenshots, the demo
animation and the banner in the main README are produced with it.

## Requirements

- Python 3.9+ and Pillow (`pip install pillow`)
- A C/C++ compiler: **zig** (recommended, a single archive: https://ziglang.org/download/) or gcc / clang
- The LVGL library (default: `Documents/Arduino/libraries/lvgl`, override with `--lvgl` or `LVGL_DIR`)

## Usage

```bash
python tools/simulator/build.py --zig C:/tools/zig/zig.exe             # English screenshots
python tools/simulator/build.py --zig C:/tools/zig/zig.exe --lang tr   # Turkish UI
python tools/simulator/build.py --zig C:/tools/zig/zig.exe --demo --banner
python tools/simulator/build.py --cc gcc --cxx g++                      # with gcc
```

Output:
- `docs/screenshots/<lang>/*.png` — every screen, scaled 2× pixel-exact (`--scale 1` for native size)
- `docs/screenshots/overview_<lang>.png` — all screens on one page
- `docs/media/demo.gif` — animated tour (`--demo`), `docs/media/banner.png` — 1280×640 hero / social preview
  image (`--banner`)

The first build compiles about 500 files including LVGL; later builds are incremental.

## Files

| File | Purpose |
|---|---|
| `sim_main.cpp` | In-memory LVGL display and input driver, virtual clock, scenarios, frame capture |
| `sim_platform.cpp` | PC implementation of `core/platform.h` |
| `build.py` | Parallel incremental build, run, PNG conversion, overview sheet |
| `media.py` | Demo GIF assembly (with touch markers) and banner rendering |

To extend the tour, add `tap()`, `swipe()`, `long_press()`, `run_until()` and `snap()` steps to `scenario()` in
`sim_main.cpp` (or `demo_tour()` for the animation).
