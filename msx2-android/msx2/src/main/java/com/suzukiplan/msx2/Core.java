/*
 * micro MSX2+ - Core Module JNI
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
package com.suzukiplan.msx2;

import android.graphics.Bitmap;

public class Core {
    /**
     * initialize emulator context
     *
     * @return emulator context
     */
    public static native long init();

    /**
     * terminate emulator context
     *
     * @param context emulator context
     */
    public static native void term(long context);

    /**
     * Setup secondary slot exist
     *
     * @param context emulator context
     * @param page0   0x0000 ~ 0x3FFF
     * @param page1   0x4000 ~ 0x7FFF
     * @param page2   0x8000 ~ 0xBFFF
     * @param page3   0xC000 ~ 0xFFFF
     */
    public static native void setupSecondaryExist(long context,
                                                  boolean page0,
                                                  boolean page1,
                                                  boolean page2,
                                                  boolean page3);

    /**
     * setup RAM page
     *
     * @param context emulator context
     * @param pri     number of primary slot
     * @param sec     number of secondary slot
     */
    public static native void setupRAM(long context, int pri, int sec);

    /**
     * setup BIOS
     *
     * @param context emulator context
     * @param pri     number of primary slot
     * @param sec     number of secondary slot
     * @param idx     index of page (address ÷ 8KB)
     * @param data    BIOS data
     * @param label   label of BIOS
     */
    public static native void setup(long context,
                                    int pri,
                                    int sec,
                                    int idx,
                                    byte[] data,
                                    String label);

    /**
     * Load KNJFNT16
     *
     * @param context emulator context
     * @param font    font data
     */
    public static native void loadFont(long context, byte[] font);

    /**
     * Setup key assign of SELECT/START buttons
     *
     * @param context emulator context
     * @param select  key code of SELECT button
     * @param start   key code of START button
     */
    public static native void setupSpecialKeyCode(long context,
                                                  int select,
                                                  int start);

    /**
     * Tick the emulator one frame.
     *
     * @param context emulator context
     * @param pad1    Player-1 JoyPad code
     * @param pad2    Player-2 JoyPad code
     * @param key     Key code
     * @param vram    VRAM after tick (568x240 RGB565)
     */
    public static native void tick(long context,
                                   int pad1,
                                   int pad2,
                                   int key,
                                   Bitmap vram);

    /**
     * Load ROM data
     *
     * @param context emulator context
     * @param rom     ROM data
     * @param romType value of RomType
     */
    public static native void loadRom(long context,
                                      byte[] rom,
                                      int romType);

    /**
     * Insert Floppy Disk
     *
     * @param context  emulator context
     * @param driveId  drive identifier
     * @param disk     disk image
     * @param sha256   SHA256 of disk image
     * @param readOnly read only flag
     */
    public static native void insertDisk(long context,
                                         int driveId,
                                         byte[] disk,
                                         String sha256,
                                         boolean readOnly);

    /**
     * Eject Floppy Disk
     *
     * @param context emulator context
     * @param driveId drive identifier
     */
    public static native void ejectDisk(long context, int driveId);

    /**
     * Quick Save
     *
     * @param context emulator context
     * @return quick save data
     */
    public static native byte[] quickSave(long context);

    /**
     * Quick Load
     *
     * @param context emulator context
     * @param save    quick save data
     */
    public static native void quickLoad(long context, byte[] save);

    /**
     * Reset
     *
     * @param context emulator context
     */
    public static native void reset(long context);
}
