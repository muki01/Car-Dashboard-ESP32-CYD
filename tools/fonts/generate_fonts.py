#!/usr/bin/env python3
"""
CarCYD font & icon generator.

Generates the LVGL bitmap fonts used by the firmware (src/ui/fonts/*.c) and the
icon code-point header (src/ui/fonts/ui_icons.h) from the TTF sources in ./ttf.

Requirements:
    - Node.js (npx) — lv_font_conv is fetched automatically on first run.

Usage:
    python tools/fonts/generate_fonts.py
    python tools/fonts/generate_fonts.py --lv-font-conv path/to/lv_font_conv.js

This file is the single source of truth for glyph ranges and icons. Never edit
the generated files by hand; change this script and re-run it instead.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TTF_DIR = Path(__file__).resolve().parent / "ttf"
OUT_DIR = ROOT / "CarCYD" / "src" / "ui" / "fonts"
LV_FONT_CONV_VERSION = "1.5.3"

# ---------------------------------------------------------------------------
# Text glyphs: printable ASCII, the extra Latin letters of the Turkish translation
# and a few typographic symbols.
# ---------------------------------------------------------------------------
TEXT_RANGES = [
    "0x20-0x7E",        # printable ASCII
    "0xB0",             # degree sign
    "0xB1",             # plus-minus sign
    "0xB7",             # middle dot
    "0xD7",             # multiplication sign
    "0xC7,0xE7",        # C with cedilla (upper / lower)
    "0xD6,0xF6",        # O with diaeresis
    "0xDC,0xFC",        # U with diaeresis
    "0x11E,0x11F",      # G with breve
    "0x130,0x131",      # I with dot above / dotless i
    "0x15E,0x15F",      # S with cedilla
    "0x2013,0x2014",    # en dash, em dash
    "0x2022",           # bullet (LV_SYMBOL_BULLET)
    "0x2026",           # horizontal ellipsis
]

DIGIT_SYMBOLS = " -.,:0123456789"

# ---------------------------------------------------------------------------
# LVGL built-in symbols used internally by widgets (dropdown arrow, checkmark,
# etc.). They are re-drawn from Material Design Icons so the whole UI shares one
# icon style. Format: (LV_SYMBOL code point, MDI source code point)
# ---------------------------------------------------------------------------
LV_SYMBOL_REMAP = [
    (0xF00C, 0xF012C),  # LV_SYMBOL_OK       <- check
    (0xF00D, 0xF0156),  # LV_SYMBOL_CLOSE    <- close
    (0xF053, 0xF0141),  # LV_SYMBOL_LEFT     <- chevron-left
    (0xF054, 0xF0142),  # LV_SYMBOL_RIGHT    <- chevron-right
    (0xF067, 0xF0415),  # LV_SYMBOL_PLUS     <- plus
    (0xF068, 0xF0374),  # LV_SYMBOL_MINUS    <- minus
    (0xF071, 0xF0026),  # LV_SYMBOL_WARNING  <- alert
    (0xF077, 0xF0143),  # LV_SYMBOL_UP       <- chevron-up
    (0xF078, 0xF0140),  # LV_SYMBOL_DOWN     <- chevron-down
]

# ---------------------------------------------------------------------------
# Application icons (Material Design Icons). Each icon is re-mapped into the
# Unicode private use area starting at U+E000, in the order listed here.
# Format: (C macro suffix, MDI name, MDI code point)
# ---------------------------------------------------------------------------
ICONS = [
    # Navigation
    ("DASHBOARD", "speedometer", 0xF04C5),
    ("LIVE", "chart-line", 0xF012A),
    ("DIAG", "car-wrench", 0xF1814),
    ("TRIP", "map-marker-distance", 0xF08F0),
    ("SETTINGS", "cog", 0xF0493),
    # Vehicle signals
    ("ENGINE", "engine", 0xF01FA),
    ("COOLANT", "coolant-temperature", 0xF03C8),
    ("OIL_TEMP", "oil-temperature", 0xF0FF8),
    ("BATTERY", "car-battery", 0xF010C),
    ("FUEL", "gas-station", 0xF0298),
    ("FUEL_RATE", "fuel", 0xF07CA),
    ("THERMOMETER", "thermometer", 0xF050F),
    ("AIR", "weather-windy", 0xF059D),
    ("GAUGE", "gauge", 0xF029A),
    ("GAUGE_LOW", "gauge-low", 0xF0875),
    ("SPEED", "speedometer-medium", 0xF0F85),
    ("THROTTLE", "valve", 0xF1066),
    ("TURBO", "car-turbocharger", 0xF101A),
    ("TIMING", "angle-acute", 0xF0937),
    ("TRIM", "tune-vertical", 0xF066A),
    ("PISTON", "piston", 0xF088A),
    ("SHIFT", "car-shift-pattern", 0xF0F40),
    ("SPEED_LIMIT", "car-speed-limiter", 0xF190E),
    ("FLASH", "lightning-bolt", 0xF140B),
    ("CAR", "car", 0xF010B),
    ("CAR_INFO", "car-info", 0xF11BE),
    # Trip / statistics
    ("ROAD", "road-variant", 0xF0462),
    ("TIMER", "timer-outline", 0xF051B),
    ("STOPWATCH", "timer", 0xF13AB),
    ("HISTORY", "history", 0xF02DA),
    ("SIGMA", "sigma", 0xF04A0),
    ("ARROW_UP", "arrow-up-bold", 0xF0737),
    # Status & feedback
    ("ALERT", "alert", 0xF0026),
    ("ALERT_CIRCLE", "alert-circle", 0xF0028),
    ("INFO", "information-outline", 0xF02FD),
    ("CHECK_CIRCLE", "check-circle", 0xF05E0),
    ("CHECK", "check", 0xF012C),
    ("CLOSE", "close", 0xF0156),
    ("CHEVRON_RIGHT", "chevron-right", 0xF0142),
    ("BACK", "arrow-left", 0xF004D),
    ("PLUS", "plus", 0xF0415),
    ("MINUS", "minus", 0xF0374),
    ("LINK", "lan-connect", 0xF0318),
    ("LINK_OFF", "lan-disconnect", 0xF0319),
    ("VIEW_GRID", "view-grid-outline", 0xF11D9),
    ("VIEW_DASH", "view-dashboard-outline", 0xF0A1D),
    # Actions
    ("SCAN", "magnify", 0xF0349),
    ("DELETE", "trash-can-outline", 0xF0A7A),
    ("RESTORE", "restore", 0xF099B),
    ("RESTART", "restart", 0xF0709),
    # Settings
    ("BRIGHTNESS", "brightness-6", 0xF00DF),
    ("BRIGHTNESS_AUTO", "brightness-auto", 0xF00E1),
    ("SUN", "white-balance-sunny", 0xF05A8),
    ("NIGHT", "weather-night", 0xF0594),
    ("ROTATE", "screen-rotation", 0xF0475),
    ("PALETTE", "palette", 0xF03D8),
    ("RULER", "ruler", 0xF046D),
    ("BELL", "bell-ring", 0xF009E),
    ("VOLUME", "volume-high", 0xF057E),
    ("LED", "led-on", 0xF032C),
    ("TOUCH", "gesture-tap", 0xF0741),
    ("TARGET", "crosshairs", 0xF01A3),
    ("LANGUAGE", "translate", 0xF05CA),
    ("CHIP", "chip", 0xF061A),
    ("MEMORY", "memory", 0xF035B),
    ("DATABASE", "database", 0xF01BC),
]

# Large icons used for empty states and the splash screen.
ICONS_LARGE = ["CHECK_CIRCLE", "SCAN", "ALERT_CIRCLE", "ENGINE", "TARGET", "LINK_OFF", "DASHBOARD"]

PUA_BASE = 0xE000

# ---------------------------------------------------------------------------
# Font set. Each entry produces one C file.
#   name, size, text ttf (or None), text spec, include LV symbols, icon set
#   text spec: "text" = TEXT_RANGES, "digits" = DIGIT_SYMBOLS only
#   icon set:  None, "all" or "large"
# ---------------------------------------------------------------------------
FONTS = [
    ("ui_font_12", 12, "BarlowSemiCondensed-Medium.ttf", "text", True, "all"),
    ("ui_font_14", 14, "BarlowSemiCondensed-Medium.ttf", "text", True, "all"),
    ("ui_font_16", 16, "BarlowSemiCondensed-SemiBold.ttf", "text", True, "all"),
    ("ui_font_20", 20, "BarlowSemiCondensed-SemiBold.ttf", "text", True, "all"),
    ("ui_font_28", 28, "BarlowSemiCondensed-SemiBold.ttf", "text", False, None),
    ("ui_font_speed", 64, "BarlowCondensed-SemiBold.ttf", "digits", False, None),
    ("ui_icons_24", 24, None, None, False, "all"),
    ("ui_icons_48", 48, None, None, False, "large"),
]

MDI_TTF = "materialdesignicons-webfont.ttf"


def icon_codepoint(index: int) -> int:
    return PUA_BASE + index


def utf8_escape(cp: int) -> str:
    return "".join(f"\\x{b:02X}" for b in chr(cp).encode("utf-8"))


def find_converter(explicit: str | None) -> list[str]:
    if explicit:
        return ["node", explicit]
    npx = shutil.which("npx") or shutil.which("npx.cmd")
    if not npx:
        sys.exit("error: Node.js (npx) not found. Install Node.js or pass --lv-font-conv.")
    return [npx, "--yes", f"lv_font_conv@{LV_FONT_CONV_VERSION}"]


def rel(path: Path) -> str:
    """Repository-relative POSIX path, so generated file headers are machine independent."""
    return path.relative_to(ROOT).as_posix()


def build_args(name, size, text_ttf, text_spec, lv_symbols, icon_set) -> list[str]:
    args = ["--bpp", "4", "--size", str(size), "--format", "lvgl", "--no-compress",
            "--lv-include", "lvgl.h", "--lv-font-name", name,
            "-o", rel(OUT_DIR / f"{name}.c")]
    if text_ttf:
        args += ["--font", rel(TTF_DIR / text_ttf)]
        if text_spec == "text":
            args += ["-r", ",".join(TEXT_RANGES)]
        elif text_spec == "digits":
            args += ["--symbols", DIGIT_SYMBOLS]
    mdi_ranges = []
    if lv_symbols:
        mdi_ranges += [f"0x{src:X}=>0x{dst:X}" for dst, src in LV_SYMBOL_REMAP]
    if icon_set:
        for i, (macro, _mdi, cp) in enumerate(ICONS):
            if icon_set == "all" or macro in ICONS_LARGE:
                mdi_ranges.append(f"0x{cp:X}=>0x{icon_codepoint(i):X}")
    if mdi_ranges:
        args += ["--font", rel(TTF_DIR / MDI_TTF)]
        for r in mdi_ranges:
            args += ["-r", r]
    return args


def write_icons_header() -> None:
    lines = [
        "/**",
        " * @file ui_icons.h",
        " * Icon glyphs (Material Design Icons, Apache-2.0) mapped to the private use area.",
        " *",
        " * GENERATED by tools/fonts/generate_fonts.py - DO NOT EDIT.",
        " *",
        " * Icons are available in ui_font_12/14/16/20 (inline with text) and in",
        " * ui_icons_24. ui_icons_48 contains only: "
        + ", ".join(f"ICON_{m}" for m in ICONS_LARGE) + ".",
        " */",
        "#pragma once",
        "",
    ]
    width = max(len(m) for m, _, _ in ICONS) + 6
    for i, (macro, mdi, _cp) in enumerate(ICONS):
        cp = icon_codepoint(i)
        lines.append(f'#define {("ICON_" + macro).ljust(width)} "{utf8_escape(cp)}"  /* U+{cp:04X} mdi-{mdi} */')
    lines.append("")
    (OUT_DIR / "ui_icons.h").write_text("\n".join(lines), encoding="utf-8")


def write_fonts_header() -> None:
    lines = [
        "/**",
        " * @file ui_fonts.h",
        " * Font declarations.",
        " *",
        " * GENERATED by tools/fonts/generate_fonts.py - DO NOT EDIT.",
        " *",
        " * Text faces: Barlow Semi Condensed / Barlow Condensed (SIL OFL 1.1).",
        " * Icons: Material Design Icons (Apache-2.0).",
        " */",
        "#pragma once",
        "",
        "#include <lvgl.h>",
        "",
        "#ifdef __cplusplus",
        'extern "C" {',
        "#endif",
        "",
    ]
    for name, size, ttf, spec, _sym, icons in FONTS:
        what = []
        if ttf:
            what.append(f"{ttf.replace('.ttf', '')} {size}px" + (" digits" if spec == "digits" else ""))
        if icons:
            what.append("icons" if icons == "all" else "large icons")
        lines.append(f"LV_FONT_DECLARE({name})".ljust(34) + f"/* {', '.join(what)} */")
    lines += ["", "#ifdef __cplusplus", "}", "#endif", ""]
    (OUT_DIR / "ui_fonts.h").write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--lv-font-conv", help="path to lv_font_conv.js (default: npx lv_font_conv)")
    opts = parser.parse_args()

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    converter = find_converter(opts.lv_font_conv)

    for font in FONTS:
        name = font[0]
        print(f"  generating {name}.c")
        cmd = converter + build_args(*font)
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
        if result.returncode != 0:
            print(result.stdout, result.stderr, sep="\n")
            return 1

    write_icons_header()
    write_fonts_header()
    print(f"done: {len(FONTS)} fonts, {len(ICONS)} icons -> {OUT_DIR.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
