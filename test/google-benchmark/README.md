# Performance Tester for MSX1 with Google-Benchmark

## Description

Google Benchmark を用いた性能評価を行います

## How to Use

```bash
git submodule init benchmark
make
```

> _初回ビルド時は Google Benchmark のビルドに時間がかかります_

## Result

実行が成功すれば result1〜3.txt に結果が記録されています。

```
2023-07-14T09:16:50+09:00
Running ./test
Run on (8 X 1200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB
  L1 Instruction 32 KiB
  L2 Unified 512 KiB (x4)
  L3 Unified 8192 KiB
Load Average: 3.99, 3.19, 3.06
-------------------------------------------------------------
Benchmark                   Time             CPU   Iterations
-------------------------------------------------------------
MSX1Init               123164 ns       122503 ns         5875
MSX1Execute1Tick       703478 ns       700244 ns          921
MSX1Execute60Ticks   43171058 ns     43118875 ns           16
```

実行の都度結果が変わるので、念のため3回実行するようにしています。

## License

本プログラム（[test.cpp](test.cpp)）のライセンスは [MIT](LICENSE.txt) とします。

また、本プログラムには以下のソフトウェアに依存しているため、再配布時にはそれぞれのライセンス条項の遵守をお願いいたします。

- Benchmark
  - Web Site: [https://github.com/google/benchmark](https://github.com/google/benchmark)
  - License: [Apache License, Version 2.0](../../licenses-copy/benchmark.txt)
- SUZUKI PLAN - Z80 Emulator
  - Web Site: [https://github.com/suzukiplan/z80](https://github.com/suzukiplan/z80)
  - License: [MIT](../../licenses-copy/z80.txt)
  - `Copyright (c) 2019 Yoji Suzuki.`
- micro MSX2+
  - Web Site: [https://github.com/suzukiplan/micro-msx2p](https://github.com/suzukiplan/micro-msx2p)
  - License: [MIT](../../LICENSE.txt)
  - `Copyright (c) 2023 Yoji Suzuki.`
