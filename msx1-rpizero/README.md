# [WIP] micro MSX2+ for RaspberryPi Zero (Bare Metal)

RaspberryPi Zero シリーズ全般（無印、W、WH、2W）のベアメタル環境（OS無し）で動作する micro MSX2+ (MSX1 コア) の実装例です。

> 想定ターゲットは RaspberryPi Zero ですが、RaspberryPi 2〜4 あたりでも動作する可能性があります。
> 通常の Linux 環境で動作させたい場合は [SDL2版](../msx2-sdl2) を用いてください。

## WIP Status

- [x] 映像出力（HDMI）
- [x] micro MSX2+ (MSX1 core) 組み込み
- [x] 音声出力 (HDMI)
- [ ] USBキーボード入力
- [ ] USBゲームパッド入力

## Prerequest

- GNU Make
- [GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/gnu-rm)

## How to Build

```
# リポジトリを取得
git clone https://github.com/suzukiplan/micro-msx2p

# リポジトリのディレクトリへ移動
cd micro-msx2p/msx2-rpizero

# ビルド
make
```

## How to Use

### Make Boot SD Card

以下のファイルを micro SD カード（FAT32フォーマット）のルートディレクトリに配置したもの準備してください。

- kernel.img ([How to Build](#how-to-build) の手順で生成)
- [bootcode.bin](https://github.com/raspberrypi/firmware/blob/master/boot/bootcode.bin)
- [start.elf](https://github.com/raspberrypi/firmware/blob/master/boot/start.elf)

### Launch Sequence

1. RaspberryPi Zero の SD カードスロットに準備した SD カードを挿入
2. HDMI ケーブルで RaspberryPi Zero と 640x480 以上の解像度のモニタ（テレビ等）を接続
3. USB ケーブルで RaspberryPi Zero へ給電

## License

本プログラムのライセンスは [MIT](LICENSE.txt) とします。

また、本プログラムには以下のソフトウェアに依存しているため、再配布時にはそれぞれのライセンス条項の遵守をお願いいたします。

- C-BIOS
  - Web Site: [https://cbios.sourceforge.net/](https://cbios.sourceforge.net/)
  - License: [2-Clause BSD](../licenses-copy/cbios.txt)
  - `Copyright (c) 2002-2005 BouKiCHi.  All rights reserved.`
  - `Copyright (c) 2003 Reikan.  All rights reserved.`
  - `Copyright (c) 2004-2006,2008-2010 Maarten ter Huurne.  All rights reserved.`
  - `Copyright (c) 2004-2006,2008-2011 Albert Beevendorp.  All rights reserved.`
  - `Copyright (c) 2004-2005 Patrick van Arkel.  All rights reserved.`
  - `Copyright (c) 2004,2010-2011 Manuel Bilderbeek.  All rights reserved.`
  - `Copyright (c) 2004-2006 Joost Yervante Damad.  All rights reserved.`
  - `Copyright (c) 2004-2006 Jussi Pitkänen.  All rights reserved.`
  - `Copyright (c) 2004-2007 Eric Boon.  All rights reserved.`
- Circle
    - Web Site:
      - [https://circle-rpi.readthedocs.io/](https://circle-rpi.readthedocs.io/)
      - [https://github.com/rsta2/circle](https://github.com/rsta2/circle)
    - License: [GPLv3](../licenses-copy/circle.txt)
- SUZUKI PLAN - Z80 Emulator
  - Web Site: [https://github.com/suzukiplan/z80](https://github.com/suzukiplan/z80)
  - License: [MIT](../licenses-copy/z80.txt)
  - `Copyright (c) 2019 Yoji Suzuki.`
- micro MSX2+
  - Web Site: [https://github.com/suzukiplan/micro-msx2p](https://github.com/suzukiplan/micro-msx2p)
  - License: [MIT](../LICENSE.txt)
  - `Copyright (c) 2023 Yoji Suzuki.`
