# Vectrexia
![Build/Test Workflow](https://github.com/beardypig/vectrexia-emulator/actions/workflows/build-test.yml/badge.svg)

Vectrexia is a work in progress Vectrex emulator written as a `libretro` core. It is being developed as part of a series of blog posts about writing a [Vectrex Emulator](https://beardypig.github.io/2016/01/15/emulator-build-along-1/).


## Compilation

To compile this `libretro` core use `cmake`. Your C++ compiler must support the C++23 standard, eg. `clang >= 17` or `gcc >= 12` or `MSVC >= 17.5 `.
 
``` shell
$ cmake --preset ninja-debug
$ cmake --build --preset ninja-debug
```

The tests can also be run with CTest
```shell
$ ctest --preset ninja-test
```

## Authors
- @beardypig
- @pelorat
