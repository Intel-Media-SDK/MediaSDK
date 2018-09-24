// Copyright (c) 2018 Intel Corporation
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