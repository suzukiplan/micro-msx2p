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

// RAM の割当
msx2.setupRAM(3, 0);

// 漢字フォントを読み込む
msx2.loadFont(knjfnt16, sizeof(knjfnt16));

// 必須 BIOS (MSX2P.ROM, MSX2PEXT.ROM) を読み込む
msx2.setup(0, 0, 0, msx2p, sizeof(msx2p), "MAIN");
msx2.setup(3, 1, 0, msx2pext, sizeof(msx2pext), "SUB");

// 漢字BASIC BIOS が必要な場合は読み込む
msx2.setup(3, 1, 2, msxkanji_, sizeof(msxkanji_), "KNJ");

// FM音源が必要な場合は読み込む
msx2.setup(0, 2, 2, fmbios, sizeof(fmbios), "FM"); // 注: ラベルは必ず "FM" にする

// FDDが必要な場合は DISK BIOS を読み込む
static unsigned char empty[0x4000];
msx2.setup(3, 2, 0, empty, sizeof(empty), "DISK"); // ラベルは必ず "DISK" & 空バッファを指定
msx2.setup(3, 2, 2, disk, sizeof(disk), "DISK"); // ラベルは必ず "DISK" & BIOSを指定
msx2.setup(3, 2, 4, empty, sizeof(empty), "DISK"); // ラベルは必ず "DISK" & 空バッファを指定
msx2.setup(3, 2, 6, empty, sizeof(empty), "DISK"); // ラベルは必ず "DISK" & 空バッファを指定
```

### load media

```c++
// カートリッジソフトを使う場合（適切なメガロム種別の指定が必要）
// - MSX2_ROM_TYPE_NORMAL 0
// - MSX2_ROM_TYPE_ASC8 1
// - MSX2_ROM_TYPE_ASC8_SRAM2 2
// - MSX2_ROM_TYPE_ASC16 3
// - MSX2_ROM_TYPE_KONAMI_SCC 4
// - MSX2_ROM_TYPE_KONAMI 5
msx2.loadRom(rom, romSize, megaRomType);

// FDDを使う場合
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
// - Color: RGB555 or RGB565
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

#### （ディスク挿入状態のロード手順）

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
