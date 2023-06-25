/*
 * micro MSX2+ - MSX2View for Android
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
package com.suzukiplan.msx2

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Paint
import android.graphics.Rect
import android.os.Build
import android.util.AttributeSet
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import java.security.MessageDigest
import kotlin.math.abs

class MSX2View(context: Context, attribute: AttributeSet) : SurfaceView(context, attribute),
    SurfaceHolder.Callback, Runnable {
    companion object {
        init {
            System.loadLibrary("msx2")
        }
    }

    private val vramWidth = 568
    private val vramHeight = 480
    private val vramRect = Rect(0, 0, vramWidth, vramHeight / 2)
    private val viewRect = Rect(0, 0, 0, 0)
    private val drawRect = Rect(0, 0, 0, 0)
    private val paint = Paint()
    private var aliveSubThread = false
    private var subThread: Thread? = null
    private var paused = true
    private var emulatorContext = 0L
    private var onPause: OnPauseListener? = null

    @Suppress("MemberVisibilityCanBePrivate")
    var delegatePad1: DelegatePad1? = null

    interface DelegatePad1 {
        fun msx2ViewDidRequirePad1Code(): Int
    }

    interface OnPauseListener {
        fun onPause()
    }

    init {
        holder.addCallback(this)
    }

    @Suppress("unused")
    fun setupCBios(main: ByteArray, logo: ByteArray, sub: ByteArray) {
        pause(object : OnPauseListener {
            override fun onPause() {
                setupSecondaryExist(page0 = false, page1 = false, page2 = false, page3 = true)
                setup(0, 0, 0, main, "MAIN")
                setup(0, 0, 4, logo, "LOGO")
                setup(3, 0, 0, sub, "SUB")
                setupRAM(3, 3)
            }
        })
    }

    @Suppress("MemberVisibilityCanBePrivate")
    fun setupSecondaryExist(page0: Boolean, page1: Boolean, page2: Boolean, page3: Boolean) {
        pause(object : OnPauseListener {
            override fun onPause() {
                Core.setupSecondaryExist(emulatorContext, page0, page1, page2, page3)
            }
        })
    }

    @Suppress("MemberVisibilityCanBePrivate")
    fun setupRAM(pri: Int, sec: Int) {
        pause(object : OnPauseListener {
            override fun onPause() {
                Core.setupRAM(emulatorContext, pri, sec)
            }
        })
    }

    @Suppress("MemberVisibilityCanBePrivate")
    fun setup(pri: Int, sec: Int, idx: Int, data: ByteArray, label: String) {
        pause(object : OnPauseListener {
            override fun onPause() {
                Core.setup(emulatorContext, pri, sec, idx, data, label)
            }
        })
    }

    @Suppress("unused")
    fun loadFont(font: ByteArray) {
        pause(object : OnPauseListener {
            override fun onPause() {
                Core.loadFont(emulatorContext, font)
            }
        })
    }

    @Suppress("unused")
    fun setupSpecialKeyCode(select: Int, start: Int) {
        pause(object : OnPauseListener {
            override fun onPause() {
                Core.setupSpecialKeyCode(emulatorContext, select, start)
            }
        })
    }

    @Suppress("unused")
    fun loadRom(rom: ByteArray, type: RomType) {
        Core.loadRom(emulatorContext, rom, type.value)
    }

    private fun makeSHA256(bytes: ByteArray): String {
        val md = MessageDigest.getInstance("SHA-256")
        val digest = md.digest(bytes)
        return digest.fold("") { str, it -> str + "%02x".format(it) }
    }

    @Suppress("unused")
    fun insertDisk(driveId: Int, disk: ByteArray, readOnly: Boolean) {
        Core.insertDisk(emulatorContext, driveId, disk, makeSHA256(disk), readOnly)
    }

    @Suppress("unused")
    fun ejectDisk(driveId: Int) {
        Core.ejectDisk(emulatorContext, driveId)
    }

    @Suppress("unused")
    fun quickSave(): ByteArray? {
        return Core.quickSave(emulatorContext)
    }

    @Suppress("unused")
    fun quickLoad(save: ByteArray) {
        Core.quickLoad(emulatorContext, save)
    }

    fun reset() {
        Core.reset(emulatorContext)
        resume()
    }

    @Suppress("MemberVisibilityCanBePrivate")
    fun pause(listener: OnPauseListener) {
        if (paused) {
            listener.onPause()
            return
        }
        paused = true
        onPause = listener
    }

    fun resume() {
        paused = false
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            holder.surface.setFrameRate(60.0f, Surface.FRAME_RATE_COMPATIBILITY_FIXED_SOURCE)
        }
        startSubThread()
    }

    override fun surfaceDestroyed(p0: SurfaceHolder) {
        stopSubThread()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        viewRect.set(0, 0, width, height)
        val scale = if (width.toDouble() / vramWidth < height.toDouble() / vramHeight) {
            width.toDouble() / vramWidth
        } else {
            height.toDouble() / vramHeight
        }
        val drawWidth = (vramWidth * scale).toInt()
        val drawHeight = (vramHeight * scale).toInt()
        val x = abs((drawWidth - width) / 2)
        val y = abs((drawHeight - height) / 2)
        drawRect.set(x, y, x + drawWidth, y + drawHeight)
    }

    private fun startSubThread() {
        aliveSubThread = true
        subThread = Thread(this)
        subThread?.start()
    }

    private fun stopSubThread() {
        aliveSubThread = false
        subThread?.join()
        paused = true
    }

    override fun run() {
        val vram = Bitmap.createBitmap(vramWidth, vramHeight / 2, Bitmap.Config.RGB_565)
        var currentInterval = 0
        val intervals = intArrayOf(17, 17, 16)
        val minInterval = 16
        while (aliveSubThread) {
            val start = System.currentTimeMillis()
            if (!paused) {
                Core.tick(
                    emulatorContext,
                    delegatePad1?.msx2ViewDidRequirePad1Code() ?: 0,
                    0,
                    0,
                    vram
                )
                val canvas = holder.lockHardwareCanvas()
                if (null != canvas) {
                    canvas.drawBitmap(vram, vramRect, drawRect, paint)
                    holder.unlockCanvasAndPost(canvas)
                }
            } else {
                onPause?.onPause()
                onPause = null
            }
            val procTime = System.currentTimeMillis() - start
            currentInterval++
            currentInterval %= intervals.size
            if (procTime < minInterval) {
                Thread.sleep(intervals[currentInterval] - procTime)
            }
        }
        vram.recycle()
    }

    override fun onAttachedToWindow() {
        super.onAttachedToWindow()
        emulatorContext = Core.init()
    }

    override fun onDetachedFromWindow() {
        Core.term(emulatorContext)
        super.onDetachedFromWindow()
    }
}