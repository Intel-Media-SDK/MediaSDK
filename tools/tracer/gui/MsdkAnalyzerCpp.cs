// Copyright (c) 2012-2022 Intel Corporation
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
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics.Eventing;
using Microsoft.Win32;

namespace msdk_analyzer
{
    class MsdkAnalyzerCpp
    {
#if DEBUG
#if WIN64
        const string msdk_analyzer_path = "mfx-tracer_64_d.dll";
#else
        const string msdk_analyzer_path = "mfx-tracer_32_d.dll";
#endif
#else
#if WIN64
        const string msdk_analyzer_path = "mfx-tracer_64.dll";
#else
        const string msdk_analyzer_path = "mfx-tracer_32.dll";
#endif
#endif


        [DllImport(msdk_analyzer_path, CharSet = CharSet.Auto, CallingConvention = CallingConvention.StdCall)]
        public static extern void convert_etl_to_text
            ([MarshalAs(UnmanagedType.U4)] int hwnd
           , [MarshalAs(UnmanagedType.U4)] int hinst
           , [MarshalAs(UnmanagedType.LPStr)]string lpszCmdLine
           , int nCmdShow);

        [DllImport(msdk_analyzer_path, CharSet = CharSet.Auto, CallingConvention = CallingConvention.StdCall)]
        public static extern UInt32 install([MarshalAs(UnmanagedType.LPStr)] string folder_path,
                                            [MarshalAs(UnmanagedType.LPStr)] string app_data,
                                            [MarshalAs(UnmanagedType.LPStr)] string conf_path);//default place for log file

        [DllImport(msdk_analyzer_path, CharSet = CharSet.Auto, CallingConvention = CallingConvention.StdCall)]
        public static extern UInt32 uninstall();

        [DllImport(msdk_analyzer_path, CharSet = CharSet.Auto, CallingConvention = CallingConvention.StdCall)]
        public static extern void start();

        [DllImport(msdk_analyzer_path, CharSet = CharSet.Auto, CallingConvention = CallingConvention.StdCall)]
        public static extern void stop();


    }


}
