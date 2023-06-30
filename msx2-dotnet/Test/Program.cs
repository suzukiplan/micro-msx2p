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
            IntPtr context = MSX2.Core.CreateContext(MSX2.ColorMode.RGB555);

            Console.WriteLine("SetupSecondaryExist: 0 0 0 1");
            MSX2.Core.SetupSecondaryExist(context, false, false, false, true);

            Console.WriteLine("Setup: MAIN");
            byte[] romMain = File.ReadAllBytes("roms/cbios_main_msx2+_jp.rom");
            MSX2.Core.Setup(context, 0, 0, 0, romMain, "MAIN");

            Console.WriteLine("Setup: LOGO");
            byte[] romLogo = File.ReadAllBytes("roms/cbios_logo_msx2+.rom");
            MSX2.Core.Setup(context, 0, 0, 4, romLogo, "LOGO");

            Console.WriteLine("Setup: SUB");
            byte[] romSub = File.ReadAllBytes("roms/cbios_sub.rom");
            MSX2.Core.Setup(context, 3, 0, 0, romLogo, "SUB");

            Console.WriteLine("Setup: RAM");
            MSX2.Core.SetupRam(context, 3, 3);

            Console.WriteLine("Setup: Special Key Code");
            MSX2.Core.SetupSpecialKeyCode(context, 0x1B, 0x20);

            Console.WriteLine("Load ROM");
            byte[] romGame = File.ReadAllBytes("roms/game.rom");
            MSX2.Core.LoadRom(context, romGame, MSX2.RomType.Normal);

            Console.WriteLine("Reset");
            MSX2.Core.Reset(context);

            Console.WriteLine("Release micro-msx2p context");
            MSX2.Core.ReleaseContext(context);
        }
    }
}
