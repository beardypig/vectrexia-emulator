# Vectrexia

Vectrex emulator written in C++23, built as a `libretro` core. CMake + vcpkg (git submodule in `vcpkg/`). CI builds on Linux, macOS and Windows, so keep code portable.

## Layout

- `src/` - emulator core (`m6809`, `via6522`, `ay38910`, vector rendering) and `src/libretro/`
- `tests/` - Catch2 unit tests (Catch2, trompeloeil, fmt, cxxopts come from `vcpkg.json`)
- `vectgif/` - GIF helper tool

## Build and test

All commands are run from the repo root using presets from `CMakePresets.json`.

Visual Studio 2026 (Windows, no special shell needed, also what CI uses):

```shell
cmake --preset msvc2026-x64-debug
cmake --build --preset msvc2026-x64-debug
ctest --preset msvc2026-x64-test -C Debug
```

Visual Studio 2022 (Windows): same commands with the `msvc-x64-debug` / `msvc-x64-test` presets. CI does not use these, because GitHub's `windows-latest` image no longer has Visual Studio 2022.

Ninja (Linux, macOS, or Windows):

```shell
cmake --preset ninja-debug
cmake --build --preset ninja-debug
ctest --preset ninja-test
```

Notes:

- On Windows, `cmake` may not be on PATH in a plain terminal. Use a Visual Studio Developer PowerShell (View > Terminal in VS). The Ninja presets require that environment because they need `cl.exe`; the Visual Studio presets do not.
- Each preset has its own build directory under `build/`. Do not reuse one directory across different generators.
- The first configure is slow (vcpkg installs dependencies).
- Always run the tests after changing code under `src/`. Expect all 32 tests to pass.

## Conventions

- Keep the build warning-clean where practical. Existing known warnings: narrowing in `src/m6809.h`, `strtok` in `src/libretro/libretro.cpp`, shift in `vectgif/gif.h`.
- Do not edit anything under `vcpkg/` (submodule).
