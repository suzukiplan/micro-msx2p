# micro MSX2+ for RaspberryPi Zero 2W (Bare Metal) - MSX1 core

RaspberryPi Zero 2W のベアメタル環境（OS無し）で動作する micro MSX2+ (MSX1 コア) の実装例です。

- RaspberryPi Zero, Zero W, Zero WH では動作しません
- 想定ターゲットは RaspberryPi Zero 2W ですが、RaspberryPi 3 でも動作する可能性があります
- 通常の Linux 環境（Raspbian や DietPi）で動作させたい場合は [SDL2版](../msx2-sdl2) を用いてください

## Prerequest

### Hardware

- RaspberryPi Zero 2W
- HDMI で映像と音声の出力に対応したディスプレイ + 接続ケーブル
- USB ゲームパッド + 接続ケーブル

> USB ゲームパッドについて:
>
> 本リポジトリの実装は [Elecom JC-U3312S](https://www2.elecom.co.jp/peripheral/gamepad/jc-u3312s/) で問題なく動作するキー割り当てになっています。お手持ちの USB ゲームパッドで適切に動作しない場合、[circle/sample/27-usbgamepad](https://github.com/rsta2/circle/tree/master/sample/27-usbgamepad) を動かして適切なキーコードを確認して、[kernel.cpp](kernel.cpp) の `CKernel::updateUsbStatus` の実装を修正してください。

### Software

- GNU Make
- GCC
- [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
  - `aarch64-none-elf` をダウンロード & インストールしてパスを切ってください
    - macOS: `/Applications/ArmGNUToolchain/13.2.Rel1/aarch64-none-elf/bin`
    - `13.2.Rel1` の箇所はダウンロードした最新版に適宜変更してください

## How to Build

```
# リポジトリを取得
git clone https://github.com/suzukiplan/micro-msx2p

# リポジトリのディレクトリへ移動
cd micro-msx2p/msx1-rpizero2

# ビルド
make
```

## How to Use

### Make Boot SD Card

以下のファイルを micro SD カード（FAT32フォーマット）のルートディレクトリに配置したもの準備してください。

- kernel8.img ([How to Build](#how-to-build) の手順で生成)
- [bootcode.bin](https://github.com/raspberrypi/firmware/blob/master/boot/bootcode.bin)
- [start.elf](https://github.com/raspberrypi/firmware/blob/master/boot/start.elf)

### Launch Sequence

1. RaspberryPi Zero の SD カードスロットに準備した SD カードを挿入
2. HDMI ケーブルで RaspberryPi Zero と 640x480 以上の解像度でリフレッシュレート 60Hz のモニタ（テレビ等）を接続
3. USB ケーブルで RaspberryPi Zero へ給電

> リフレッシュレートが 60Hz よりも速いモニタでは正常に動作しない可能性があります。

## Replace to your game ROM

- [./bios/game.rom](./bios/game.rom) を起動対象のゲーム ROM ファイルに置換
- メガROM の場合 [./kernel_run.cpp](./kernel_run.cpp) の `msx1.loadRom` の引数を修正
- `make`

## License

本プログラムのライセンスは [MIT](../LICENSE.txt) とします。

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
