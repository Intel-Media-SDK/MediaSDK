/* ****************************************************************************** *\

Copyright (C) 2020 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: dump_mfxplugin.cpp

\* ****************************************************************************** */

#include "dump.h"

std::string DumpContext::dump(const std::string structName, const mfxPlugin &plugin)
{
    std::string str;
    str += structName + ".pthis=" + ToString(plugin.pthis) + "\n";
    str += structName + ".PluginInit=" + ToString(plugin.PluginInit) + "\n";
    str += structName + ".PluginClose=" + ToString(plugin.PluginClose) + "\n";
    str += structName + ".GetPluginParam=" + ToString(plugin.GetPluginParam) + "\n";
    str += structName + ".Submit=" + ToString(plugin.Submit) + "\n";
    str += structName + ".Execute=" + ToString(plugin.Execute) + "\n";
    str += structName + ".FreeResources=" + ToString(plugin.FreeResources) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(plugin.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxCoreParam &CoreParam)
{
    std::string str;
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(CoreParam.reserved) + "\n";
    str += structName + ".Impl=" + GetmfxIMPL(CoreParam.Impl) + "\n";
    str += dump(structName + ".Version=", CoreParam.Version) + "\n";
    str += structName + ".NumWorkingThread=" + ToString(CoreParam.NumWorkingThread) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxPluginParam &PluginParam)
{
    std::string str;
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(PluginParam.reserved) + "\n";
    str += structName + ".reserved1=" + ToString(PluginParam.reserved1) + "\n";
    str += structName + ".PluginVersion=" + ToString(PluginParam.PluginVersion) + "\n";
    str += dump(structName + ".APIVersion=", PluginParam.APIVersion) + "\n";
    str += structName + ".PluginUID=" + dump_hex_array(PluginParam.PluginUID.Data) + "\n";
    str += structName + ".Type=" + ToString(PluginParam.Type) + "\n";
    str += structName + ".CodecId=" + GetCodecIdString(PluginParam.CodecId) + "\n";
    str += structName + ".ThreadPolicy=" + ToString(PluginParam.ThreadPolicy) + "\n";
    str += structName + ".MaxThreadNum=" + ToString(PluginParam.MaxThreadNum) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxCoreInterface &_struct)
{
    std::string str;

    DUMP_FIELD(pthis);
    DUMP_FIELD_RESERVED(reserved1);
    str += dump(structName + ".FrameAllocator", _struct.FrameAllocator) + "\n";
    DUMP_FIELD(GetCoreParam);
    DUMP_FIELD(GetHandle);
    DUMP_FIELD(IncreaseReference);
    DUMP_FIELD(DecreaseReference);
    DUMP_FIELD(CopyFrame);
    DUMP_FIELD(CopyBuffer);
    DUMP_FIELD(MapOpaqueSurface);
    DUMP_FIELD(UnmapOpaqueSurface);
    DUMP_FIELD(GetRealSurface);
    DUMP_FIELD(GetOpaqueSurface);
    DUMP_FIELD(CreateAccelerationDevice);
    DUMP_FIELD(GetFrameHandle);
    DUMP_FIELD(GetFrameHandle);
    DUMP_FIELD(QueryPlatform);
    DUMP_FIELD_RESERVED(reserved4);

    return str;
}