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

import android.content.res.AssetManager
import android.os.Bundle
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity(), EmulatorView.Delegate {
    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }

    private lateinit var virtualJoyPad: VirtualJoyPad
    private val joyPad = InputStatus(
        up = false,
        down = false,
        left = false,
        right = false,
        a = false,
        b = false,
        select = false,
        start = false
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        val padContainer = findViewById<View>(R.id.pad_container)
        virtualJoyPad = VirtualJoyPad(padContainer)
        padContainer.setOnTouchListener(virtualJoyPad)
        findViewById<EmulatorView>(R.id.emulator).delegate = this
    }

    override fun dispatchKeyEvent(event: KeyEvent?): Boolean {
        event ?: return false
        val keyCode = event.keyCode
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (event.action == MotionEvent.ACTION_DOWN) {
                finish()
            }
            return true
        }
        val on = event.action == MotionEvent.ACTION_DOWN
        when (keyCode) {
            KeyEvent.KEYCODE_BUTTON_A -> joyPad.a = if (joyPad.a) false else on
            KeyEvent.KEYCODE_BUTTON_B -> joyPad.b = if (joyPad.b) false else on
            KeyEvent.KEYCODE_BUTTON_SELECT -> joyPad.select = if (joyPad.select) false else on
            KeyEvent.KEYCODE_BUTTON_START -> joyPad.start = if (joyPad.start) false else on
            KeyEvent.KEYCODE_DPAD_UP -> joyPad.up = if (joyPad.up) false else on
            KeyEvent.KEYCODE_DPAD_DOWN -> joyPad.down = if (joyPad.down) false else on
            KeyEvent.KEYCODE_DPAD_LEFT -> joyPad.left = if (joyPad.left) false else on
            KeyEvent.KEYCODE_DPAD_RIGHT -> joyPad.right = if (joyPad.right) false else on
            else -> return super.dispatchKeyEvent(event)
        }
        // TODO: 物理ボタンのキー変化をバーチャルパッドの見た目に反映した方が良いかも
        return true
    }

    override fun dispatchTouchEvent(ev: MotionEvent?) = super.dispatchTouchEvent(ev)
    override fun emulatorViewRequireAssets(): AssetManager = assets
    override fun emulatorViewRequirePadCode() = virtualJoyPad.code.or(joyPad.code)
}