# Performance Tester

## Description

最適化オプション `-Os` でビルドして、C-BIOS でエミュレータを起動してから1分間（3600フレーム）実行するのに要した時間を測定します。

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
