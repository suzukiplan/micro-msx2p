# Playlog to MP4 movie encoder

- m2p 形式の動画ファイル（Playlog）を `H.264+AAC` の `MP4` にエンコードします
- 本プログラムの利用には `ffmpeg` コマンドが必要です

## How to Build

### Support platform

- UNIX
- Linux
- macOS

### Pre-requests

- CLANG C++
- GNU MAKE
- ffmpeg and following codecs:
  - libx264 (H.264)
  - libfdk_aac (AAC)

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

本プログラムを動作させるとワークディレクトリ `.m2penc` 以下に1フレーム毎の画像データ（png形式）と音声データ（wav形式）が生成され、それらを `ffmpeg` を以下のようなコマンドオプション指定で実行することで `H.264+AAC` の `MP4` にエンコードします。

```
ffmpeg -y -r 60 -start_number 0 -i .m2penc/%08d.png -i .m2penc/sound.wav -acodec libfdk_aac -profile:a aac_he -afterburner 1 -vcodec libx264 -pix_fmt yuv420p -r 60 playlog.mp4
```

`ffmpeg` は次のコーデックが利用できる状態にする必要があります:

- `libx264` H.264 コーデック
- `libfdk_aac` AAC コーデック

> __参考（macOSのffmpegコマンドで `libfdk_aac` を利用する手順）__
>
> macOS の brew でインストールした ffmpeg コマンドには標準では `libfdk_aac` が含まれていません。しかし、標準の AAC コーデックの音質は実用的ではないため m2penc では `libfdk_aac` を必須としています。以下の手順を参考にして `libfdk_aac` が利用できる `ffmpeg` をインストールしてください。
>
> [https://scrapbox.io/craftmemo/mac_で_ffmpeg_で_libfdk_aac_を使う](https://scrapbox.io/craftmemo/mac_%E3%81%A7_ffmpeg_%E3%81%A7_libfdk_aac_%E3%82%92%E4%BD%BF%E3%81%86)

### Settings: settings.json

- m2penc を利用するには利用する BIOS ファイルやスロット構成を定義した設定ファイル `settings.json` が必要です
- m2p ファイルの録画を行った時と同じ BIOS 構成にしなければ、正常な動画出力がされないことがあるため注意してください
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

## Execution Example

```
% ./m2penc playlog.m2p 
Unuse font
Setup Slot0-0 Page#0 = MAIN <../../msx2-osx/bios/cbios_main_msx2+_jp.rom>
Setup Slot0-0 Page#1 = MAIN <../../msx2-osx/bios/cbios_main_msx2+_jp.rom>
Setup Slot0-0 Page#2 = LOGO <../../msx2-osx/bios/cbios_logo_msx2+.rom>
Setup Slot3-0 Page#0 = SUB <../../msx2-osx/bios/cbios_sub.rom>
insert ROM data
load state
Writing frame: 4233 of 4233 (100%) ... done
ffmpeg -y -r 60 -start_number 0 -i .m2penc/%08d.png -i .m2penc/sound.wav -acodec libfdk_aac -profile:a aac_he -afterburner 1 -vcodec libx264 -pix_fmt yuv420p -r 60 playlog.mp4
ffmpeg version 6.0 Copyright (c) 2000-2023 the FFmpeg developers
  built with Apple clang version 14.0.3 (clang-1403.0.22.14.1)
  configuration: --prefix=/usr/local/Cellar/ffmpeg/6.0-with-options_1 --enable-shared --cc=clang --host-cflags= --host-ldflags= --enable-gpl --enable-libaom --enable-libdav1d --enable-libmp3lame --enable-libopus --enable-libsnappy --enable-libtheora --enable-libvorbis --enable-libvpx --enable-libx264 --enable-libx265 --enable-libfontconfig --enable-libfreetype --enable-frei0r --enable-libass --enable-demuxer=dash --enable-opencl --enable-audiotoolbox --enable-videotoolbox --disable-htmlpages --enable-libfdk-aac --enable-nonfree
  libavutil      58.  2.100 / 58.  2.100
  libavcodec     60.  3.100 / 60.  3.100
  libavformat    60.  3.100 / 60.  3.100
  libavdevice    60.  1.100 / 60.  1.100
  libavfilter     9.  3.100 /  9.  3.100
  libswscale      7.  1.100 /  7.  1.100
  libswresample   4. 10.100 /  4. 10.100
  libpostproc    57.  1.100 / 57.  1.100
Input #0, image2, from '.m2penc/%08d.png':
  Duration: 00:02:49.32, start: 0.000000, bitrate: N/A
  Stream #0:0: Video: png, rgb48be(pc), 284x240, 25 fps, 25 tbr, 25 tbn
Guessed Channel Layout for Input Stream #1.0 : stereo
Input #1, wav, from '.m2penc/sound.wav':
  Duration: 00:01:10.55, bitrate: 1411 kb/s
  Stream #1:0: Audio: pcm_s16le ([1][0][0][0] / 0x0001), 44100 Hz, 2 channels, s16, 1411 kb/s
Stream mapping:
  Stream #0:0 -> #0:0 (png (native) -> h264 (libx264))
  Stream #1:0 -> #0:1 (pcm_s16le (native) -> aac (libfdk_aac))
Press [q] to stop, [?] for help
[image2 @ 0x7f8c171043c0] Thread message queue blocking; consider raising the thread_queue_size option (current value: 8)
[libx264 @ 0x7f8c1700b1c0] using cpu capabilities: MMX2 SSE2Fast SSSE3 SSE4.2 AVX FMA3 BMI2 AVX2
[libx264 @ 0x7f8c1700b1c0] profile High, level 2.1, 4:2:0, 8-bit
[libx264 @ 0x7f8c1700b1c0] 264 - core 164 r3095 baee400 - H.264/MPEG-4 AVC codec - Copyleft 2003-2022 - http://www.videolan.org/x264.html - options: cabac=1 ref=3 deblock=1:0:0 analyse=0x3:0x113 me=hex subme=7 psy=1 psy_rd=1.00:0.00 mixed_ref=1 me_range=16 chroma_me=1 trellis=1 8x8dct=1 cqm=0 deadzone=21,11 fast_pskip=1 chroma_qp_offset=-2 threads=7 lookahead_threads=1 sliced_threads=0 nr=0 decimate=1 interlaced=0 bluray_compat=0 constrained_intra=0 bframes=3 b_pyramid=2 b_adapt=1 b_bias=0 direct=1 weightb=1 open_gop=0 weightp=2 keyint=250 keyint_min=25 scenecut=40 intra_refresh=0 rc_lookahead=40 rc=crf mbtree=1 crf=23.0 qcomp=0.60 qpmin=0 qpmax=69 qpstep=4 ip_ratio=1.40 aq=1:1.00
Output #0, mp4, to 'playlog.mp4':
  Metadata:
    encoder         : Lavf60.3.100
  Stream #0:0: Video: h264 (avc1 / 0x31637661), yuv420p(tv, progressive), 284x240, q=2-31, 60 fps, 15360 tbn
    Metadata:
      encoder         : Lavc60.3.100 libx264
    Side data:
      cpb: bitrate max/min/avg: 0/0/0 buffer size: 0 vbv_delay: N/A
  Stream #0:1: Audio: aac (HE-AAC) (mp4a / 0x6134706D), 44100 Hz, stereo, s16, 64 kb/s
    Metadata:
      encoder         : Lavc60.3.100 libfdk_aac
frame= 4234 fps=650 q=-1.0 Lsize=    1230kB time=00:01:10.52 bitrate= 142.9kbits/s dup=1 drop=0 speed=10.8x     
video:600kB audio:554kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: 6.582767%
[libx264 @ 0x7f8c1700b1c0] frame I:18    Avg QP:21.75  size:  6223
[libx264 @ 0x7f8c1700b1c0] frame P:1148  Avg QP:21.56  size:   357
[libx264 @ 0x7f8c1700b1c0] frame B:3068  Avg QP:21.33  size:    30
[libx264 @ 0x7f8c1700b1c0] consecutive B-frames:  0.9%  6.0%  4.0% 89.0%
[libx264 @ 0x7f8c1700b1c0] mb I  I16..4: 15.0% 57.6% 27.4%
[libx264 @ 0x7f8c1700b1c0] mb P  I16..4:  0.2%  0.2%  0.4%  P16..4:  3.5%  1.0%  0.7%  0.0%  0.0%    skip:94.1%
[libx264 @ 0x7f8c1700b1c0] mb B  I16..4:  0.2%  0.0%  0.0%  B16..8:  2.9%  0.1%  0.0%  direct: 0.0%  skip:96.8%  L0:38.9% L1:60.6% BI: 0.5%
[libx264 @ 0x7f8c1700b1c0] 8x8 transform intra:40.0% inter:3.3%
[libx264 @ 0x7f8c1700b1c0] coded y,uvDC,uvAC intra: 22.6% 20.9% 20.0% inter: 0.4% 0.5% 0.4%
[libx264 @ 0x7f8c1700b1c0] i16 v,h,dc,p: 49% 33% 18%  0%
[libx264 @ 0x7f8c1700b1c0] i8 v,h,dc,ddl,ddr,vr,hd,vl,hu: 45% 26% 29%  0%  0%  0%  0%  0%  0%
[libx264 @ 0x7f8c1700b1c0] i4 v,h,dc,ddl,ddr,vr,hd,vl,hu: 23% 29% 30%  2%  3%  3%  3%  4%  4%
[libx264 @ 0x7f8c1700b1c0] i8c dc,h,v,p: 64% 30%  5%  1%
[libx264 @ 0x7f8c1700b1c0] Weighted P-Frames: Y:0.0% UV:0.0%
[libx264 @ 0x7f8c1700b1c0] ref P L0: 78.9%  4.6%  9.8%  6.7%
[libx264 @ 0x7f8c1700b1c0] ref B L0: 82.3% 14.7%  3.0%
[libx264 @ 0x7f8c1700b1c0] ref B L1: 97.1%  2.9%
[libx264 @ 0x7f8c1700b1c0] kb/s:69.59
% ls -l playlog.mp4 
-rw-r--r--@ 1 suzukiplan  staff  1259512  6 20 16:17 playlog.mp4
%
```

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
