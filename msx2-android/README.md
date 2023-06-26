# micro MSX2+ for Android

## About

- micro-msx2p の機能を利用できる [ライブラリ](msx2) を提供しています
- [ライブラリ](msx2) を利用した [サンプルアプリ](app) を提供しています


## Basic Usage

### Build.gradle

```
TODO: MavenCentral へ公開
上記が完了すれば implementation をすれば組み込めます
```

### [MSX2View](msx2/src/main/java/com/suzukiplan/msx2/MSX2View.kt)

micro-msx2p を簡単に扱える Android 用の View です

__（基本的な使い方）__

#### 1. [activity_main.xml](app/src/main/res/layout/activity_main.xml) などで定義

```xml
<com.suzukiplan.msx2.MSX2View
    android:id="@+id/emulator"
    android:layout_width="match_parent"
    android:layout_height="match_parent" />
```

#### 2. Initialize

`MSX2View` はレイアウトで配置した状態だと初期化待ち状態（ブラックスクリーン）になります。

Activity や Fragment 上で `MSX2View.initialize` を呼び出し、初期化処理を行うことで動き始めます。

```kotlin
Thread {
    msx2View.initialize(
        0x1B, // SELECT ボタンのキー割当 (0x1B = ESC)
        0x20, // START ボタンのキー割当 (0x20 = SPACE)
        assets.open("cbios_main_msx2+_jp.rom").readBytes(),
        assets.open("cbios_logo_msx2+.rom").readBytes(),
        assets.open("cbios_sub.rom").readBytes(),
        assets.open("game.rom").readBytes(),
        RomType.NORMAL
    )
}.start()
```

#### 3. Set Delegate

`MSX2View` を扱う Activity または Fragment 上で `MSX2View.Delegate` の実装を行います。


```kotlin
class MainActivity : AppCompatActivity(), MSX2View.Delegate {
    private lateinit var msx2View: MSX2View
    private val joyPad = JoyPad()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        msx2View = findViewById(R.id.emulator)
        msx2View.delegate = this
           :
    }

    // 毎フレームコールバックされる（入力キーコードを返す）
    override fun msx2ViewDidRequirePad1Code() = joyPad.code

    // エミュレータ起動時（動作直前）に呼び出される（quickLoadで状態復元を行う）
    override fun msx2ViewDidStart() {
        val save = File(applicationContext.cacheDir, "save.dat")
        if (save.exists()) {
            val data = save.readBytes()
            msx2View.quickLoad(data)
        }
    }

    // エミュレータ停止時（破棄直前）に呼び出される（quickSaveで状態保持を行う）
    override fun msx2ViewDidStop() {
        val save = msx2View.quickSave() ?: return
        File(applicationContext.cacheDir, "save.dat").writeBytes(save)
    }
}
```

- `msx2ViewDidRequirePad1Code` 毎フレームコールバックされる（入力キーコードを返す）
- `msx2ViewDidStart` エミュレータ起動時（動作直前）に呼び出される
- `msx2ViewDidStop` エミュレータ停止時（破棄直前）に呼び出される

`MSX2View` は `SurfaceView` の派生クラスで、micro-msx2p のインスタンスはサーフェースのライフサイクルに従います。そして、サーフェースはアプリを終了する時やホーム画面に戻った時に破棄されます。そこで、基本的には上記で例に示しているように、クイックセーブ・ロードで状態の保持を復元を行うことを想定しています。これにより、ゲームプレイ中に突然電話が鳴ってきても、電話応対後にアプリを起動すれば状態を維持することができます。

なお `MSX2View.Delegate` の全てのコールバックは、サブスレッドから呼び出されます。

### Example

[app](app) ディレクトリ以下が `MSX2View` を用いたアプリケーション実装の例です。

![image](screen.png)

[MainActivity.kt](app/src/main/java/com/suzukiplan/msx2_android/MainActivity.kt) の実装を見れば `MSX2View` の使い方を簡単に把握できるようになっています。

```kotlin
package com.suzukiplan.msx2_android

import android.os.Bundle
import android.view.*
import androidx.appcompat.app.AppCompatActivity
import com.suzukiplan.msx2.MSX2View
import com.suzukiplan.msx2.RomType
import java.io.File


class MainActivity : AppCompatActivity(), MSX2View.Delegate {
    private lateinit var msx2View: MSX2View
    private lateinit var virtualJoyPad: VirtualJoyPad

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        val padContainer = findViewById<View>(R.id.pad_container)
        virtualJoyPad = VirtualJoyPad(padContainer)
        padContainer.setOnTouchListener(virtualJoyPad)
        msx2View = findViewById(R.id.emulator)
        msx2View.delegate = this
        Thread {
            msx2View.initialize(
                0x1B,
                0x20,
                assets.open("cbios_main_msx2+_jp.rom").readBytes(),
                assets.open("cbios_logo_msx2+.rom").readBytes(),
                assets.open("cbios_sub.rom").readBytes(),
                assets.open("game.rom").readBytes(),
                RomType.NORMAL
            )
        }.start()
    }

    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        menuInflater.inflate(R.menu.main, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when (item.itemId) {
            R.id.reset_button -> msx2View.reset()
        }
        return true
    }

    override fun dispatchTouchEvent(ev: MotionEvent?) = super.dispatchTouchEvent(ev)

    override fun msx2ViewDidRequirePad1Code() = virtualJoyPad.code

    override fun msx2ViewDidStart() {
        val save = File(applicationContext.cacheDir, "save.dat")
        if (save.exists()) {
            val data = save.readBytes()
            msx2View.quickLoad(data)
        }
    }

    override fun msx2ViewDidStop() {
        val save = msx2View.quickSave() ?: return
        File(applicationContext.cacheDir, "save.dat").writeBytes(save)
    }
}
```

Android Studio でビルドすれば [assets](app/src/main/assets) に組み込まれた game.rom が起動します

- デフォルトの game.rom は `Hello, World!` を表示するシンプルな ROM ファイルです
- game.rom を置き換えることで任意のゲームを起動できます
- メガロムを起動する時は `MSX2View.initialize` に指定している `RomType` を適切に変更してください

## Advanced Usage

`MSX2View` を用いることで、Androidで簡単に micro-msx2p を用いることができますが、JNI インタフェースをそのまま利用できる [Coreクラス](msx2/src/main/java/com/suzukiplan/msx2/Core.java) を用いることで、micro-msx2p の全ての機能を活用した高度なプログラムを開発することもできます。

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

> [app](./app)ディレクトリ配下と[msx2](./msx2)ディレクトリ配下のソースコードは全て micro MSX2+ の一部として同じライセンス下で利用可能です。
