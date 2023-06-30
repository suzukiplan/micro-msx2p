/**
 * micro MSX2+ - SimpleBitmap class
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

namespace Test {
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

    class SimpleBitmap {
        private BitmapHeader header = new BitmapHeader();
        private byte[] bitmap = null;

        public SimpleBitmap(int width, int height) {
            header.isize = 40;
            header.width = width;
            header.height = height;
            header.planes = 1;
            header.bits = 32;
            header.ctype = 0;
            header.gsize = (uint)(header.width * header.height * 4);
            header.xppm = 1;
            header.yppm = 1;
            header.cnum = 0;
            header.inum = 0;
            bitmap = new byte[header.gsize];
        }

        public void SetPixelRGB565(int x, int y, ushort rgb565) {
            if (x < 0 || y < 0 || header.width <= x || header.height <= y) return;
            y = header.height - 1 - y;
            byte r = (byte)((rgb565 >> 11) & 0x1F);
            byte g = (byte)((rgb565 >> 5) & 0x3F);
            byte b = (byte)(rgb565 & 0x1F);
            r = (byte)((r << 3) | (r >> 2));
            g = (byte)((g << 2) | (g >> 4));
            b = (byte)((b << 3) | (b >> 2));
            int ptr = y * header.width * 4 + x * 4;
            bitmap[ptr + 0] = b;
            bitmap[ptr + 1] = g;
            bitmap[ptr + 2] = r;
            bitmap[ptr + 3] = 0;
        }

        public void WriteFile(string path) {
            var writer = new BinaryWriter(new FileStream(path, FileMode.Create));
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
        }
    }
}