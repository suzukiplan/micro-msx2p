package com.suzukiplan.msx2_android

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
import kotlin.random.Random

class EmulatorView(context: Context, attribute: AttributeSet) : SurfaceView(context, attribute),
    SurfaceHolder.Callback, Runnable {
    private val vramWidth = 568
    private val vramHeight = 480
    private val vramRect = Rect(0, 0, vramWidth, vramHeight / 2)
    private val viewRect = Rect(0, 0, 0, 0)
    private val drawRect = Rect(0, 0, 0, 0)
    private lateinit var vram: Bitmap
    private val paint = Paint()
    private var aliveSubThread = false
    private var subThread: Thread? = null
    private val rand = Random(0)
    var delegate: Delegate? = null

    interface Delegate {
        fun getJoyPadCode(): Int
    }

    init {
        holder.addCallback(this)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            holder.surface.setFrameRate(60.0f, Surface.FRAME_RATE_COMPATIBILITY_FIXED_SOURCE)
        }
        vram = Bitmap.createBitmap(vramWidth, vramHeight / 2, Bitmap.Config.RGB_565)
        // TODO: initialize emulator
        startSubThread()
    }

    override fun surfaceDestroyed(p0: SurfaceHolder) {
        stopSubThread()
        // TODO: terminate emulator
        vram.recycle()
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
        if (aliveSubThread) return
        aliveSubThread = true
        subThread = Thread(this)
        subThread?.start()
    }

    private fun stopSubThread() {
        if (!aliveSubThread) return
        aliveSubThread = false
        subThread?.join()
    }

    override fun run() {
        while (aliveSubThread) {
            val canvas = holder.lockHardwareCanvas()
            // TODO: call tick and native-rendering procedure
            for (n in 0 until 256) {
                vram.setPixel(
                    rand.nextInt(0, vramWidth),
                    rand.nextInt(0, vramHeight / 2),
                    rand.nextInt()
                )
            }
            canvas.drawBitmap(vram, vramRect, drawRect, paint)
            holder.unlockCanvasAndPost(canvas)
            Thread.sleep(100L)
        }
    }
}