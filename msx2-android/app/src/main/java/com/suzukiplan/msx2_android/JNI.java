package com.suzukiplan.msx2_android;

import android.graphics.Bitmap;

public class JNI {
    public static native void init(byte[] main, byte[] logo, byte[] sub);

    public static native void term();

    public static native void tick(int pad, Bitmap vram);

    public static native void loadRom(byte[] rom);
}
