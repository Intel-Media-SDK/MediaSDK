// Copyright (c) 2020-2022 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace msdk_analyzer
{
    static class WinApi
    {
        [DllImport("user32.dll")]
        public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        public static extern IntPtr SendMessage(IntPtr hWnd, UInt32 Msg, IntPtr wParam, IntPtr lParam);        

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern uint RegisterWindowMessage(string lpString);

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern bool SetWindowText(IntPtr hwnd, String lpString);
    }

    static class Kernel32
    {
        [DllImport("kernel32.dll")]
        public static extern IntPtr LoadLibrary(string dllToLoad);

        [DllImport("kernel32.dll")]
        public static extern IntPtr GetProcAddress(IntPtr hModule, string procedureName);

        [DllImport("kernel32.dll")]
        public static extern bool FreeLibrary(IntPtr hModule);
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct WNODE_HEADER
    {
        public UInt32 BufferSize;        // Size of entire buffer inclusive of this ULONG
        public UInt32 ProviderId;    // Provider Id of driver returning this buffer
        //union
        //{
        public UInt64 HistoricalContext;  // Logger use
        //    struct
        //        {
        //        ULONG Version;           // Reserved
        //        ULONG Linkage;           // Linkage field reserved for WMI
        //    } DUMMYSTRUCTNAME;
        //} DUMMYUNIONNAME;

        //public union DUMMYUNIONNAME2
        //{
        //    //UInt32 CountLost;         // Reserved
        //UInt32 KernelHandle;     // Kernel handle for data block
        //[Marshal (UnmanagedType.LPArray)]
        //public UInt32 KernelHandle;               

        public UInt64 TimeStamp;
        //LARGE_INTEGER TimeStamp; // Timestamp as returned in units of 100ns
        // since 1/1/1601
        //} 
        //[MarshalAs(UnmanagedType.LPStruct.LPStruct)]
        //public Guid rGuid;                  // Guid for data block returned with results
        public UInt64 rGuidLow;
        public UInt64 rGuidHi;
        public UInt32 ClientContext;
        public UInt32 Flags;             // Flags, see below
    }


    [StructLayout(LayoutKind.Sequential)]
    public struct S128
    {
        public UInt64 q, w, e, r, t, y, u, i, o, p, a, s, d, f, g, h;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct S2048
    {
        public S128 q, w, e, r, t, y, u, i, o, p, a, s, d, f, g, h;
    }


    [StructLayout(LayoutKind.Sequential)]
    public class EVENT_TRACE_PROPERTIES
    {
        public WNODE_HEADER Wnode;
        //
        // data provided by caller
        public UInt32 BufferSize;                   // buffer size for logging (kbytes)
        public UInt32 MinimumBuffers;               // minimum to preallocate
        public UInt32 MaximumBuffers;               // maximum buffers allowed
        public UInt32 MaximumFileSize;              // maximum logfile size (in MBytes)
        public UInt32 LogFileMode;                  // sequential, circular
        public UInt32 FlushTimer;                   // buffer flush timer, in seconds
        public UInt32 EnableFlags;                  // trace enable flags
        public UInt32 AgeLimit;                     // unused

        // data returned to caller
        public UInt32 NumberOfBuffers;              // no of buffers in use
        public UInt32 FreeBuffers;                  // no of buffers free
        public UInt32 EventsLost;                   // event records lost
        public UInt32 BuffersWritten;               // no of buffers written to file
        public UInt32 LogBuffersLost;               // no of logfile write failures
        public UInt32 RealTimeBuffersLost;          // no of rt delivery failures
        //[MarshalAs (UnmanagedType.LPArray)]
        public UInt32 LoggerThreadId;               // thread id of Logger
        public UInt32 LogFileNameOffset;            // Offset to LogFileName
        public UInt32 LoggerNameOffset;             // Offset to LoggerName

        //memory reserving for buffer
        S2048 need;
    }

    class AdvApi
    {
        const string dll_path = "Advapi32.dll";

        [DllImport(dll_path, CharSet = CharSet.Auto, CallingConvention = CallingConvention.StdCall)]
        public static extern UInt64 QueryTrace(UInt64 TraceHandle,
                [MarshalAs(UnmanagedType.LPTStr)] string InstanceName,
                [Out, MarshalAs(UnmanagedType.LPStruct)] EVENT_TRACE_PROPERTIES Properties);
    }
}
