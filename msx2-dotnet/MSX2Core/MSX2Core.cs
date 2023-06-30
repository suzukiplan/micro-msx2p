using System;
using System.Runtime.InteropServices;

namespace MSX2 {
    public enum ColorMode : int
    {
        RGB555 = 0,
        RGB565 = 1,
    }

    public enum RomType : int
    {
        Normal = 0,
        Asc8 = 1,
        Asc8Sram2 = 2,
        Asc16 = 3,
        Asc16Sram2 = 4,
        KonamiScc = 5,
        Konami = 6,
    }

    public class Core
    {
        [DllImport("libMSX2.so", EntryPoint="msx2_createContext")]
        private static extern IntPtr ICreateContext(int colorMode);
    
        public static IntPtr CreateContext(ColorMode colorMode)
        {
            return ICreateContext((int)colorMode);
        }

        [DllImport("libMSX2.so", EntryPoint="msx2_releaseContext")]
        public static extern void ReleaseContext(IntPtr context);

        [DllImport("libMSX2.so", EntryPoint="msx2_setupSecondaryExist")]
        public static extern void SetupSecondaryExist(IntPtr context, bool page0, bool page1, bool page2, bool page3);

        [DllImport("libMSX2.so", EntryPoint="msx2_setupRAM")]
        public static extern void SetupRam(IntPtr context, int pri, int sec);

        [DllImport("libMSX2.so", EntryPoint="msx2_setup")]
        private static extern void ISetup(IntPtr context, int pri, int sec, int idx, byte[] data, int size, string label);

        public static void Setup(IntPtr context, int pri, int sec, int idx, byte[] data, string label)
        {
            ISetup(context, pri, sec, idx, data, data.Length, label);
        }

        [DllImport("libMSX2.so", EntryPoint="msx2_loadFont")]
        private static extern void ILoadFont(IntPtr context, byte[] font, int size);

        public static void LoadFont(IntPtr context, byte[] font)
        {
            ILoadFont(context, font, font.Length);
        }

        [DllImport("libMSX2.so", EntryPoint="msx2_setupSpecialKeyCode")]
        public static extern void SetupSpecialKeyCode(IntPtr context, int select, int start);

        [DllImport("libMSX2.so", EntryPoint="msx2_tick")]
        public static extern void Tick(IntPtr context, int pad1, int pad2, int key);

        [DllImport("libMSX2.so", EntryPoint="msx2_getDisplay")]
        private static extern void IGetDisplay(IntPtr context, byte[] display);

        public static byte[] GetDisplay(IntPtr context)
        {
            byte[] result = new byte[GetDisplayWidth(context) * GetDisplayHeight(context) * 2];
            IGetDisplay(context, result);
            return result;
        }

        [DllImport("libMSX2.so", EntryPoint="msx2_getDisplayWidth")]
        public static extern int GetDisplayWidth(IntPtr context);

        [DllImport("libMSX2.so", EntryPoint="msx2_getDisplayHeight")]
        public static extern int GetDisplayHeight(IntPtr context);

        [DllImport("libMSX2.so", EntryPoint="msx2_getMaxSoundSize")]
        private static extern int IGetMaxSoundSize(IntPtr context);

        [DllImport("libMSX2.so", EntryPoint="msx2_getMaxSoundSize")]
        private static extern void IGetSound(IntPtr context, byte[] sound, IntPtr size);

        public static byte[] GetSound(IntPtr context)
        {
            int maxSize = IGetMaxSoundSize(context);
            byte[] result = new byte[maxSize];
            IntPtr getSize = new IntPtr();
            IGetSound(context, result, getSize);
            Array.Resize(ref result, getSize.ToInt32());
            return result;
        }

        [DllImport("libMSX2.so", EntryPoint="msx2_loadRom")]
        private static extern void ILoadRom(IntPtr context, byte[] rom, int size, int romType);

        public static void LoadRom(IntPtr context, byte[] rom, RomType romType)
        {
            ILoadRom(context, rom, rom.Length, (int)romType);
        }

        [DllImport("libMSX2.so", EntryPoint="msx2_ejectRom")]
        public static extern void EjectRom(IntPtr context);

        [DllImport("libMSX2.so", EntryPoint="msx2_insertDisk")]
        private static extern void IInsertDisk(IntPtr context, int driveId, byte[] disk, int size, bool readOnly);

        public static void InsertDisk(IntPtr context, int driveId, byte[] disk, bool readOnly)
        {
            IInsertDisk(context, driveId, disk, disk.Length, readOnly);
        }

        [DllImport("libMSX2.so", EntryPoint="msx2_ejectDisk")]
        public static extern void EjectDisk(IntPtr context, int driveId);

        [DllImport("libMSX2.so", EntryPoint="msx2_getQuickSaveSize")]
        private static extern int IGetQuickSaveSize(IntPtr context);

        [DllImport("libMSX2.so", EntryPoint="msx2_quickSave")]
        private static extern IntPtr IQuickSave(IntPtr context);

        public static byte[] QuickSave(IntPtr context)
        {
            int size = IGetQuickSaveSize(context);
            IntPtr data = IQuickSave(context);
            byte[] bytes = new byte[size];
            Marshal.Copy(bytes, 0, data, bytes.Length);
            return bytes;
        }

        [DllImport("libMSX2.so", EntryPoint="msx2_quickLoad")]
        private static extern void IQuickLoad(IntPtr context, byte[] save, int size);

        public static void QuickLoad(IntPtr context, byte[] save) 
        {
            IQuickLoad(context, save, save.Length);
        }

        [DllImport("libMSX2.so", EntryPoint="msx2_reset")]
        public static extern void Reset(IntPtr context);
    }
}
