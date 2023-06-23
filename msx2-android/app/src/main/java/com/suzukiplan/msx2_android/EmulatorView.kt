package com.suzukiplan.msx2_android

import android.content.Context
import android.content.res.AssetManager
import android.graphics.Bitmap
import android.graphics.Paint
import android.graphics.Rect
import android.os.Build
import android.util.AttributeSet
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import kotlin.math.abs

class EmulatorView(context: Context, attribute: AttributeSet) : SurfaceView(context, attribute),
    SurfaceHolder.Callback, Runnable {
    private val vramWidth = 568
    private val vramHeight = 480
    private val vramRect = Rect(0, 0, vramWidth, vramHeight / 2)
    private val viewRect = Rect(0, 0, 0, 0)
    private val drawRect = Rect(0, 0, 0, 0)
    private val paint = Paint()
    private var aliveSubThread = false
    private var subThread: Thread? = null
    var delegate: Delegate? = null

    interface Delegate {
        fun emulatorViewRequirePadCode(): Int
        fun emulatorViewRequireAssets(): AssetManager
    }

    init {
        holder.addCallback(this)
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
        val vram = Bitmap.createBitmap(vramWidth, vramHeight / 2, Bitmap.Config.RGB_565)
        val assets = delegate?.emulatorViewRequireAssets() ?: return
        JNI.init(
            assets.open("cbios_main_msx2+_jp.rom").readBytes(),
            assets.open("cbios_logo_msx2+.rom").readBytes(),
            assets.open("cbios_sub.rom").readBytes()
        )
        JNI.loadRom(assets.open("game.rom").readBytes())
        var currentInterval = 0
        val intervals = intArrayOf(17, 17, 16)
        while (aliveSubThread) {
            val start = System.currentTimeMillis()
            JNI.tick(delegate?.emulatorViewRequirePadCode() ?: 0, vram)
            val canvas = holder.lockHardwareCanvas()
            if (null != canvas) {
                canvas.drawBitmap(vram, vramRect, drawRect, paint)
                holder.unlockCanvasAndPost(canvas)
            }
            val procTime = System.currentTimeMillis() - start
            currentInterval++
            currentInterval %= intervals.size
            if (procTime < intervals[currentInterval]) {
                Thread.sleep(intervals[currentInterval] - procTime)
            }
        }
        JNI.term()
        vram.recycle()
    }
}