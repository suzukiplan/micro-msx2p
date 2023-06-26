/**
 * micro MSX2+ - Android Example
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

import android.annotation.SuppressLint
import android.util.SparseArray
import android.view.MotionEvent
import android.view.View
import android.widget.ImageView
import androidx.core.util.forEach
import com.suzukiplan.msx2.JoyPad

class VirtualJoyPad(padContainer: View) : View.OnTouchListener {
    private enum class TouchArea {
        DPad,
        Button,
        Select,
        Start
    }

    private data class TouchInfo(
        var x: Int,
        var y: Int,
        val area: TouchArea
    )

    private val touches = SparseArray<TouchInfo>()
    private val dpad = padContainer.findViewById<View>(R.id.pad_dpad_container)
    private val up = padContainer.findViewById<ImageView>(R.id.pad_up)
    private val down = padContainer.findViewById<ImageView>(R.id.pad_down)
    private val left = padContainer.findViewById<ImageView>(R.id.pad_left)
    private val right = padContainer.findViewById<ImageView>(R.id.pad_right)
    private val buttons = padContainer.findViewById<View>(R.id.pad_btn_container)
    private val both = padContainer.findViewById<View>(R.id.pad_btn_double)
    private val a = padContainer.findViewById<ImageView>(R.id.pad_btn_a)
    private val b = padContainer.findViewById<ImageView>(R.id.pad_btn_b)
    private val select = padContainer.findViewById<ImageView>(R.id.pad_ctrl_select)
    private val start = padContainer.findViewById<ImageView>(R.id.pad_ctrl_start)
    private val inputStatus = JoyPad(
        up = false,
        down = false,
        left = false,
        right = false,
        a = false,
        b = false,
        select = false,
        start = false
    )
    val code: Int get() = inputStatus.code

    private fun checkArea(x: Int, y: Int): TouchArea? {
        return if (dpad.left < x && dpad.top < y && x < dpad.right && y < dpad.bottom) {
            TouchArea.DPad
        } else if (buttons.left < x && buttons.top < y && x < buttons.right && y < buttons.bottom) {
            TouchArea.Button
        } else if (select.left < x && select.top < y && x < select.right && y < select.bottom) {
            TouchArea.Select
        } else if (start.left < x && start.top < y && x < start.right && y < start.bottom) {
            TouchArea.Start
        } else {
            null
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouch(v: View?, event: MotionEvent?): Boolean {
        when (event?.actionMasked) {
            MotionEvent.ACTION_DOWN,
            MotionEvent.ACTION_POINTER_DOWN,
            MotionEvent.ACTION_BUTTON_PRESS -> {
                val id = event.getPointerId(event.actionIndex)
                try {
                    val x = event.getX(id).toInt()
                    val y = event.getY(id).toInt()
                    val area = checkArea(x, y)
                    if (null != area) {
                        touches.append(id, TouchInfo(x, y, area))
                    }
                } catch (e: IllegalArgumentException) {
                    // ignore
                }
            }
            MotionEvent.ACTION_MOVE -> {
                for (index in 0 until event.pointerCount) {
                    val id = event.getPointerId(index)
                    touches[id]?.x = event.getX(index).toInt()
                    touches[id]?.y = event.getY(index).toInt()
                }
            }
            MotionEvent.ACTION_UP,
            MotionEvent.ACTION_CANCEL,
            MotionEvent.ACTION_POINTER_UP,
            MotionEvent.ACTION_BUTTON_RELEASE -> {
                val id = event.getPointerId(event.actionIndex)
                if (touches.get(id) != null) {
                    touches.remove(id)
                }
            }
        }
        val previous = inputStatus.copy()
        inputStatus.up = false
        inputStatus.down = false
        inputStatus.left = false
        inputStatus.right = false
        inputStatus.a = false
        inputStatus.b = false
        inputStatus.select = false
        inputStatus.start = false
        touches.forEach { _, value ->
            when (value.area) {
                TouchArea.DPad -> checkDPad(value.x, value.y)
                TouchArea.Button -> checkButton(value.x)
                TouchArea.Select -> inputStatus.select = true
                TouchArea.Start -> inputStatus.start = true
            }
        }
        if (previous.up != inputStatus.up) {
            if (inputStatus.up) {
                up.setImageResource(R.drawable.pad_up_on)
            } else {
                up.setImageResource(R.drawable.pad_up_off)
            }
        }
        if (previous.down != inputStatus.down) {
            if (inputStatus.down) {
                down.setImageResource(R.drawable.pad_down_on)
            } else {
                down.setImageResource(R.drawable.pad_down_off)
            }
        }
        if (previous.left != inputStatus.left) {
            if (inputStatus.left) {
                left.setImageResource(R.drawable.pad_left_on)
            } else {
                left.setImageResource(R.drawable.pad_left_off)
            }
        }
        if (previous.right != inputStatus.right) {
            if (inputStatus.right) {
                right.setImageResource(R.drawable.pad_right_on)
            } else {
                right.setImageResource(R.drawable.pad_right_off)
            }
        }
        if (previous.a != inputStatus.a) {
            if (inputStatus.a) {
                a.setImageResource(R.drawable.pad_btn_on)
            } else {
                a.setImageResource(R.drawable.pad_btn_off)
            }
        }
        if (previous.b != inputStatus.b) {
            if (inputStatus.b) {
                b.setImageResource(R.drawable.pad_btn_on)
            } else {
                b.setImageResource(R.drawable.pad_btn_off)
            }
        }
        if (previous.select != inputStatus.select) {
            if (inputStatus.select) {
                select.setImageResource(R.drawable.pad_ctrl_on)
            } else {
                select.setImageResource(R.drawable.pad_ctrl_off)
            }
        }
        if (previous.start != inputStatus.start) {
            if (inputStatus.start) {
                start.setImageResource(R.drawable.pad_ctrl_on)
            } else {
                start.setImageResource(R.drawable.pad_ctrl_off)
            }
        }
        return true
    }

    private fun checkDPad(x: Int, y: Int) {
        val cx = dpad.left + dpad.width / 2.0
        val cy = dpad.top + dpad.height / 2.0
        var r = kotlin.math.atan2(x - cx, y - cy) + Math.PI / 2.0
        while (Math.PI * 2 <= r) r -= Math.PI * 2
        while (r < 0) r += Math.PI * 2
        inputStatus.down = r in 0.5235987755983..2.6179938779915
        inputStatus.right = r in 2.2689280275926..4.1887902047864
        inputStatus.up = r in 3.6651914291881..5.7595865315813
        inputStatus.left = r <= 1.0471975511966 || 5.235987755983 <= r
    }

    private fun checkButton(x: Int) {
        if (x < buttons.left + both.left) {
            inputStatus.b = true
        } else if (buttons.left + both.right < x) {
            inputStatus.a = true
        } else {
            inputStatus.a = true
            inputStatus.b = true
        }
    }
}