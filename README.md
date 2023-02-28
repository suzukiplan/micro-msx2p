# msx2-osx

macOS 用の MSX2+ エミュレータ

# core modules

- エミュレータのコアモジュール([./msx2-osx/core](./msx2-osx/core))はOS非依存
  - iOS, Android, Windows, macOS, PlayStation, NintendoSwitch などで使用可能な筈
- セーブデータはエンディアンモデルが異なるコンピュータ間では互換性が無い
- 32bit CPU は非サポート（64bit CPU only)

## [./msx2-osx/core](./msx2-osx/core) の使い方


### include


```c++
#include "msx2.hpp"
```

### setup

```c++
// ディスプレイのカラーモードを指定 (0: RGB555, 1: RGB565)
MSX2 msx2(0);      

// 拡張スロットの有効化設定
msx2.setupSecondaryExist(false, false, false, true);

// 漢字フォントを読み込む
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

### load media

#### （カートリッジソフトを使う場合）

```c++
// 適切なメガロム種別の指定が必要:
// - MSX2_ROM_TYPE_NORMAL 0
// - MSX2_ROM_TYPE_ASC8 1
// - MSX2_ROM_TYPE_ASC8_SRAM2 2
// - MSX2_ROM_TYPE_ASC16 3
// - MSX2_ROM_TYPE_KONAMI_SCC 4
// - MSX2_ROM_TYPE_KONAMI 5
msx2.loadRom(rom, romSize, megaRomType);
```

#### (FDDを使う場合)

```c++
msx2.insertDisk(driveId, data, size, true);
msx2.ejectDisk(driveId);
```

### execution

```c++
// リセット
msx2.reset();

// 1フレーム実行
// キー入力は1フレームに1キーのみ送信可能（動画対応のための制限）
tick(pad1, pad2, key);

// 1フレーム実行後の音声データを取得 (44100Hz 16bit Stereo)
size_t soundSize;
void* sound = msx2.getSound(&soundSize);

// 1フレーム実行後の映像を取得
// - Size: 568(width) x 480(height) x 2(16bit-color)
// - Color: RGB555 or RGB565 (コンストラクタで指定したもの)
unsigned short* display = msx2.vdp.display;
```

### quick save/load

```c++
// セーブ
size_t size;
const void* saveData = msx2.quickSave(&size);

// ロード
msx2.quickLoad(saveData, size);
```

#### (セーブデータサイズについての補足)

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
|`R:1`|65,536|y|マッパー1 RAM (128KB)|
|`R:2`|65,536|y|マッパー2 RAM (192KB)|
|`R:3`|65,536|y|マッパー3 RAM (256KB)|

#### (ディスク挿入状態のロード手順)

フロッピーディスクの挿入状態は記憶されるが、挿入データの復元は手動で行う必要がある。

挿入されていたフロッピーディスクはCRC符号のみFDCコンテキストに記憶されている。

```c++
msx2.fdc.ctx.crc[driveId]
```

次のように実装することでディスク挿入状態を自動的に復元できる。

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

#### （ディスク書き込み状態の復元についての補足）

`msx2.insertDisk` の第4引数 `readOnly` を `false` にすることで書き込み可能ディスクとして挿入できる。

```
msx2.insertDisk(driveId, myDisk.data, myDisk.size, false);
```

- ディスクの書き込み状態は、ディスク（CRC）のセクタ番号（絶対セクタ番号）単位でジャーナルに記憶している。
- ジャーナルは、クイックセーブ時に有効な全て記憶され、クイックロード時に復元される。
- ジャーナル情報が存在する場合 `msx2.insertDisk` が行われた時に自動的にオンメモリのディスクキャッシュに反映される。

> つまり、何も考えずに quick save/load して `msx2.insertDisk` すればディスク更新状態も自動的に復元される。 
