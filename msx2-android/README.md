# micro MSX2+ - example implementation for Android

![image](screen.png)

## About

- Android 版の micro MSX2+ 実装例です
- macOS 版とは異なりシンプルな機能のみ提供しています
  - 固定ゲームの起動のみサポート
  - バーチャルパッドによる入力

## How to Use

Android Studio でビルドすれば [assets](app/src/main/assets) に組み込まれた game.rom が起動します

### game.rom

- デフォルトでは Hello, World! を表示するシンプルな ROM ファイルが組み込まれています
- [app/src/main/assets/game.rom](app/src/main/assets/game.rom) を置き換えてビルドすることで任意のゲームを起動できます

### How to launch MegaRom

メガロムを起動する場合 [app/src/main/cpp/jni.cpp](app/src/main/cpp/jni.cpp) の `loadRom` をしている箇所の `MSX2_ROM_TYPE` を書き換えてください。

```c++
msx2->loadRom(rom.data, (int) rom.size, MSX2_ROM_TYPE_NORMAL);
```

## License

本プログラムには次の OSS が含まれています。利用に当たっては、著作権（財産権）及び著作者人格権は各作者に帰属する点の理解と、ライセンス条項の厳守をお願いいたします。

- Android Open Source Project
  - Web Site: [https://source.android.com/](https://source.android.com/)
  - License: [Apache License, Version 2.0](../licenses-copy/android.txt)
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

> [app](./app)ディレクトリ配下のソースコードは全て micro MSX2+ の一部として同じライセンス下で利用可能です。
