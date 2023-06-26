/**
 * micro MSX2+ - Android
 * -----------------------------------------------------------------------------
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Yoji Suzuki.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */
package com.suzukiplan.msx2_android

import android.os.Bundle
import android.view.*
import androidx.appcompat.app.AppCompatActivity
import com.suzukiplan.msx2.MSX2View
import com.suzukiplan.msx2.RomType
import java.io.File
import java.util.concurrent.Executors

class MainActivity : AppCompatActivity(), MSX2View.Delegate {
    private lateinit var msx2View: MSX2View
    private lateinit var virtualJoyPad: VirtualJoyPad
    private val executor = Executors.newSingleThreadExecutor()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        val padContainer = findViewById<View>(R.id.pad_container)
        virtualJoyPad = VirtualJoyPad(padContainer)
        padContainer.setOnTouchListener(virtualJoyPad)
        msx2View = findViewById(R.id.emulator)
        msx2View.delegate = this
        executor.execute {
            msx2View.initialize(
                0x1B,
                0x20,
                assets.open("cbios_main_msx2+_jp.rom").readBytes(),
                assets.open("cbios_logo_msx2+.rom").readBytes(),
                assets.open("cbios_sub.rom").readBytes(),
                assets.open("game.rom").readBytes(),
                RomType.NORMAL
            )
        }
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