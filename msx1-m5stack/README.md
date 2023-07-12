# micro MSX2+ for M5Stack <WIP>

- micro-msx2p を M5Stack Core2 での動作をサポートしたものです
- M5Stack の性能上の問題で MSX2/2+ の再現度の高いエミュレーションは難しいと判断したため MSX1 コアを使っています
  - _ただし、それでもまだ性能面での問題を完全にはクリアできていません_

## WIP status

- 性能目標が未達成: https://github.com/suzukiplan/micro-msx2p/issues/17
  - 目標: tick 2 frames < 33ms
  - 現状: tick 2 frames = 41ms (min:34, max:52)
- 音声未実装: https://github.com/suzukiplan/micro-msx2p/issues/18
  - 現状、音が鳴りません
- 入力未対応:  https://github.com/suzukiplan/micro-msx2p/issues/19
  - M5Stack Faces GamePad panel に対応予定
- ホットキー未実装: https://github.com/suzukiplan/micro-msx2p/issues/20
  - 再開
  - リセット
  - 音量調整 (無音, 小, 中, 大)
  - バックライト調整 (暗い, 標準, 明るい)
  - セーブスロット選択 (#1, #2, #3)
  - セーブ (ショートカット1と等価)
  - ロード (ショートカット2と等価)
- ショートカットキー未実装: https://github.com/suzukiplan/micro-msx2p/issues/21

補足事項:

- 性能目標は C-BIOS で `Hello World!` を表示するシンプルな ROM を動かした時の性能（最終的にもっと画面表示や音声出力する ROM でもテスト予定）
- M5Stack には 3 つのシステムボタンがあり、ホットキー、ショートカット(1,2) として割り当てる想定

## Pre-requests

- [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/index.html)
  - macOS: `brew install platformio`
- GNU Make

> Playform I/O は Visual Studio Code 経由で用いる方式が一般的には多いですが、本プロジェクトでは CLI (pioコマンド) のみ用いるので Visual Studio Code や Plugin のインストールは不要です。

## Build Support OS

- macOS
- Linux

> ビルド以外にも色々な事前処理を行っており（詳細は [Makefile](Makefile) を参照）、その関係で基本的に **macOS または Linux 系 OS でのコマンドラインビルドのみサポート** します。Windows で試したい場合は WSL2 等をご利用ください。

### Build and Upload Firmware

```
make
```

上記コマンドを実行すると次の手続きを実行します:

1. MSX1 エミュレータコアモジュール [src1](../src1) を [include](include) に展開
2. [bios](bios) 以下の ROM ファイルから [src](src) 以下に `rom_xxx.c` を自動生成
3. `src/rom_xxx.c` から [include](include) 以下に `roms.hpp` を自動生成
4. `pio run -t upload` で M5Stack 向けのビルドとアップロード（ファームウェア書き込み）

### Build only

```
make build
```

> ステップ 4 の手続きが `pio run -t upload` → `pio run` に変わります。

## Replace ROM file

1. [bios/game.rom](bios/game.rom) を更新
2. `src/rom_game.c` を削除
3. メガロムの場合 [src/app.cpp](src/app.cpp) の `setup` 関数で実行している `loadRom` の `MSX1_ROM_TYPE_` を対象のメガロム種別に変更
