/**
 * micro MSX2+ - SimpleWave class
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
using System.Collections.Generic;

namespace Test {
    struct WaveHeader {
        public string riff;
        public uint fsize;
        public string wave;
        public string fmt;
        public uint bnum;
        public ushort fid;
        public ushort ch;
        public uint sample;
        public uint bps;
        public ushort bsize;
        public ushort bits;
        public string data;
        public uint dsize;
    }

    class SimpleWave {
        private WaveHeader header = new WaveHeader();
        private List<byte[]> pcmList = new List<byte[]>();
        private bool autoHeadDetection = false;
        private bool detectHead = false;

        public SimpleWave(int sample, int bits, int ch) {
            header.riff = "RIFF";
            header.wave = "WAVE";
            header.fmt = "fmt ";
            header.data = "data";
            header.fsize = (uint)0;
            header.bnum = (uint)16; // linear PCM
            header.fid = (ushort)1; // uncompressed
            header.ch = (ushort)ch;
            header.sample = (uint)sample;
            header.bits = (ushort)bits;
            header.bsize = (ushort)(ch * bits / 8);
            header.bps = (uint)(header.bsize * header.sample);
            header.dsize = (uint)0;
        }

        // true if you want to ignore silence PCM appends until the first non-silent PCM is detected
        public void SetAutoHeadDetection(bool value) {
            autoHeadDetection = value;
        }

        public void Append(byte[] pcm)
        {
            // skip if undetected head when autoHeadDetection mode
            if (autoHeadDetection && !detectHead) {
                bool silent = true;
                foreach (byte p in pcm) {
                    if (0x00 != p) {
                        silent = false;
                        break;
                    }
                }
                if (silent) {
                    return;
                }
                detectHead = true;
            }
            pcmList.Add(pcm);
            header.dsize += (uint)pcm.Length;
        }

        public bool WriteFile(string path) {
            if (header.dsize < 1) {
                return false; // no sound
            }
            header.fsize = (uint)(header.dsize + 44 - 8);
            var writer = new BinaryWriter(new FileStream(path, FileMode.Create));
            try {
                writer.Write(System.Text.Encoding.ASCII.GetBytes(header.riff));
                writer.Write(header.fsize);
                writer.Write(System.Text.Encoding.ASCII.GetBytes(header.wave));
                writer.Write(System.Text.Encoding.ASCII.GetBytes(header.fmt));
                writer.Write(header.bnum);
                writer.Write(header.fid);
                writer.Write(header.ch);
                writer.Write(header.sample);
                writer.Write(header.bps);
                writer.Write(header.bsize);
                writer.Write(header.bits);
                writer.Write(System.Text.Encoding.ASCII.GetBytes(header.data));
                writer.Write(header.dsize);
                foreach (byte[] pcm in pcmList) {
                    writer.Write(pcm, 0, pcm.Length);
                }
            } finally {
                writer.Close();
            }
            return true;
        }
    }
}