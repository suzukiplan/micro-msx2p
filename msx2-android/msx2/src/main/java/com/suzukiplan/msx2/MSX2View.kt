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
    private var paused = false
    private var onPause: OnPauseListener? = null
    private var bootInfo: BootInfo? = null

    @Suppress("MemberVisibilityCanBePrivate")
    var delegate: Delegate? = null

    interface Delegate {
        fun msx2ViewDidRequirePad1Code(): Int
        fun msx2ViewDidStart()
        fun msx2ViewDidStop()
    }

    interface OnPauseListener {
        fun onPause()
    }

    private class BootInfo(
        var context: Long,
        var keySelect: Int,
        var keyStart: Int,
        var main: ByteArray?,
        var logo: ByteArray?,
        var sub: ByteArray?,
        var rom: ByteArray?,
        var romType: RomType
    )

    init {
        holder.addCallback(this)
    }

    /**
     * initialize emulator
     * @param keySelect key code of SELECT button
     * @param keyStart key code of START button
     * @param main C-BIOS main ROM
     * @param logo C-BIOS logo ROM
     * @param sub C-BIOS sub ROM
     * @param rom ROM data
     * @param rom Mega-ROM Type of ROM data
     */
    fun initialize(
        keySelect: Int,
        keyStart: Int,
        main: ByteArray,
        logo: ByteArray,
        sub: ByteArray,
        rom: ByteArray,
        romType: RomType
    ) {
        bootInfo = BootInfo(0L, keySelect, keyStart, main, logo, sub, rom, romType)
    }

    /**
     * Quick save all state of the emulator
     * @return Save data
     */
    fun quickSave(): ByteArray? {
        val context = bootInfo?.context ?: return null
        return Core.quickSave(context)
    }

    /**
     * Quick load all state of the emulator
     * @param save Save data
     */
    fun quickLoad(save: ByteArray) {
        val context = bootInfo?.context ?: return
        Core.quickLoad(context, save)
    }

    /**
     * Reset emulator
     */
    fun reset() {
        val context = bootInfo?.context ?: return
        Core.reset(context)
    }

    /**
     * Pause the emulation
     * @param listener Callback after paused
     */
    fun pause(listener: OnPauseListener?) {
        if (paused) {
            listener?.onPause()
        } else {
            paused = true
            onPause = listener
        }
    }

    /**
     * Resume the emulation
     */
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
    }

    override fun run() {
        while (null == bootInfo && aliveSubThread) {
            Thread.sleep(100)
        }
        bootInfo?.context = Core.init()
        val context = bootInfo?.context ?: 0L
        Core.setupSecondaryExist(context, false, false, false, true)
        Core.setup(context, 0, 0, 0, bootInfo?.main, "MAIN")
        Core.setup(context, 0, 0, 4, bootInfo?.logo, "LOGO")
        Core.setup(context, 3, 0, 0, bootInfo?.sub, "SUB")
        Core.setupRAM(context, 3, 3)
        Core.setupSpecialKeyCode(context, bootInfo?.keySelect ?: 0, bootInfo?.keyStart ?: 0)
        Core.loadRom(context, bootInfo?.rom, bootInfo?.romType?.value ?: 0)
        Core.reset(context)
        val vram = Bitmap.createBitmap(vramWidth, vramHeight / 2, Bitmap.Config.RGB_565)
        var currentInterval = 0
        val intervals = intArrayOf(17, 17, 16)
        val minInterval = 16
        delegate?.msx2ViewDidStart()
        while (aliveSubThread) {
            val start = System.currentTimeMillis()
            if (!paused) {
                Core.tick(
                    context,
                    delegate?.msx2ViewDidRequirePad1Code() ?: 0,
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
        delegate?.msx2ViewDidStop()
        vram.recycle()
        Core.term(context)
    }
}