using System;
using MSX2;

namespace Test
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Create micro-msx2p context");
            IntPtr context = MSX2.Core.CreateContext(0);

            Console.WriteLine("Release micro-msx2p context");
            MSX2.Core.ReleaseContext(context);
        }
    }
}
