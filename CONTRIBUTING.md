# Contributing to CarCYD

Thanks for your interest in improving CarCYD! Bug reports, ideas, translations, new gauge styles, board variants
and code are all welcome.

## Ways to contribute

- **Report a bug** — open an issue with the bug report template (board variant, versions, serial log).
- **Suggest a feature** — open a feature request and describe the use case.
- **Translate** — add a language (see *Adding a language* below).
- **Code** — pick an open issue or an item from the roadmap in the README. For larger changes, please open an
  issue first so we can agree on the approach.

## Development setup

1. Install the Arduino IDE 2.x (or arduino-cli), the **esp32** core 3.x, **lvgl** 9.6+ and **LovyanGFX** 1.2+.
2. Open `CarCYD/CarCYD.ino` and select your board variant in `CarCYD/src/config/board_config.h`.
3. Build with all warnings enabled — the project compiles without warnings and should stay that way:
   ```bash
   arduino-cli compile --fqbn esp32:esp32:esp32 --warnings all CarCYD
   ```
4. For UI work, use the desktop simulator (no board needed):
   ```bash
   python tools/simulator/build.py --zig /path/to/zig
   ```

## Architecture rules

- `core/` must stay platform independent (no `Arduino.h`, no LVGL).
- `ui/` talks to hardware only through `core/platform.h`.
- `hal/` contains drivers only; business rules belong in `core/`.
- Visual constants come from `ui/theme.h` — do not hard-code colours or sizes in components.
- Persisted structures (`Settings`, `TripData`) are extended by **appending** fields only.

See [docs/architecture.md](docs/architecture.md) and [docs/ui-guidelines.md](docs/ui-guidelines.md).

## Coding style

- C++17 as used by the ESP32 Arduino core, formatted with the repository's `.clang-format` (Google based,
  120 columns).
- Types `PascalCase`, functions and variables `snake_case`, constants `kPascalCase`, members end with `_`.
- Comments explain *why*; keep them short. Public headers start with a `@file` block describing the module.
- Never edit generated files (`src/ui/fonts/*`) — change `tools/fonts/generate_fonts.py` and regenerate.

## Adding a language

1. Add a value to `core::Language` (`core/types.h`) and a column to `I18N_STRINGS` (`core/i18n.h`).
2. Return the native language name in `i18n::language_name()` and add it to the language picker.
3. If the alphabet needs new glyphs, extend `TEXT_RANGES` in `tools/fonts/generate_fonts.py` and regenerate.
4. Add translated DTC descriptions in `core/dtc.cpp` if you can (optional).

## Pull requests

- Keep pull requests focused; one topic per PR.
- Use [Conventional Commits](https://www.conventionalcommits.org/) style messages, e.g.
  `feat(ui): add boost gauge`, `fix(touch): ...`, `docs: ...`.
- Update the documentation when behaviour changes.
- UI changes: attach simulator screenshots (before / after).
- Hardware changes: mention the board variant you tested on.

By contributing, you agree that your contributions are licensed under the
[GNU General Public License v3.0](LICENSE), that the author may also offer them under a commercial license, and
that you follow the [Code of Conduct](CODE_OF_CONDUCT.md).
