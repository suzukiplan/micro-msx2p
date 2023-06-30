# micro MSX2+ for .NET Core (WIP)

micro MSX2+ を .NET Core で利用できるクラス形式にしたものです。


## WIP status

（.NETなのに）まだ Windows には対応できてません。

また、最終的に **.NET Framework (Windows Desktop) や Unity** で micro-msx2p を扱いやすい形にする（ex: NuGetで配信したりインタフェースを整えたりする）ことを設計目標としていますが、いかんせん .NET Framework も Unity も使ったことがなく、とりあえず .NET Core でビルドして動かせたぞ...という状態です。

つまり、破壊的変更を含む修正がまだガリガリ入る可能性があります。

## Pre-requests

### Linux, macOS

- .NET Core 7.0
- GNU Make
- CLANG

### Windows

_TODO: まだ対応していない_

## How to Build

### Linux, macOS

```
git clone https://github.com/suzukiplan/micro-msx2p.git
cd micro-msx2p/msx2-dotnet
make
```

make を実行すると以下の順番でビルドが実行されます。

1. [DLL](DLL) ... アンマネージドコードの DLL モジュール `libmsx2.so` (`msx2.dll`) を作成
2. [MSX2Core](MSX2Core) ... `libmsx2.so` (`msx2.dll`) を用いた C# ラッパークラス `MSX2.Core`
3. [Test](Test) ... `MSX2.Core` を用いたテストモジュール（Hello, World）

[Test](Test) プロジェクトでは `MSX2.Core` で単純に Hello World を表示するシンプルな ROM を動かし、起動から 600 フレーム後のスクリーンショットを .bmp 形式で出力します。

```
% cd Test && dotnet run
Create micro-msx2p context
SetupSecondaryExist: 0 0 0 1
Setup: MAIN 0-0 $0000~$7FFF (32768 bytes)
Setup: LOGO 0-0 $8000~$BFFF (16384 bytes)
Setup: SUB  3-0 $0000~$3FFF (16384 bytes)
Setup: RAM  3-3 $0000~$FFFF
Setup: Special Key Code (Select=ESC, Start=SPACE)
Load ROM (32768 bytes, type=Normal)
Reset
Tick 600 times
Get Display (568x240)
Convert display to Bitmap
Writing result.bmp
Release micro-msx2p context
%
```

![Test/result.bmp](Test/result.png)

### Windows

_TODO: まだ対応していない_

## How to Use

[MSX2.Coreクラス](MSX2Core/MSX2Core.cs) が micro-msx2p コアモジュールと概ね同じ仕様で利用できます。
