# micro MSX2+

- micro MSX2+ は、自作の MSX, MSX2, MSX2+ 用のゲームソフトを Nintendo Switch、PlayStation、XBOX などの家庭用ゲーム機、スマートフォンアプリ、PCアプリ（Steam等）などとして販売する用途（組み込み用途）に特化した最小構成の MSX2+ エミュレータです。
- 一応実機 BIOS で動作することも可能ですが、基本的には C-BIOS を用いた ROM カートリッジ形式のゲームソフトの販売用途での利用を想定しています。
  - [ROMカートリッジ形式のゲームソフトの作成方法についての参考資料](https://qiita.com/suzukiplan/items/b369d3f9b41be55b247e)
  - MSX-BASIC のプログラムは C-BIOS では動かせません（実機BIOSが必要です）
  - FDC (東芝製) の実装は入っていますが FDC へのアクセスには実機 DISK BIOS が必要です
  - OPLL (YM-2413) の実装は入ってますが FMPAC の BIOS 実装は入ってません（ポート $7C, $7D を直叩きすることで再生することは可能）
- 本リポジトリでは、micro MSX2+ の実装例として Cocoa (macOS) 用のエミュレータ実装が付随しています。

## Unimplemented Features

- VDP
  - Screen Mode: TEXT1
    - そもそもこの機能が無いと動かないゲームがあるのか？
    - TEXT2 については vdptest というプログラムの動作検証のため一応実装済み
  - インタレース機能
    - 実装そのものは簡単にできるが処理負荷が無駄に上がってしまうため実装していない
    - そもそもこの機能が無いと動かないゲームがあるのか？
  - Even/Oddフラグ
    - そもそもこの機能が無いと動かないゲームがあるのか？
  - TEXT2 の BLINK 機能
    - そもそもこの機能が無いと動かないゲームがあるのか？
- Sound
  - Y8950 (MSX-AUDIO)

## Core Modules

- エミュレータ・コアモジュール([./msx2-osx/core](./msx2-osx/core))はOS非依存
  - iOS, Android, Windows, macOS, PlayStation, NintendoSwitch などで使用可能な筈
  - 64bit CPU 専用（32bit CPU は非サポート）
- セーブデータはエンディアンモデルが異なるコンピュータ間では互換性が無い

## [./msx2-osx/core](./msx2-osx/core) の使い方

### 1. Include

```c++
#include "msx2.hpp"
```

### 2. Setup

#### 2-1. usign C-BIOS

```c++
// ディスプレイのカラーモードを指定 (0: RGB555, 1: RGB565)
MSX2 msx2(0);      

// 拡張スロットの有効化設定
msx2.setupSecondaryExist(false, false, false, true);

// 必須 BIOS (main, logo, sub) を読み込む
msx2.setup(0, 0, 0, data.main, 0x8000, "MAIN");
msx2.setup(0, 0, 4, data.logo, 0x4000, "LOGO");
msx2.setup(3, 0, 0, data.ext, 0x4000, "SUB");

// RAM の割当 (3-3)
msx2.setupRAM(3, 3);
```

#### 2-2. usign FS-A1WSX BIOS

権利者から公式 BIOS の利用と再配布がライセンスされているケースであれば、次のような形で使うこともできます。

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

> 上記は FS-A1WSX の実機 BIOS が正常に動作する構成です。
> 機種の BIOS の種類によって拡張スロットの有効化範囲、各種 BIOS ROM の配置ページ、RAMの配置ページが色々と異なります。
> 実機 BIOS を利用する時は、実機のマニュアルを参照するなどして正しい配置にしてください。

### 3. Load media

#### 3-1. using ROM cartridges

```c++
// 適切なメガロム種別の指定が必要:
// - MSX2_ROM_TYPE_NORMAL (16KB or 32KB)
// - MSX2_ROM_TYPE_ASC8 (ASCII8 メガロム)
// - MSX2_ROM_TYPE_ASC8_SRAM2 (ASCII8 メガロム+SRAM)
// - MSX2_ROM_TYPE_ASC16 3 (ASCII16 メガロム)
// - MSX2_ROM_TYPE_KONAMI_SCC 4 (SCC搭載KONAMI メガロム)
// - MSX2_ROM_TYPE_KONAMI 5 (標準KONAMI メガロム)
msx2.loadRom(rom, romSize, megaRomType);
```

#### 3-2. using Floppy Disks

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

// 1フレーム実行 (キー入力は1フレームに1キーのみ送信可能な仕様)
tick(pad1, pad2, key);

// 1フレーム実行後の音声データを取得 (44100Hz 16bit Stereo)
size_t soundSize;
void* sound = msx2.getSound(&soundSize);

// 1フレーム実行後の映像を取得
// - Size: 568(width) x 480(height) x 2(16bit-color)
// - Color: RGB555 or RGB565 (コンストラクタで指定したもの)
unsigned short* display = msx2.vdp.display;
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

JCT は、クイックセーブ時に有効な全てが記憶され、クイックロード時にオンメモリで復元されます。

JCT が存在する場合 `msx2.insertDisk` が行われた時に自動的にオンメモリのディスクキャッシュに反映されます。

> つまり、何も考えずに quick save/load して `msx2.insertDisk` すればディスクの更新状態も自動的に復元されます。
> しかし、ストレージ上のオリジナルのディスクファイル（.dsk）への変更内容の commit は行われません。

#### 5-3. セーブデータサイズ

セーブデータサイズは可変で、以下のデータを LZ4 で高速圧縮しています。（無風時の圧縮後サイズは5KBほど）

LZ4 解凍後のセーブデータは、

- チャンク名: 4バイト
- チャンクサイズ: 4バイト（Little Endian）
- データ: チャンクサイズ分のデータ

という形式になっていて、チャンク情報には以下の種類があります。

|Chunk|Size in Byte|Optional|Describe|
|:-:|:-:|:-:|:-|
|`Z80`|40|n|CPUコンテキスト（レジスタ等）|
|`MMU`|48|n|メモリ管理システム（スロット）のコンテキスト（レジスタ等）|
|`SCC`|204|y|Sound Creative Chip（コナミ音源）のコンテキスト|
|`PSG`|108|n|AY-3-8910（PSG音源）のコンテキスト|
|`RTC`|104|n|クロックICのSRAM+コンテキスト|
|`KNJ`|12|n|漢字制御システムのコンテキスト|
|`VDP`|131,288|n|V9958（Video Display Processor）のRAM+コンテキスト|
|`FDC`|544|n|フロッピディスク制御装置 (TC8655AF) のコンテキスト|
|`JCT`|4|n|フロッピの書き込みセクタジャーナル|
|`JDT`|520 x JCT|n|フロッピの書き込みデータ（セクタ単位）|
|`OPL`|4,280|n|FM音源システム（YM2413）のコンテキスト|
|`SRM`|8,192|y|メガROMカートリッジSRAM|
|`PAC`|8,192|n|FM-PACのSRAM|
|`R:0`|65,536|n|マッパー0 RAM|

## Licenses

- C-BIOS:
  - Web Site: [https://cbios.sourceforge.net/](https://cbios.sourceforge.net/)
  - License: 2 caluse BSD
- emu2413:
  - Web Site: [https://github.com/digital-sound-antiques/emu2413](https://github.com/digital-sound-antiques/emu2413)
  - License: MIT
- SUZUKI PLAN - Z80 Emulator
  - Web Site: [https://github.com/suzukiplan/z80](https://github.com/suzukiplan/z80)
  - License: MIT
- micro MSX2+
  - Web Site: [https://github.com/suzukiplan/micro-msx2p](https://github.com/suzukiplan/micro-msx2p)
  - License: MIT
