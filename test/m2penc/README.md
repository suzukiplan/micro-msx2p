# Playlog to MP4 movie encoder

- m2p 形式の動画ファイル（Playlog）を `H.264+AAC` の `MP4` にエンコードします
- 本プログラムの利用には `ffmpeg` コマンドが必要です

## How to Build

### Pre-requests

- CLANG C++
- GNU MAKE
- ffmpeg command

### Build

```
make
```

## Usage

### Using standalone (uncompressed AVI)

```
m2penc [-o /path/to/output.mp4]
       [-s /path/to/settings.json]
       [-w /path/to/workdir]
       /path/to/playlog.m2p
```

- `[-o /path/to/output.mp4]` を省略した場合、出力先は `/path/to/playlog.mp4` になります
- `[-s /path/to/settings.json]` を省略した場合、カレントパスの `settings.json` が読み込まれます
- `[-w /path/to/workdir]` を省略した場合、カレントパスの `.m2penc` がワークディレクトリとして作成されます

### Required Condition: `ffmpeg` command

本プログラムを動作させるとワークディレクトリ `.m2penc` 以下に1フレーム毎の画像データ（png形式）と音声データ（wav形式）が生成され、それらを `ffmpeg` で `H.264+AAC` の `MP4` にエンコードします。

`ffmpeg` は次のコーデックが利用できる状態にする必要があります:

- `libx264` H.264 コーデック
- `libfdk_aac` AAC コーデック

> __参考（macOSのffmpegコマンドで `libfdk_aac` を利用する手順）__
>
> macOS の brew でインストールした ffmpeg コマンドには標準では `libfdk_aac` が含まれていません。
> 以下の手順を参考にして `libfdk_aac` が利用できる `ffmpeg` をインストールしてください。
> [https://scrapbox.io/craftmemo/mac_で_ffmpeg_で_libfdk_aac_を使う](https://scrapbox.io/craftmemo/mac_%E3%81%A7_ffmpeg_%E3%81%A7_libfdk_aac_%E3%82%92%E4%BD%BF%E3%81%86)

### Settings: settings.json

- m2penc を利用するには BIOS ファイルやスロット構成を定義した設定ファイル `settings.json` が必要です
- m2p ファイルの録画を行った時と同じ BIOS 構成にしなければ、正常な動画出力がされないことがあります
- 具体的な書式例は [settings_cbios2p.json](settings_cbios2p.json) または [settings_fsa1wsx.json](settings_fsa1wsx.json) を参照してください

__（要素解説）__

- `font` フォントファイルの指定
- `slots` スロット0〜3の定義
  - `extra` 拡張スロットか否かを示す真偽値
  - `ram` RAMか否かを示す真偽値 (未指定または `false` なら ROM) 
  - `pages` ページ0〜3の定義 (※ `extra` が `false` の場合のみ定義)
    - `data` ROMデータのファイルパスまたは null (空バッファ)
    - `offset` ROMデータのオフセット ÷ 0x4000
      - 0: 0x0000, 1: 0x4000, 2: 0x8000, 3: 0xC000
      - 省略時は 0 を仮定
    - `label` ROMデータのラベルテキスト
  - `extraPages` 拡張スロット0〜3の `pages` を定義
- `name` 特に意味はありません（オブジェクトの位置情報を意味するコメント）

## License

本プログラム（[src/m2penc.cpp](src/m2penc.cpp)）のライセンスは Public Domain とします。

ただし、以下のソフトウェアに依存しているため、再配布時にはそれぞれのライセンス条項の遵守をお願いいたします。

- JSON for Modern C++
  - Web Site: [https://github.com/nlohmann/json](https://github.com/nlohmann/json)
  - License: [MIT](src/json/LICENSE.MIT)
  - `Copyright (c) 2013-2022 Niels Lohmann`
- PNGwriter
  - Web Site: [https://github.com/pngwriter/pngwriter](https://github.com/pngwriter/pngwriter)
  - License: [GNU General Public License](src/pngwriter/LICENSE)
  - `(C) 2002-2018 Paul Blackburn`
  - `(C) 2013-2018 Axel Huebl`
  - `(C) 2016-2018 Rene Widera`
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
