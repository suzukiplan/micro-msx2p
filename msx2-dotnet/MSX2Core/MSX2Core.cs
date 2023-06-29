using System;
using System.Runtime.InteropServices;

public class MSX2Core
{
    [DllImport("MSX2", EntryPoint="msx2_createContext")]
    public static extern IntPtr CreateContext(int colorMode);

    [DllImport("MSX2", EntryPoint="msx2_releaseContext")]
    public static extern IntPtr ReleaseContext(IntPtr context);
}
