# micro MSX2+ for SDL2

- SDL2 を用いた micro MSX2+ (MSX2+ コア) の実装例です
- 現状、macOS と Linux のみサポートしています

## How to Build

### macOS

```
# SDL2 をインストール
brew install sdl2

# リポジトリを取得
git clone https://github.com/suzukiplan/micro-msx2p

# リポジトリのディレクトリへ移動
cd micro-msx2p/msx2-sdl2

# ビルド
make

# 実行
./app
```

### Linux (Ubuntu)

```
# SDL2 をインストール
sudo apt-get install libsdl2-dev

# ALSA をインストール
sudo apt-get install libasound2
sudo apt-get install libasound2-dev

# リポジトリを取得
git clone https://github.com/suzukiplan/micro-msx2p

# リポジトリのディレクトリへ移動
cd micro-msx2p/msx2-sdl2

# ビルド
make

# 実行
./app
```

## Usage

### Boot Option

```
usage: app [/path/to/file.rom] ........... Use ROM
           [-t { normal .................. 16KB/32KB <default>
               | asc8 .................... MegaRom: ASCII-8
               | asc16 ................... MegaRom: ASCII-16
               | asc8+sram2 .............. MegaRom: ASCII-8 + SRAM2
               | asc16+sram2 ............. MegaRom: ASCII-16 + SRAM2
               | konami .................. MegaRom: KONAMI
               | konami+scc .............. MegaRom: KONAMI+SCC
               }]
           [-d /path/to/disk*.dsk ...] ... Use Floppy Disk(s) *Max 9 disks
           [-g { None .................... GPU: Do not use
               | OpenGL .................. GPU: OpenGL <default>
               | Vulkan .................. GPU: Vulkan
               | Metal ................... GPU: Metal
               }]
           [-f] .......................... Full Screen Mode
```

> `-d` (ディスク) は C-BIOS ではできない点をご注意ください

### About GPU Option

- `-g` オプション（GPU 描画）はフルスクリーンモード (`-f`) の時にのみ有効です
- ウィンドウモード（`-f` 未指定）の場合、常に Surface 描画（CPU 描画）になります

### Keyboard Assign

キーボードの割当は Mac のキーボードを仮定しています。

> 将来的に 109 キーボード等にも対応するかもしれません。

### Hot Key

|Hot Key|Description|
|:-:|:-|
|⌘+1~9|ディスク交換|
|⌘+0|ディスクを排出|
|⌘+R|リセット|
|⌘+Q|終了|

## License

本プログラムには次の OSS が含まれています。利用に当たっては、著作権（財産権）及び著作者人格権は各作者に帰属する点の理解と、ライセンス条項の厳守をお願いいたします。

- LZ4 Library
  - Web Site: [https://github.com/lz4/lz4](https://github.com/lz4/lz4) - [lib](https://github.com/lz4/lz4/tree/dev/lib)
  - License: [2-Clause BSD](../licenses-copy/lz4-library.txt)
  - `Copyright (c) 2011-2020, Yann Collet`
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
- emu2413
  - Web Site: [https://github.com/digital-sound-antiques/emu2413](https://github.com/digital-sound-antiques/emu2413)
  - License: [MIT](../licenses-copy/emu2413.txt)
  - `Copyright (c) 2001-2019 Mitsutaka Okazaki`
- SUZUKI PLAN - Z80 Emulator
  - Web Site: [https://github.com/suzukiplan/z80](https://github.com/suzukiplan/z80)
  - License: [MIT](../licenses-copy/z80.txt)
  - `Copyright (c) 2019 Yoji Suzuki.`
- micro MSX2+
  - Web Site: [https://github.com/suzukiplan/micro-msx2p](https://github.com/suzukiplan/micro-msx2p)
  - License: [MIT](../LICENSE.txt)
  - `Copyright (c) 2023 Yoji Suzuki.`
