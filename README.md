# Vectrexia
![Build/Test Workflow](https://github.com/beardypig/vectrexia-emulator/actions/workflows/build-test.yml/badge.svg)

Vectrexia is a work in progress Vectrex emulator written as a `libretro` core. It is being developed as part of a series of blog posts about writing a [Vectrex Emulator](https://beardypig.github.io/2016/01/15/emulator-build-along-1/).


## Compilation

To compile this `libretro` core use `cmake` (3.21 or newer). Your C++ compiler must support the C++23 standard, eg. `clang >= 17` or `gcc >= 12` or `MSVC >= 17.5 `.

Dependencies are installed by [vcpkg](https://github.com/microsoft/vcpkg), which is included as a git submodule, so clone with submodules:

``` shell
$ git clone --recurse-submodules https://github.com/beardypig/vectrexia-emulator.git
```

If you already cloned without them, run `git submodule update --init`. vcpkg is bootstrapped and the dependencies are built automatically the first time you configure, which can take a while. On Linux it needs `curl`, `zip`, `unzip`, `tar` and `pkg-config` to be installed.

Pick a preset from `CMakePresets.json` and use it to configure, build and test. For example, with Ninja:

``` shell
$ cmake --preset ninja-debug
$ cmake --build --preset ninja-debug
$ ctest --preset ninja-test
```

| Configure/build preset | Test preset | Use for |
|---|---|---|
| `ninja-debug`, `ninja-release` | `ninja-test` (debug only) | Linux, macOS, or Windows from a Visual Studio Developer shell |
| `msvc2026-x64-debug`, `msvc2026-x64-release` | `msvc2026-x64-test` (debug only) | Windows with Visual Studio 2026 |
| `msvc-x64-debug`, `msvc-x64-release` | `msvc-x64-test` (debug only) | Windows with Visual Studio 2022 |

- The Visual Studio presets work from any terminal, and the generated solution can be opened in the IDE. On Windows the Ninja presets need `cl.exe` on the `PATH`, so use a Developer PowerShell or Developer Command Prompt.
- On Windows with a Visual Studio preset `ctest` needs the configuration, so run `ctest --preset msvc2026-x64-test -C Debug`.
- Each preset builds in its own directory under `build/`, so don't point two presets at the same one.

The libretro core is built as `vectrexia_libretro.so` (`.dylib` on macOS, `.dll` on Windows) in the `src` folder of the build directory, for example `build/ninja-debug/src/`. With the Visual Studio presets it is in a configuration subfolder, for example `build/vs2026-x64-Debug/src/Debug/`. Only the `retro_*` functions are exported. A small GIF tool, `vectgif`, is built alongside it.

## Authors
- @beardypig
- @pelorat
