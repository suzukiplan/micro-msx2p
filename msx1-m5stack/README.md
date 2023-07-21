# micro MSX2+ for M5Stack

- micro-msx2p を M5Stack での動作をサポートしたものです
- M5Stack の性能上の都合で MSX2/2+ の再現度の高いエミュレーションは難しいと判断したため MSX1 コアを使っています
  - ただし、それでもまだ性能面での問題を完全にはクリアできていません
  - 現状、実機より 17%〜20% 程度遅く (48〜50fps程度で) 動作します
  - 性能目標: https://github.com/suzukiplan/micro-msx2p/issues/17

## Pre-requests

### M5Stack Hardware

- M5Stack CoreS3
  - 本当は Core2 に対応予定でしたが検証過程で壊れたので CoreS3 のみテストしてます
  - [platform.ini](platform.ini) の `default_envs` を `m5stack-core2` にすれば一応 Core2 でも動くかもしれません（テストしてません）
- M5 Face II
  - ゲームの操作を行うには M5 Face II (※Faces II ではなく MSX0 付属の Bottom Base) が必要です

> M5Stamp Pico と M5Stamp S3 への追加対応を検討中です

### Build Support OS

- macOS
- Linux

> ビルド以外にも色々な事前処理を行っており（詳細は [Makefile](Makefile) を参照）、その関係で基本的に **macOS または Linux 系 OS でのコマンドラインビルドのみサポート** します。Windows で試したい場合は WSL2 等をご利用ください。

### Middleware

- [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/index.html)
  - macOS: `brew install platformio`
- GNU Make

> Playform I/O は Visual Studio Code 経由で用いる方式が一般的には多いですが、本プロジェクトでは CLI (pioコマンド) のみ用いるので Visual Studio Code や Plugin のインストールは不要です。

## How to Build

### initialize SPIFFS

初期状態の M5Stack は SPIFFS（本体内部ストレージを使用したファイルシステム）が利用できない状態のため、設定状態やセーブ＆ロードが常に失敗します。

そこで、以下のコマンドを最初に1回実行してください。

```
make init
```

> 上記を実行すると、空の data ディレクトリを作成してそれを `pio run -t uploadfs` で本体にアップロードします。無意味なようですが、これをやらなければ SPIFFS が利用できません。

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
3. メガロムの場合 [src/app.cpp](src/app.cpp) の `setup` 関数で実行している `MSX1::loadRom` の `MSX1_ROM_TYPE_` を対象のメガロム種別に変更

## License

本プログラムのライセンスは [MIT](LICENSE.txt) とします。

また、本プログラムには以下のソフトウェアに依存しているため、再配布時にはそれぞれのライセンス条項の遵守をお願いいたします。

- M5GFX
  - Web Site: [https://github.com/m5stack/M5GFX](https://github.com/m5stack/M5GFX)
  - License: [MIT](../licenses-copy/M5GFX.txt)
  - `Copyright (c) 2021 M5Stack`
  - M5GFX には次のライセンスが含まれます
    - LovyanGFX
      - Web Site: [https://github.com/lovyan03/LovyanGFX](https://github.com/lovyan03/LovyanGFX)
      - License: [FreeBSD](../licenses-copy/LovyanGFX.txt)
      - `Copyright (c) 2012 Adafruit Industries.  All rights reserved.`
      - `Copyright (c) 2020 Bodmer (https://github.com/Bodmer)`
      - `Copyright (c) 2020 lovyan03 (https://github.com/lovyan03)`
    - TJpgDec
      - Web Site: [http://elm-chan.org/fsw/tjpgd/00index.html](http://elm-chan.org/fsw/tjpgd/00index.html)
      - License: [original](../licenses-copy/TJpgDec.txt)
      - `(C)ChaN, 2019`
    - Pngle
      - Web Site: [https://github.com/kikuchan/pngle](https://github.com/kikuchan/pngle)
      - License: [MIT](../licenses-copy/Pngle.txt)
      - `Copyright (c) 2019 kikuchan`
    - QRCode
      - Web Site: [https://github.com/ricmoo/QRCode](https://github.com/ricmoo/QRCode)
      - License: [MIT](../licenses-copy/QRCode.txt)
      - `Copyright (c) 2017 Richard Moore     (https://github.com/ricmoo/QRCode)`
      - `Copyright (c) 2017 Project Nayuki    (https://www.nayuki.io/page/qr-code-generator-library)`
    - result
      - Web Site: [https://github.com/bitwizeshift/result](https://github.com/bitwizeshift/result)
      - License: [MIT](../licenses-copy/result.txt)
      - `Copyright (c) 2017-2021 Matthew Rodusek`
    - GFX font and GLCD font
      - Web Site: [https://github.com/adafruit/Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library)
      - License: [2-clause BSD](../licenses-copy/Adafruit-GFX-Library.txt)
      - `Copyright (c) 2012 Adafruit Industries.  All rights reserved.`
    - Font 2,4,6,7,8
      - Web Site: [https://github.com/Bodmer/TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
      - License: [FreeBSD](../licenses-copy/TFT_eSPI.txt)
      - `Copyright (c) 2012 Adafruit Industries.  All rights reserved.`
      - `Copyright (c) 2023 Bodmer (https://github.com/Bodmer)`
    - converted IPA font
      - Web Site: [https://www.ipa.go.jp/index.html](https://www.ipa.go.jp/index.html)
      - License: [IPA Font License Agreement v1.0](../licenses-copy/IPA_Font_License_Agreement_v1.0.txt)
    - efont
      - Web Site: [http://openlab.ring.gr.jp/efont/](http://openlab.ring.gr.jp/efont/)
      - License: [3-clause BSD](../licenses-copy/efont.txt)
      - `(c) Copyright 2000-2001 /efont/ The Electronic Font Open Laboratory. All rights reserved.`
    - TomThumb font
      - Web Site:
      - License: [3-clause BSD](../licenses-copy/TomThumb.txt)
      - `Copyright 1999 Brian J. Swetland`
      - `Copyright 1999 Vassilii Khachaturov`
      - `Portions (of vt100.c/vt100.h) copyright Dan Marks`
- M5Unified
  - Web Site: [https://github.com/m5stack/M5Unified](https://github.com/m5stack/M5Unified)
  - License: [MIT](../licenses-copy/M5Unified.txt)
  - `Copyright (c) 2021 M5Stack`
- SUZUKI PLAN - Z80 Emulator
  - Web Site: [https://github.com/suzukiplan/z80](https://github.com/suzukiplan/z80)
  - License: [MIT](../licenses-copy/z80.txt)
  - `Copyright (c) 2019 Yoji Suzuki.`
- micro MSX2+
  - Web Site: [https://github.com/suzukiplan/micro-msx2p](https://github.com/suzukiplan/micro-msx2p)
  - License: [MIT](../LICENSE.txt)
  - `Copyright (c) 2023 Yoji Suzuki.`
