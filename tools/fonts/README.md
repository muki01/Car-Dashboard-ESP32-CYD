# Fonts and icons

Every file in `CarCYD/src/ui/fonts/` is **generated** — do not edit them by hand.

```bash
python tools/fonts/generate_fonts.py
```

Requirement: Node.js (`npx`). The script downloads `lv_font_conv` 1.5.3 automatically; to use an installed copy
pass `--lv-font-conv <path>/lv_font_conv.js`.

## Single source of truth: `generate_fonts.py`

| Section | Content |
|---|---|
| `TEXT_RANGES` | ASCII + the extra Latin letters needed by the translations + ° ± · × – — • … |
| `LV_SYMBOL_REMAP` | LVGL's internal symbols (check, close, arrows, +/−, warning) redrawn from MDI |
| `ICONS` | Application icons, mapped to the Unicode private use area (U+E000…); generates `ui_icons.h` |
| `FONTS` | Fonts to generate: size, source TTF, glyph set, icon set |

Adding an icon: find its name in the [Pictogrammers MDI library](https://pictogrammers.com/library/mdi/), take the
code point from the `_variables.scss` of the `@mdi/font` package (or the "codepoint" shown on the website), append
it to the **end** of `ICONS` (existing code points must not change) and run the script.

Adding a language: extend `TEXT_RANGES` with the glyphs of the new alphabet, regenerate the fonts and add the
strings in `CarCYD/src/core/i18n.h`.

## Sources and licenses (`ttf/`)

| File | Source | License |
|---|---|---|
| `BarlowSemiCondensed-*.ttf`, `BarlowCondensed-*.ttf` | Google Fonts / The Barlow Project | SIL OFL 1.1 (`OFL-Barlow.txt`) |
| `materialdesignicons-webfont.ttf` | @mdi/font 7.4.47 (Pictogrammers) | Apache 2.0 (`LICENSE-MaterialDesignIcons.txt`) |
