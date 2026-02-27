# Order Book V1

See [00\_COURSEWORK.md](./docs/00_COURSEWORK.md) for the project specs. Inspired by
[alissawu/miniex](https://github.com/alissawu/miniex).

This project uses the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) and targets C++20.

## Build, Test, and Run

```bash
$ # Replace "Debug" with "Release" for the release build
$ cmake -S . -B build/Debug -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug
$ cmake --build build/Debug
$ ctest --test-dir build/Debug # Run tests (add --verbose for more detail)
$ ./clob_cli # Simulation/Replay tool
```

## Run Benchmarks

Initial performance considerations and optimizations are documented in [02_PERFORMANCE.md](./docs/02_PERFORMANCE.md).

```bash
$ cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=1
$ cmake --build build/Release && ./build/Release/orderbook_benchmark
```
