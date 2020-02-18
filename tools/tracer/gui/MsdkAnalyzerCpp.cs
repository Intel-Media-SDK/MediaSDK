/* ****************************************************************************** *\

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012-2020 Intel Corporation. All Rights Reserved.

File Name: MsdkAnalyzerCpp.cs

\* ****************************************************************************** */

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
