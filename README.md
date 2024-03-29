# micro MSX2+ [![CircleCI](https://dl.circleci.com/status-badge/img/gh/suzukiplan/micro-msx2p/tree/master.svg?style=svg)](https://dl.circleci.com/status-badge/redirect/gh/suzukiplan/micro-msx2p/tree/master)

## Description

- micro MSX2+ は **組み込み用途に特化** した MSX2+ エミュレータです
  - 例えば、自作の MSX, MSX2, MSX2+ 用のゲームソフトを家庭用ゲーム機（Nintendo Switch, PlayStation, XBOXなど）、スマートフォンアプリ（iOS, Androidなど）、PCアプリ（Windows, macOS, Linuxなど）などの各種プラットフォーム向けに販売する用途（組み込み用途）を想定しています
  - つまり、エンドユーザ向けに開発された一般的な MSX エミュレータ（[openMSX](https://openmsx.org/) や [WebMSX](https://github.com/ppeccin/webmsx) など）とは異なり、MSX用のアプリケーションやゲームなどを自作して配布（販売）したい開発者向けのエミュレータのコアプログラムです
- 実機 BIOS で動作させることも可能ですが、基本的には **C-BIOS を用いて ROM カートリッジ形式のゲームソフトで利用（配布）** する用途を想定しています
  - [ROMカートリッジ形式のゲームソフトの作成方法についての参考資料](https://qiita.com/suzukiplan/items/b369d3f9b41be55b247e)
  - C-BIOS を用いることで発生する制約:
    - MSX-BASIC のプログラムは動作できません
      - MSX-BASIC のプログラムの動作には実機BIOSが必要
      - BIOS の著作権は Microsoft 等（※機種により異なる）が有しているため再配布にはそれら著作権者からの許諾が必要
    - FloppyDisk を扱うことができません
      - FDC (東芝製) の実装は入っていますが FDC へのアクセスには実機の DISK BIOS が必要
    - FM-PAC の BIOS またはカートリッジを扱うことができません
      - OPLL (YM-2413) の実装自体は入っているので、FM-BIOS 経由でのアクセスではなく、出力ポート $7C, $7D を直叩きすることで OPLL を再生することは可能
    - 商標 `MSX` は MSX Licenses Corporation の登録商標のため、製品に商標を含める等（利用）に当たっては MSX Licensing Corporation からの許諾が必要
      - 商標許諾を得ていない（大半の）ケースでは、ゲームタイトルに「〜 for MSX」と記載したり、商品パッケージ、カセットラベル等に MSX の商標や意匠を含めることができない点を注意
    - 実機 BIOS や登録商標 `MSX` を用いる必要があるケースでは [プロジェクトEGGクリエイターズ](https://www.amusement-center.com/project/egg/creators/) を使った方が良さそうです（私が知る限りではこれ以外に実機 BIOS と MSX の商標を合法的に利用する手段は無いと思われます）
- micro MSX2+ が対応するメガロムの種別は [msx2def.h](https://github.com/suzukiplan/micro-msx2p/blob/master/src/msx2def.h) を参照してください
  - `MSX2_ROM_TYPE_NORMAL` 標準ROM (16KB, 32KB)
  - `MSX2_ROM_TYPE_ASC8` ASCII8 メガロム
  - `MSX2_ROM_TYPE_ASC8_SRAM2` ASCII8 + SRAM メガロム
  - `MSX2_ROM_TYPE_ASC16` ASCII16 メガロム
  - `MSX2_ROM_TYPE_ASC16_SRAM2` ASCII16 + SRAM メガロム
  - `MSX2_ROM_TYPE_KONAMI_SCC` KONAMI SCC メガロム
  - `MSX2_ROM_TYPE_KONAMI` KONAMI 標準メガロム

## Platform Implementations

|Directory|Platform|Core|
|:-|:-|:-|
|[msx1-osx](./msx1-osx)|macOS+Cocoa|MSX1|
|[msx2-osx](./msx2-osx)|macOS+Cocoa|MSX2+|
|[msx2-android](./msx2-android)|Android|MSX2+|
|[msx2-ios](./msx2-ios)|iOS|MSX2+|
|[msx2-dotnet](./msx2-dotnet)|.NET Core|MSX2+|
|[msx1-m5stack](./msx1-m5stack)|M5Stack CoreS3|MSX1|
|[msx1-m5stamps3](./msx1-m5stamps3)|M5StampS3|MSX1|
|[msx2-sdl2](./msx2-sdl2)|SDL2 (macOS and Linux)|MSX2+|
|[msx1-rpizero](./msx1-rpizero)|RaspberryPi Zero, Zero W, Zero WH (Bare Metal)|MSX1|
|[msx2-rpizero](./msx2-rpizero)|RaspberryPi Zero, Zero W, Zero WH (Bare Metal)|MSX2+|
|[msx1-rpizero2](./msx1-rpizero2)|RaspberryPi Zero 2W (Bare Metal)|MSX1|
|[msx2-rpizero2](./msx2-rpizero2)|RaspberryPi Zero 2W (Bare Metal)|MSX2+|

Following platform support is now planning:

|Directory|Platform|Core|
|:-|:-|:-|
|msx1-rpi4b|RaspberryPi 4 model B (Bare Metal)|MSX1|
|msx2-rpi4b|RaspberryPi 4 model B (Bare Metal)|MSX2+|
|msx1-dotnet-wpf|Windows .NET Framework|MSX1|
|msx2-dotnet-wpf|Windows .NET Framework|MSX2+|
|msx1-uwp|Universal Windows Platform|MSX1|
|msx2-uwp|Universal Windows Platform|MSX2+|
|msx1-win32|Windows native x86 (32bit)|MSX1|
|msx2-win32|Windows native x86 (32bit)|MSX2+|
|msx1-win64|Windows native x64|MSX1|
|msx2-win64|Windows native x64|MSX2+|
|msx1-win64a|Windows native ARMv8|MSX1|
|msx2-win64a|Windows native ARMv8|MSX2+|

## Unimplemented Features

幾つかの MSX2/MSX2+ の機能はまだ実装されていません。（※一部パフォーマンスを優先して意図的に省略している機能もありますが、それらを含めて将来的に実装する可能性があります）

- VDP
  - インタレース機能
    - 実装そのものは簡単にできるが、処理負荷が増大してしまうため実装を省略
    - _そもそもこの機能が無いと動かないゲームがあるのか？_
  - Even/Oddフラグ
  - TEXT2 の BLINK 機能
  - 同期モード
- Sound
  - Y8950 (MSX-AUDIO)
    - 主流は OPLL (FM-PAC) だと考えられるので実装省略
- MMU
  - Memory Mapper (RAM サイズは 64KB 固定)
- その他
  - BEEP音 (仕様規定が無いし使い所も分からないので省略)
  - Printer, RS-232C, Modem, Mouse, Light Pen 等の周辺機器対応
  - System Control, AV Control

## How to use [micro MSX2+ core module](./src)

- **エミュレータ・コアモジュール** ([./src](./src)以下の全てのファイル) を C++ プロジェクトに組み込んで使用します
  - C/C++の標準ライブラリ以外は使用していないので、プラットフォーム（Nintendo Switch、PlayStation、XBOX, iOS, Android, Windows, macOS, PlayStation, NintendoSwitch など）に関係無くビルド可能な筈です
  - ESP32 だと動かないんじゃないかな（性能的な意味で）
- セーブデータはエンディアンモデルが異なるコンピュータ間では互換性が無いため、プラットフォーム間でセーブデータのやりとりをする場合は注意してください

### 1. Include

```c++
#include "msx2.hpp"
```

### 2. Setup

#### 2-1. Create Instance

コンストラクタ引数にディスプレイカラーモードを指定してインスタンスを生成します。

```c++
// ディスプレイのカラーモードを指定 (0: RGB555, 1: RGB565)
MSX2 msx2(0);

// ポート 7C,7D 直叩きで OPLL (YM2413) を使いたい場合、第2引数に true を指定すれば
// FMBIOS を用いなくても OPLL のインスタンスが生成されて利用可能になります
MSX2 msx2(0, true);
```

ディスプレイカラーモードの指定により `msx2.getDisplay()` に格納される画面表示用データのピクセル形式が RGB555 (0) または RGB565 (1) の何れかになります。

#### 2-2. Setup Slot

##### (1) C-BIOS を用いる場合

```c++

// 拡張スロットの有効化設定
msx2.setupSecondaryExist(false, false, false, true);

// 必須 BIOS (main, logo, sub) を読み込む
msx2.setup(0, 0, 0, data.main, 0x8000, "MAIN");
msx2.setup(0, 0, 4, data.logo, 0x4000, "LOGO");
msx2.setup(3, 0, 0, data.ext, 0x4000, "SUB");

// RAM の割当 (3-3)
msx2.setupRAM(3, 3);
```

##### (2) FS-A1WSX BIOS を用いる場合

権利者から公式 BIOS の利用と再配布がライセンスされているケースや、ご自身で吸い出した実機 BIOS を用いてプログラムを配信せずに趣味の範囲で楽しみたい場合の例を示します。

以下、Panasonic FS-A1WSX の実機 BIOS を用いる手順を示します。

```c++
// 拡張スロットの有効化設定
msx2.setupSecondaryExist(false, false, false, true);

// 漢字フォントを読み込む (optional)
msx2.loadFont(knjfnt16, sizeof(knjfnt16));

// RAM の割当 (3-0)
msx2.setupRAM(3, 0);

// 必須 BIOS (MSX2P.ROM, MSX2PEXT.ROM) を読み込む
msx2.setup(0, 0, 0, msx2p, 0x8000, "MAIN");
msx2.setup(3, 1, 0, msx2pext, 0x4000, "SUB");

// 漢字BASIC BIOS を読み込む (optional)
msx2.setup(3, 1, 2, knjdrv, 0x8000, "KNJ");

// DISK BIOS を読み込む (optional)
// ※FDCはTOSHIBA TC8566AF のみ対応
static unsigned char empty[0x4000];
msx2.setup(3, 2, 0, empty, 0x4000, "DISK"); // ラベルは必ず "DISK" & 空バッファを指定
msx2.setup(3, 2, 2, disk, 0x4000, "DISK"); // ラベルは必ず "DISK" & BIOSを指定
msx2.setup(3, 2, 4, empty, 0x4000, "DISK"); // ラベルは必ず "DISK" & 空バッファを指定
msx2.setup(3, 2, 6, empty, 0x4000, "DISK"); // ラベルは必ず "DISK" & 空バッファを指定

// FM-PAC BIOS を読み込む (optional)
msx2.setup(3, 3, 2, data.fm, sizeof(data.fm), "FM");
```

機種によって拡張スロットの有効化範囲、各種 BIOS ROM の配置、RAMの配置が異なり、正しい配置にしなければ正常に動作しません。（正しい配置については、各機種付属のマニュアルに記載されている筈です）

`setup` の引数には次のようにパラメタを指定します。

```c++
msx2.setup(基本スロット, 拡張スロット, 開始アドレス÷0x2000, サイズ, "ラベル");
```

ラベルは次のように指定します。

- `"MAIN"` ... メインBIOS (MSX2P.ROM)
- `"SUB"` ... サブROM (MSX2PEXT.ROM)
- `"KNJ"` ... 漢字ドライバ (KNJDRV.ROM)
- `"DISK"` ... ディスクBIOS (DISK.ROM)
- `"FM"` ... FM-PAC (FMBIOS.ROM)

上記以外の BIOS が含まれる機種の場合、上記以外の任意の文字列を設定してください。

#### 2-3. Special Key Assign

micro MSX2+ は、1フレーム毎にジョイパッド入力（1P/2P各）と入力キー（ASCIIコード等）を指定し、ジョイパッドの入力は 1バイト で 1P/2P のそれぞれに入力キービットをセットして指定する仕様です。

```c
#define MSX2_JOY_UP 0b00000001
#define MSX2_JOY_DW 0b00000010
#define MSX2_JOY_LE 0b00000100
#define MSX2_JOY_RI 0b00001000
#define MSX2_JOY_T1 0b00010000
#define MSX2_JOY_T2 0b00100000
#define MSX2_JOY_S1 0b01000000
#define MSX2_JOY_S2 0b10000000
```

例えば、1P側のジョイパッドの上ボタン（UP）、左ボタン（LE）、トリガ1（T1）を押している状態で1フレーム動かす場合、次のように指定します。

```c++
msx2.tick(MSX2_JOY_UP | MSX2_JOY_LE | MSX2_JOY_T1, 0, 0);
```

MSX のジョイパッドは通常、DPAD（上下左右）と2トリガーの6ボタンの仕様のため、上位2ビット（bit-7, bit-6）の指定は無視されますが、micro MSX2+ ではそれらのボタン（S1/S2）に任意のキーアサインを行うことができます。

例えば、1P側のS1ボタン（ファミコンのスタートボタン相当）に SPACE キー、S2ボタン（ファミコンのセレクトボタン相当）に ESC キーを割り当てる場合は、次のようにセットアップしてください。

```c++
msx2.setKeyAssign(0, MSX2_JOY_S1, ' '); // SPACE キーを S1 (START) ボタンに割り当てる
msx2.setKeyAssign(0, MSX2_JOY_S2, 0x1B); // ESC キーを S2 (SELECT) ボタンに割り当てる
```

大半のゲームデザインでは、ジョイパッド以外のキーアサインが2つあれば問題ないものと思われます。しかし、実用プログラムなどで3つ以上のキーを利用することが不可避なケースでは、tickの第3引数にキーコードを指定する方式での実装が必要になります。

キーコードは通常 ASCII コード（`'A'` など）で指定できますが、一部の制御コードには特殊な割り当てがされています。

（特殊キーコード）

- `'\t'` : TAB
- `'\r'` : RETURN
- `'\n'` : RETURN
- `0x18` : CTRL + STOP
- `0x1B` : ESC
- `0x7F` : DEL
- `0xC0` : up cursor
- `0xC1` : down cursor
- `0xC2` : left cursor
- `0xC3` : right cursor
- `0xF1` : f1
- `0xF2` : f2
- `0xF3` : f3
- `0xF4` : f4
- `0xF5` : f5
- `0xF6` : f6 (shift + f1)
- `0xF7` : f7 (shift + f2)
- `0xF8` : f8 (shift + f3)
- `0xF9` : f9 (shift + f4)
- `0xFA` : f10 (shift + f5)

実用的なプログラムで、上記以外のキー（HOME, SELECTなど）やより細かなキー操作を行いたい場合 `tickWithKeyCodeMap` でキーコードマップを指定することも可能です。

```c++
msx2.tickWithKeyCodeMap(pad1, pad2, keyCodeMap);
```

`keyCodeMap` は、MSXのキーマトリクスと対応する 11 bytes の unsigned char 型配列で、各要素・各 bit でキーの押下状態を指定します。

|要素番号|bit-7|bit-6|bit-5|bit-4|bit-3|bit-2|bit-1|bit-0|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|0|`7`|`6`|`5`|`4`|`3`|`2`|`1`|`0`|
|1|`;`|`[`|`@`|`\`|`^`|`-`|`9`|`8`|
|2|`B`|`A`|`_`|`/`|`.`|`,`|`]`|`:`|
|3|`J`|`I`|`H`|`G`|`F`|`E`|`D`|`C`|
|4|`R`|`Q`|`P`|`O`|`N`|`M`|`L`|`K`|
|5|`Z`|`Y`|`X`|`W`|`V`|`U`|`T`|`S`|
|6|F3|F2|F1|かな|CAPS|GRAPH|CTRL|SHIFT|
|7|RETURN|SELECT|BS|STOP|TAB|ESC|F5|F4|
|8|→|↓|↑|←|DEL|INS|HOME|SPACE|
|9 (tenkey)|`4`|`3`|`2`|`1`|`0`|option|option|option|
|10 (tenkey)|`,`|`.`|`-`|`9`|`8`|`7`|`6`|`5`|

### 3. Load External Media

#### 3-1. ROM cartridges

```c++
// 適切なメガロム種別の指定が必要:
// - MSX2_ROM_TYPE_NORMAL ....... 標準ROM(16KB or 32KB)
// - MSX2_ROM_TYPE_ASC8 ......... ASCII8 メガロム
// - MSX2_ROM_TYPE_ASC8_SRAM2 ... ASCII8 メガロム+SRAM
// - MSX2_ROM_TYPE_ASC16 ........ ASCII16 メガロム
// - MSX2_ROM_TYPE_ASC16_SRAM2 .. ASCII16 メガロム+SRAM
// - MSX2_ROM_TYPE_KONAMI_SCC ... KONAMI メガロム (SCC搭載)
// - MSX2_ROM_TYPE_KONAMI ....... KONAMI メガロム
msx2.loadRom(rom, romSize, megaRomType);
```

#### 3-2. Floppy Disks

```c++
msx2.insertDisk(driveId, data, size, true); // write protect
msx2.insertDisk(driveId, data, size, false); // writable
msx2.ejectDisk(driveId);
```

FDのアクセスには時間が掛かるため、セクタ読み込みや書き込みのタイミングでアクセスランプの点灯やバイブレーション等の実装をすることが望ましいです。

```c++
msx2.fdc.setDiskReadListener(this, [](void* arg, int driveId, int sector) {
    // ディスクが1セクタ読み込まれるタイミングでコールバック
});
msx2.fdc.setDiskWriteListener(this, [](void* arg, int driveId, int sector) {
    // ディスクへ1セクタ書き込まれるタイミングでコールバック
});
```

### 4. Execution

```c++
// リセット
msx2.reset();

// 1フレーム実行 (キー入力は1フレームに1キーのみ送信で十分な場合)
msx2.tick(pad1, pad2, key);

// 1フレーム実行（キー入力にコードマップを用いる場合）
msx2.tickWithKeyCodeMap(pad1, pad2, keyCodeMap);

// 1フレーム実行後の音声データを取得 (44100Hz 16bit Stereo)
size_t soundSize;
void* sound = msx2.getSound(&soundSize);

// 1フレーム実行後の映像を取得
// - Size: 568(width) x 240(height) x 2(16bit-color)
// - Color: RGB555 or RGB565 (コンストラクタで指定したもの)
unsigned short* display = msx2.getDisplay();
int displayWidth = msx2.getDisplayWidth(); // 568
int displayHeight = msx2.getDisplayHeight(); // 240 (※将来的にインタレース対応時に480になる可能性がある)
```

### 5. Quick Save/Load

```c++
// セーブ
size_t size;
const void* saveData = msx2.quickSave(&size);

// ロード
msx2.quickLoad(saveData, size);
```

#### 5-1. ディスク挿入状態のロード手順

フロッピーディスクの挿入状態は記憶されますが、挿入データの復元はされないため、復元のための追加実装が必要です。

まず、挿入されていたフロッピーディスクはCRC符号のみFDCコンテキストに記憶されています。

```c++
msx2.fdc.ctx.crc[driveId]
```

このコンテキスト情報を参照して、次のような手順でディスク挿入状態の復元を行ってください。

1. `msx2.quickLoad` でクイックロード
2. `msx2.fdc.calcDiskCrc` で上記CRC符号と一致するディスクを探索
3. `msx2.insertDisk` で挿入

```c++
for (int driveId = 0; driveId < 2; driveId++) {
    for (int i = 0; i < MY_DISK_NUM; i++) {
        auto crc = msx2.fdc.calcDiskCrc(myDisks[i].data, myDisks[i].size);
        if (crc == fdc.ctx.crc[driveId]) {
            msx2.insertDisk(driveId, myDisks[i].data, myDisks[i].size, true);
            break;
        }
    }
}
```

#### 5-2. ディスク書き込み状態の復元についての補足

`msx2.insertDisk` の第4引数 `readOnly` を `false` にすることで書き込み可能ディスクとして挿入できます。

```c++
msx2.insertDisk(driveId, myDisk.data, myDisk.size, false);
```

micro MSX2+ では、ディスクの書き込み状態は、ディスク（CRC）のセクタ番号（絶対セクタ番号）単位でジャーナル (JCT) に記憶します。

JCT は、クイックセーブ時に各セクタの最新情報のみが記憶され、クイックロード時にオンメモリで復元されます。

JCT が存在する場合 `msx2.insertDisk` が行われた時に自動的にオンメモリのディスクキャッシュに反映されます。

> つまり、何も考えずに quick save/load して `msx2.insertDisk` すればディスクの更新状態も自動的に復元されます。
> しかし、ストレージ上のオリジナルのディスクファイル（.dsk）への変更内容の commit _(.dskファイルの更新)_ は行われません。

#### 5-3. セーブデータサイズ

セーブデータサイズは可変で、以下のデータを LZ4 で高速圧縮しています。（無風時の圧縮後サイズは5KBほど）

LZ4 解凍後のセーブデータは、

- チャンク名: 4バイト
- チャンクサイズ: 4バイト（Little Endian）
- データ: チャンクサイズ分のデータ

という形式になっていて、チャンク情報には以下の種類があります。

|Chunk|Size in Byte|Optional|Describe|
|:-:|:-:|:-:|:-|
|`BRD`|260|n|VMコンテキスト|
|`Z80`|40|n|CPUコンテキスト（レジスタ等）|
|`MMU`|48|n|メモリ管理システム（スロット）のコンテキスト（レジスタ等）|
|`SCC`|204|y|Sound Creative Chip（コナミ音源）のコンテキスト|
|`PSG`|108|n|AY-3-8910（PSG音源）のコンテキスト|
|`RTC`|104|n|クロックICのSRAM+コンテキスト|
|`KNJ`|12|n|漢字制御システムのコンテキスト|
|`VDP`|131,288|n|V9958（Video Display Processor）のRAM+コンテキスト|
|`FDC`|544|y|フロッピディスク制御装置 (TC8655AF) のコンテキスト|
|`JCT`|4|y|フロッピの書き込みセクタジャーナル|
|`JDT`|520 x JCT|y|フロッピの書き込みデータ（セクタ単位）|
|`OPL`|4,280|y|FM音源システム（YM2413）のコンテキスト|
|`SRM`|8,192|y|メガROMカートリッジSRAM|
|`PAC`|8,192|n|FM-PACのSRAM|
|`R:0`|65,536|n|マッパー0 RAM|

## How to use [micro MSX1 core module](./src1)

MSX2/2+ は古いパソコンの割に要求スペックが大きく、例えば IoT 機器などで使われている Arduino や ESP32 など、搭載メモリ容量が小さく CPU も遅い組み込み用マイクロプロセッサ向けのエミュレーションはとても困難です。

そこで、MSX2/2+ と比較して要求スペックがかなり低い MSX1 に絞ったコアモジュールも併せて提供しています。

詳しい使い方は [M5Stack 版サンプルの app.cpp](./msx1-m5stack/src/app.cpp) でご確認ください。

基本的な使い方は MSX2/2+ とほぼ同じですが、ROM カートリッジ以外の外部メディア、OPLL、SCC などは非サポートとしています。

## Licenses

micro MSX2+ には次のソフトウェアが含まれています。

利用に当たっては、著作権（財産権）及び著作者人格権は各作者に帰属する点の理解と、ライセンス条項の厳守をお願いいたします。

- LZ4 Library
  - Web Site: [https://github.com/lz4/lz4](https://github.com/lz4/lz4) - [lib](https://github.com/lz4/lz4/tree/dev/lib)
  - License: [2-Clause BSD](./licenses-copy/lz4-library.txt)
  - `Copyright (c) 2011-2020, Yann Collet`
- C-BIOS
  - Web Site: [https://cbios.sourceforge.net/](https://cbios.sourceforge.net/)
  - License: [2-Clause BSD](./licenses-copy/cbios.txt)
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
  - License: [MIT](./licenses-copy/emu2413.txt)
  - `Copyright (c) 2001-2019 Mitsutaka Okazaki`
- SUZUKI PLAN - Z80 Emulator
  - Web Site: [https://github.com/suzukiplan/z80](https://github.com/suzukiplan/z80)
  - License: [MIT](./licenses-copy/z80.txt)
  - `Copyright (c) 2019 Yoji Suzuki.`
- micro MSX2+
  - Web Site: [https://github.com/suzukiplan/micro-msx2p](https://github.com/suzukiplan/micro-msx2p)
  - License: [MIT](LICENSE.txt)
  - `Copyright (c) 2023 Yoji Suzuki.`

### About 2-Clause BSD

- Permissions
  - ok: Commercial use (商用利用可)
  - ok: Modification (改変可)
  - ok: Distribution (再配布可)
  - ok: Private use (個人利用可)
- Limitations
  - No Liability (いかなるトラブルが発生しても権利者は責任を負わない)
  - No Warranty (仮に不具合等があっても権利者は保証義務を負わない)
- Conditions
  - License and copyright notice (製品にライセンス及びコピーライトの記載が必要)

### About MIT License

- Permissions
  - ok: Commercial use (商用利用可)
  - ok: Modification (改変可)
  - ok: Distribution (再配布可)
  - ok: Private use (個人利用可)
- Limitations
  - No Liability (いかなるトラブルが発生しても権利者は責任を負わない)
  - No Warranty (仮に不具合等があっても権利者は保証義務を負わない)
- Conditions
  - License and copyright notice (製品にライセンス及びコピーライトの記載が必要)
