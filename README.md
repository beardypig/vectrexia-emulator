# Vectrexia

[![Build Status](https://travis-ci.org/beardypig/vectrexia.svg?branch=master)](https://travis-ci.org/beardypig/vectrexia)

Vectrexia is a work in progress Vectrex emulator written as a `libretro` core. It is being developed as part of a series of blog posts about writing a [Vectrex Emulator](https://beardypig.github.io/2016/01/15/emulator-build-along-1/).


## Compilation

To compile this `libretro` core use `cmake`. Your C++ compiler must support the C++14/1y standard, eg. `clang >= 3.6` or `gcc >= 4.9`.
 
``` shell
$ cmake -E make_directory build && cmake -E chdir build cmake .. 
$ cmake --build build -- all test # compile and run the tests
```

## Authors
- Beardypig
