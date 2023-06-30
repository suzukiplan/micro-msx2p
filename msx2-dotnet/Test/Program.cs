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
    struct BitmapHeader {
        public int isize;
        public int width;
        public int height;
        public ushort planes;
        public ushort bits;
        public uint ctype;
        public uint gsize;
        public int xppm;
        public int yppm;
        public uint cnum;
        public uint inum;
    }

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
            Console.WriteLine($"Tick {tickCount} times");
            for (int i = 0; i < tickCount; i++) {
                MSX2.Core.Tick(context, 0, 0, 0);
                MSX2.Core.GetSound(context); // Unnecessary, but I'll getSound it off anyway...
            }

            int width = MSX2.Core.GetDisplayWidth(context);
            int height = MSX2.Core.GetDisplayHeight(context);
            Console.WriteLine($"Get Display ({width}x{height})");
            ushort[] display = MSX2.Core.GetDisplay(context);

            Console.WriteLine("Convert display to Bitmap");
            BitmapHeader header = new BitmapHeader();
            header.isize = 40;
            header.width = width;
            header.height = height * 2;
            header.planes = 1;
            header.bits = 32;
            header.ctype = 0;
            header.gsize = (uint)(header.width * header.height * 4);
            header.xppm = 1;
            header.yppm = 1;
            header.cnum = 0;
            header.inum = 0;
            byte[] bitmap = new byte[header.gsize];
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    ushort rgb565 = display[(height - y - 1) * width + x];
                    byte r = (byte)((rgb565 >> 11) & 0x1F);
                    byte g = (byte)((rgb565 >> 5) & 0x3F);
                    byte b = (byte)(rgb565 & 0x1F);
                    r = (byte)((r << 3) | (r >> 2));
                    g = (byte)((g << 2) | (g >> 4));
                    b = (byte)((b << 3) | (b >> 2));
                    bitmap[y * 2 * width * 4 + x * 4 + 0] = b;
                    bitmap[y * 2 * width * 4 + x * 4 + 1] = g;
                    bitmap[y * 2 * width * 4 + x * 4 + 2] = r;
                    bitmap[y * 2 * width * 4 + x * 4 + 3] = 0;
                    bitmap[(y * 2 + 1) * width * 4 + x * 4 + 0] = b;
                    bitmap[(y * 2 + 1) * width * 4 + x * 4 + 1] = g;
                    bitmap[(y * 2 + 1) * width * 4 + x * 4 + 2] = r;
                    bitmap[(y * 2 + 1) * width * 4 + x * 4 + 3] = 0;
                }
            }

            Console.WriteLine("Writing result.bmp");
            var writer = new BinaryWriter(new FileStream("result.bmp", FileMode.Create));
            try {
                writer.Write((byte)'B');
                writer.Write((byte)'M');
                writer.Write((int)(14 + header.isize + header.gsize));
                writer.Write(0);
                writer.Write(14 + header.isize);
                writer.Write(header.isize);
                writer.Write(header.width);
                writer.Write(header.height);
                writer.Write(header.planes);
                writer.Write(header.bits);
                writer.Write(header.ctype);
                writer.Write(header.gsize);
                writer.Write(header.xppm);
                writer.Write(header.yppm);
                writer.Write(header.cnum);
                writer.Write(header.inum);
                writer.Write(bitmap, 0, (int)header.gsize);
            } finally {
                writer.Close();
            }

            Console.WriteLine("Release micro-msx2p context");
            MSX2.Core.ReleaseContext(context);
        }
    }
}
