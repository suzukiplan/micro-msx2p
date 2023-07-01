/**
 * micro MSX2+ - Simple Test Program for C#
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
using System;
using System.IO;
using MSX2;

namespace Test
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Create micro-msx2p context");
            IntPtr context = MSX2.Core.CreateContext(MSX2.ColorMode.RGB565);

            Console.WriteLine("SetupSecondaryExist: 0 0 0 1");
            MSX2.Core.SetupSecondaryExist(context, false, false, false, true);

            byte[] romMain = File.ReadAllBytes("roms/cbios_main_msx2+_jp.rom");
            Console.WriteLine($"Setup: MAIN 0-0 $0000~$7FFF ({romMain.Length} bytes)");
            MSX2.Core.Setup(context, 0, 0, 0, romMain, "MAIN");

            byte[] romLogo = File.ReadAllBytes("roms/cbios_logo_msx2+.rom");
            Console.WriteLine($"Setup: LOGO 0-0 $8000~$BFFF ({romLogo.Length} bytes)");
            MSX2.Core.Setup(context, 0, 0, 4, romLogo, "LOGO");

            byte[] romSub = File.ReadAllBytes("roms/cbios_sub.rom");
            Console.WriteLine($"Setup: SUB  3-0 $0000~$3FFF ({romSub.Length} bytes)");
            MSX2.Core.Setup(context, 3, 0, 0, romSub, "SUB");

            Console.WriteLine("Setup: RAM  3-3 $0000~$FFFF");
            MSX2.Core.SetupRam(context, 3, 3);

            Console.WriteLine("Setup: Special Key Code (Select=ESC, Start=SPACE)");
            MSX2.Core.SetupSpecialKeyCode(context, 0x1B, 0x20);

            byte[] romGame = File.ReadAllBytes("roms/game.rom");
            MSX2.RomType type = MSX2.RomType.Normal;
            Console.WriteLine($"Load ROM ({romGame.Length} bytes, type={type})");
            MSX2.Core.LoadRom(context, romGame, type);

            Console.WriteLine("Reset");
            MSX2.Core.Reset(context);

            const int tickCount = 600;
            SimpleWave wave = new SimpleWave(44100, 16, 2);
            wave.SetAutoHeadDetection(true);
            Console.WriteLine($"Tick {tickCount} times");
            for (int i = 0; i < tickCount; i++) {
                MSX2.Core.Tick(context, 0, 0, 0);
                wave.Append(MSX2.Core.GetSound(context));
            }

            int width = MSX2.Core.GetDisplayWidth(context);
            int height = MSX2.Core.GetDisplayHeight(context);
            Console.WriteLine($"Get Display ({width}x{height})");
            ushort[] display = MSX2.Core.GetDisplay(context);

            Console.WriteLine("Convert display to Bitmap");
            SimpleBitmap bitmap = new SimpleBitmap(width, height * 2);
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    ushort rgb565 = display[y * width + x];
                    bitmap.SetPixelRGB565(x, y * 2, rgb565);
                    bitmap.SetPixelRGB565(x, y * 2 + 1, rgb565);
                }
            }

            Console.WriteLine("Writing result.bmp");
            bitmap.WriteFile("result.bmp");

            Console.Write("Writing result.wav ... ");
            if (wave.WriteFile("result.wav")) {
                Console.WriteLine("wrote");
            } else {
                Console.WriteLine("did not write (no sound)");
            }

            Console.WriteLine("Release micro-msx2p context");
            MSX2.Core.ReleaseContext(context);
        }
    }
}
