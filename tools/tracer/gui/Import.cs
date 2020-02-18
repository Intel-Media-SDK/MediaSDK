/* ****************************************************************************** *\

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2020 Intel Corporation. All Rights Reserved.

File Name: Import.cs

\* ****************************************************************************** */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace TAL
{
    class TalImport32
    {
        const string mfx_tal_dll = "mpa_rt32.dll";

        [DllImport(mfx_tal_dll, CallingConvention = CallingConvention.Cdecl)]
        public static extern int MPA_SetTraceFile(char[] path);

        [DllImport(mfx_tal_dll)]
        public static extern int MPA_CreateConsumer(char[] name_guid);

        [DllImport(mfx_tal_dll)]
        public static extern int MPA_FlushLoggers();

        [DllImport(mfx_tal_dll)]
        public static extern int MPA_DestroyAllConsumers();

        [DllImport(mfx_tal_dll)]
        public static extern int MPA_GetProviderEvents(char[] stingedGuid);

        [DllImport(mfx_tal_dll)]
        public static extern long MPA_GetProviderTimestamp(char[] stingedGuid);

        [DllImport(mfx_tal_dll)]
        public static extern double MPA_GetGPUUsage(int engine, int device);

        [DllImport(mfx_tal_dll)]
        public static extern int MPA_GetGPUFrequency(int i);

        [DllImport(mfx_tal_dll)]
        public static extern int MPA_ConvertETWFile(char[] name);

        [DllImport(mfx_tal_dll)]
        public unsafe static extern Int64 GetTableData(char* data, int max_data_len);
    }

    //todo this shouldbe equal to talimport32 except of library name
    class TalImport64
    {
        const string mfx_tal_dll = "mpa_rt64.dll";

        [DllImport(mfx_tal_dll, CallingConvention = CallingConvention.Cdecl)]
        public static extern int MPA_SetTraceFile(char[] path);

        [DllImport(mfx_tal_dll)]
        public static extern int MPA_CreateConsumer(char[] name_guid);

        [DllImport(mfx_tal_dll)]
        public static extern int MPA_FlushLoggers();

        [DllImport(mfx_tal_dll)]
        public static extern int MPA_DestroyAllConsumers();

        [DllImport(mfx_tal_dll)]
        public static extern int MPA_GetProviderEvents(char[] stingedGuid);

        [DllImport(mfx_tal_dll)]
        public static extern long MPA_GetProviderTimestamp(char[] stingedGuid);

        [DllImport(mfx_tal_dll)]
        public static extern double MPA_GetGPUUsage(int engine, int device);

        [DllImport(mfx_tal_dll)]
        public static extern int MPA_GetGPUFrequency(int i);

        [DllImport(mfx_tal_dll)]
        public static extern int MPA_ConvertETWFile(char[] name);

        [DllImport(mfx_tal_dll)]
        public unsafe static extern Int64 GetTableData(char* data, int max_data_len);
    }
}

namespace msdk_analyzer
{

    class MPALib
    {
        int m_nVer;
                
        public MPALib (int nBitsImplementation)
        {
            m_nVer = nBitsImplementation;
            if (m_nVer == 32)
            {
                
            }else if (m_nVer == 64)
            {
                
            }
            else throw new Exception("Unsupported Dll mfx_tal"+nBitsImplementation+".dll");
        }

        public void SetTraceFile(string file_name)
        {
            char[] s = file_name.ToCharArray();
            if (m_nVer == 32)
                TAL.TalImport32.MPA_SetTraceFile(s);
            else
                TAL.TalImport64.MPA_SetTraceFile(s);
        }
        public void CreateConsumer(string guid)
        {
            char[] s = guid.ToCharArray();
            if (m_nVer == 32)
                TAL.TalImport32.MPA_CreateConsumer(s);
            else
                TAL.TalImport64.MPA_CreateConsumer(s);
        }
        public void DestroyAllConsumers()
        {
            if (m_nVer == 32)
                TAL.TalImport32.MPA_DestroyAllConsumers();
            else
                TAL.TalImport64.MPA_DestroyAllConsumers();
        }
        public void FlushLoggers()
        {
            if (m_nVer == 32)
                TAL.TalImport32.MPA_FlushLoggers();
            else
                TAL.TalImport64.MPA_FlushLoggers();
        }
        public int GetProviderEvents(string provider_guid)
        {
            char[] s = provider_guid.ToCharArray();
            if (m_nVer == 32)
                return TAL.TalImport32.MPA_GetProviderEvents(s);
            else
                return TAL.TalImport64.MPA_GetProviderEvents(s);
        }
        public long GetProviderTimestamp(string provider_guid)
        {
            char[] s = provider_guid.ToCharArray();
            if (m_nVer == 32)
                return TAL.TalImport32.MPA_GetProviderTimestamp(s);
            else
                return TAL.TalImport64.MPA_GetProviderTimestamp(s);
        }
        public double GetGPUUsage(int engine, int device)
        {
            if (m_nVer == 32)
                return TAL.TalImport32.MPA_GetGPUUsage(engine, device);
            else
                return TAL.TalImport64.MPA_GetGPUUsage(engine, device);
        }
        public int GetGPUFrequency(int i)
        {
            if (m_nVer == 32)
                return TAL.TalImport32.MPA_GetGPUFrequency(i);
            else
                return TAL.TalImport64.MPA_GetGPUFrequency(i);
        }
        public int ConvertETWFile(string ETWFile)
        {
            char[] s = ETWFile.ToCharArray();
            //List<string> names = new List<string>();
            //names.Add("path");
            //names.Add(ETWFile);
            //names.Count, names.ToArray());
            if (m_nVer == 32)
                return TAL.TalImport32.MPA_ConvertETWFile(s);
            else
                return TAL.TalImport64.MPA_ConvertETWFile(s);
        }
    }
}
