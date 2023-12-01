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
           [-g { OpenGL .................. GPU: OpenGL <default>
               | Vulkan .................. GPU: Vulkan
               | Metal ................... GPU: Metal
               }]
           [-f] .......................... Full Screen Mode
```

> `-d` (ディスク) は C-BIOS ではできない点をご注意ください

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
