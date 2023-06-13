# MSX-BASIC Runner for CLI

- コマンドラインで MSX-BASIC の実行結果を確認できるツールです
- 本プログラムを動作させるには MSX2+ の BIOS ファイルが必要です
- だいたいの OS で問題なく動作する筈です

## Pre-requests

### UNIX, Linux, macOS

- CLANG C++
- GNU MAKE

### Windows

- Visual C++ Platform SDK (CLI)

## Setup BIOS files

カレントディレクトリに実機から吸い出した以下の BIOS ファイルを配置してください。

- `MSX2P.ROM` メイン BIOS
- `MSX2PEXT.ROM` サブ BIOS

また、必須ではないですが以下の BIOS を使用することもできます。

- `DISK.ROM` 東芝製 FDC のディスク BIOS (-dオプション利用時のみ)

> 吸い出した機種によってはこの手順では正常に動作しないことがあります。その場合、[こちら](https://github.com/suzukiplan/micro-msx2p#2-2-setup-slot) を参考にして必要な BIOS ファイルの追加と `setup` の修正をすることで動作する場合があります。（起動時に BASIC が起動せずに初期ユーティリティが動作する機種の場合、BASIC を起動するように操作処理を追加する必要があることもあります）

なお、環境変数 `RUNBAS_PATH` を指定することで、BIOSファイルの読み込み先ディレクトリを指定することができます。

```bash
export RUNBAS_PATH=/path/to/bios/dir
./runbas test.bas
```

## How to Build

### UNIX, Linux, macOS

```bash
make
```

### Windows

```bash
nmake /f Makefile.win
```

## How to Execute

```
usage: runbas [-f frames]
              [-e message]
              [-d /path/to/image.dsk]
              [-o /path/to/result.bmp]
              [/path/to/file.bas]
```

- `[-f frames]` ... 実行フレーム数
  - 省略時は `600` ≒ 10秒 を仮定
- `[-e message]` ... `message` の出力を検出したらコマンドを異常終了させる
  - 自動テスト（CI）などで利用する想定
- `[-d /path/to/image.dsk]` ... ディスクイメージを使用する
  - 本オプションで動作させた場合 `runbas.sav` の入出力は行われません
  - 本オプションで動作させるには DISK.ROM が必要です
  - DISK.ROM は東芝製 FDC (TC8566AF) のもののみ利用できます
  - 未対応の DISK.ROM (Philips 製 FDC など) を用いた場合 `Illegal function call` でファイル I/O 命令が失敗します
- `[-o /path/to/result.bmp]` ... 実行後のスクリーンショット（bmpファイル）の出力先
  - 省略時は `result.bmp` を仮定 
- `[/path/to/file.bas]` ... 実行する BASIC ファイル（※テキスト形式）
  - 省略時は[標準入力モード](#stdin-mode)で動作

実行が成功するとカレントディレクトリに次のファイルが生成されます。

- `runbas.sav` ... BASIC が起動完了時点のクイックセーブデータ
- `result.bmp` ... 指定した BASIC コード実行後の画面スクショ

なお、カレントディレクトリに `runbas.sav` が存在する環境では実行時間が（BASIC起動処理にかかる時間分）短縮されます。

> `runbas.sav` の入出力ディレクトリは環境変数 `RUNBAS_PATH` の指定に依存します。

### STDIN mode

- `runbas` を BASIC ファイルを指定せずに実行すると標準入力モードで動作します。
- 標準入力モードは次のいずれかの方法で抜けて実行することができます
  - control + d (`^+d`)
  - 空行を入力

## Example

以下のファイル（test.bas）を実行する例を示します。

```test.bas
10 ?"THIS IS TEST"
```

```text
% ./runbas test.bas
Waiting for launch MSX-BASIC...
Typing test.bas...
---------- START ----------
RUN' 
THIS IS TEST
Ok
----------- END -----------
Writing result.bmp...
%
```

|result.bmp|
|:-|
|![result.bmp](result_example.png)|

> `result.bmp` の出力先ディレクトリは環境変数 `RUNBAS_PATH` の指定による影響を受けず常にカレントディレクトリです。

## License

本プログラム（[runbas.cpp](runbas.cpp)）のライセンスは Public Domain とします。

ただし、以下のソフトウェアに依存しているため、再配布時にはそれぞれのライセンス条項の遵守をお願いいたします。

- LZ4 Library
  - Web Site: [https://github.com/lz4/lz4](https://github.com/lz4/lz4) - [lib](https://github.com/lz4/lz4/tree/dev/lib)
  - License: [2-Clause BSD](../../licenses-copy/lz4-library.txt)
  - `Copyright (c) 2011-2020, Yann Collet`
- emu2413
  - Web Site: [https://github.com/digital-sound-antiques/emu2413](https://github.com/digital-sound-antiques/emu2413)
  - License: [MIT](../../licenses-copy/emu2413.txt)
  - `Copyright (c) 2001-2019 Mitsutaka Okazaki`
- SUZUKI PLAN - Z80 Emulator
  - Web Site: [https://github.com/suzukiplan/z80](https://github.com/suzukiplan/z80)
  - License: [MIT](../../licenses-copy/z80.txt)
  - `Copyright (c) 2019 Yoji Suzuki.`
- micro MSX2+
  - Web Site: [https://github.com/suzukiplan/micro-msx2p](https://github.com/suzukiplan/micro-msx2p)
  - License: [MIT](../../LICENSE.txt)
  - `Copyright (c) 2023 Yoji Suzuki.`
