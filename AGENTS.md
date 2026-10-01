# Repository Guidelines

## Project Structure & Module Organization

`Bambu_Monitor_ESP32/` contains the Arduino sketch, shared printer status, hardware selection, and display renderers. `screen.h` implements the LilyGo grayscale layout; `screen_weact.h` implements the compact tri-color layout. Keep networking in the sketch and rendering independent of MQTT/FTP. `preview_weact.h` handles bounded thumbnail decoding. Fonts and their generator live in `tools/fonts/` and `tools/make_fonts.py`; documentation screenshots live in `img/`.

## Build, Test, and Development Commands

Use Arduino IDE with esp32 core **2.0.15** and the dependencies listed in README. Select the profile in `hardware_config.h`. LilyGo requires ESP32S3 Dev Module, OPI PSRAM, 16MB flash, and the 3MB APP/9.9MB FATFS partition. WeAct uses ESP32 Dev Module, PSRAM disabled, and Huge APP.

- `python3 tools/preview/render.py`: build and render LilyGo scenarios; requires gcc/g++, Pillow, numpy, and LilyGo-EPD47.
- `python3 tools/preview/render_weact.py`: exercise the WeAct decoder and render its actual layout; requires gcc/g++, Pillow, Adafruit GFX, and pngle.
- `python3 tools/make_fonts.py`: regenerate font headers using freetype-py.
- `python3 tools/printer_state.py`: query a real printer using paho-mqtt and local credentials.

## Coding Style & Naming Conventions

Use four-space indentation. Follow C++ camelCase functions/variables, PascalCase types, uppercase constants, and Python snake_case. No formatter or linter is configured. Regenerate font headers rather than editing them manually. Keep profile-specific hardware dependencies behind compile-time guards.

## Testing Guidelines

Compile both hardware profiles after shared firmware changes. Inspect desktop PNGs at native resolution for clipping, missing sensors, long names, and state transitions. The WeAct harness includes decoder assertions; there is no coverage threshold or general test framework. Hardware checks must cover refresh behavior, MQTT continuity, thumbnail failure, and job changes. Report hardware validation separately from host checks.

## Commit & Pull Request Guidelines

Use short, descriptive commit subjects, consistent with existing history such as “Status Codes.” Keep commits focused. PRs should explain behavior changes, validation performed, related issues when applicable, and include screenshots for layout changes.

## Security & Configuration

Copy `credentials.example.h` locally; never commit credentials, printer dumps, or generated preview output. Redact printer identifiers from shared logs. Keep thumbnail allocations bounded on the PSRAM-free WeAct target.
