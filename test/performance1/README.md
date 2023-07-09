# Performance Tester for MSX1

## Description

最適化オプション `-Os` でビルドして、C-BIOS でエミュレータ（MSX1）を起動してから1分間（3600フレーム）実行するのに要した時間を測定します。

## How to Use

```bash
% make
clang -Os -c ../../msx2-osx/core/emu2413.c
clang++ -Os -std=c++11 -o test test.cpp emu2413.o
./test
Total time: 12124ms
Frame average: 3.367778ms
Frame usage: 20.21%
```

- `Total time` : 3600フレームの実行に要した時間
- `Frame average` : 1フレームの実行に要した平均時間
- `Frame usage` : 60Hzでの垂直同期を入れた1フレームの時間（1000÷60ms）に対してコア実行に要する時間の比率（想定CPU使用率）

## License

本プログラム（[test.cpp](test.cpp)）のライセンスは [MIT](LICENSE.txt) とします。

また、本プログラムには以下のソフトウェアに依存しているため、再配布時にはそれぞれのライセンス条項の遵守をお願いいたします。

- SUZUKI PLAN - Z80 Emulator
  - Web Site: [https://github.com/suzukiplan/z80](https://github.com/suzukiplan/z80)
  - License: [MIT](../../licenses-copy/z80.txt)
  - `Copyright (c) 2019 Yoji Suzuki.`
- micro MSX2+
  - Web Site: [https://github.com/suzukiplan/micro-msx2p](https://github.com/suzukiplan/micro-msx2p)
  - License: [MIT](../../LICENSE.txt)
  - `Copyright (c) 2023 Yoji Suzuki.`
