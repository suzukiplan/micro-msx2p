# [WIP] micro MSX2+ for RaspberryPi Zero (Bare Metal)

- RaspberryPi Zero シリーズ全般（無印、W、2W）のベアメタル環境（OS無し）で動作する micro MSX2+ (MSX2+ コア) の実装例です
- 通常の Linux 環境で動作させたい場合は [SDL2版](../msx2-sdl2) を用いてください

## WIP Status

- [x] 映像出力（HDMI）
- [ ] micro MSX2+ 組み込み
- [ ] 音声出力 (HDMI)

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

- Circle
    - Web Site: [https://github.com/rsta2/circle](https://github.com/rsta2/circle)
    - License: [GPLv3](../licenses-copy/circle.txt)
- SUZUKI PLAN - Z80 Emulator
  - Web Site: [https://github.com/suzukiplan/z80](https://github.com/suzukiplan/z80)
  - License: [MIT](../licenses-copy/z80.txt)
  - `Copyright (c) 2019 Yoji Suzuki.`
- micro MSX2+
  - Web Site: [https://github.com/suzukiplan/micro-msx2p](https://github.com/suzukiplan/micro-msx2p)
  - License: [MIT](../LICENSE.txt)
  - `Copyright (c) 2023 Yoji Suzuki.`
