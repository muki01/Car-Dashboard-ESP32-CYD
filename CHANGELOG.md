# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/) and the project uses [Semantic Versioning](https://semver.org/).

## [Unreleased]

## [1.1.0] - 2026-10-01

### Changed
- English is the default UI language on a fresh device (Turkish stays available in
  *Settings → System → Language*; stored settings are kept).
- All project documentation is in English.

### Added
- Animated demo (`docs/media/demo.gif`) and hero / social preview image (`docs/media/banner.png`),
  both rendered from the real UI by the simulator (`build.py --demo --banner`).
- GitHub Actions workflow compiling the firmware for all three panel variants.
- Ready-to-flash firmware images for all three board variants attached to each GitHub release.
- Contribution guide, code of conduct, security policy, issue and pull request templates, MIT license.

## [1.0.1] - 2026-09-30

### Fixed
- Mirrored picture with `CYD_PANEL_ILI9341_2` ("CYD2USB" boards): its init sequence clears the GS bit
  (B6h), which is now compensated by the panel rotation offset (landscape MADCTL = MV). The touch
  rotation offset is derived from the panel offset, so touch input keeps working for any orientation.

### Added
- `CYD_PANEL_ROTATION_OFFSET` in `board_config.h` to override the panel orientation (4-7 = mirrored).
- The stored touch calibration is bound to the panel type / orientation; changing them in
  `board_config.h` requests a new calibration automatically at the next start.

## [1.0.0] - 2026-09-30

### Added
- Dashboard with 270° tachometer ring, digital speed, gear / up-shift indicator, red zone and start-up
  needle sweep; four configurable corner values and a second view with six mini gauges.
- Live data list of 18 signals with severity colouring and a detail view (chart, min / avg / max).
- Diagnostics: check engine light status, reading and clearing of DTCs (SAE J2012), descriptions for
  92 common codes in Turkish and English, severity and advice.
- Alert manager (coolant, oil, battery low/high, fuel, speed limit, check engine, link lost) with
  debounce, hysteresis, banner, sound and status LED.
- Trip computer with persistent distance, drive time, speeds, fuel, consumption and 0-100 timer.
- Settings: display (auto brightness, rotation, accent colour), units, gauges, warnings, sound & LED,
  data source, language (TR/EN), touch calibration, restart, factory reset, about.
- Hardware support for the ESP32-2432S028R: DMA display driver, software-SPI touch with calibration,
  PWM backlight with ambient light sensor, speaker tones, RGB LED, BOOT button, NVS storage.
- Demo data source simulating a driving car.
- Desktop simulator producing screenshots of every screen; font/icon generator.

[Unreleased]: https://github.com/muki01/Car-Dashboard-ESP32-CYD/compare/v1.1.0...HEAD
[1.1.0]: https://github.com/muki01/Car-Dashboard-ESP32-CYD/releases/tag/v1.1.0
