package com.suzukiplan.msx2_android

import android.content.res.AssetManager
import android.os.Bundle
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

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        val padContainer = findViewById<View>(R.id.pad_container)
        virtualJoyPad = VirtualJoyPad(padContainer)
        padContainer.setOnTouchListener(virtualJoyPad)
        findViewById<EmulatorView>(R.id.emulator).delegate = this
    }

    override fun dispatchTouchEvent(ev: MotionEvent?) = super.dispatchTouchEvent(ev)
    override fun emulatorViewRequireAssets(): AssetManager = assets
    override fun emulatorViewRequirePadCode() = virtualJoyPad.code
}