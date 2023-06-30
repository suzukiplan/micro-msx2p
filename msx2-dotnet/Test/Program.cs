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
            }

            Console.WriteLine("Release micro-msx2p context");
            MSX2.Core.ReleaseContext(context);
        }
    }
}
