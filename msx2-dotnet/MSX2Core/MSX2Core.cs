using System;
using System.Runtime.InteropServices;

namespace MSX2 {
    public class Core
    {
        [DllImport("libMSX2.so", EntryPoint="msx2_createContext")]
        public static extern IntPtr CreateContext(int colorMode);

        [DllImport("libMSX2.so", EntryPoint="msx2_releaseContext")]
        public static extern IntPtr ReleaseContext(IntPtr context);
    }
}
