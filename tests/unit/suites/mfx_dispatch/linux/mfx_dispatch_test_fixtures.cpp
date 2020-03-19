// Copyright (c) 2017-2019 Intel Corporation
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

#include "mfx_dispatch_test_fixtures.h"

std::vector<std::string> DispatcherPluginsTest::GeneratePluginsCfgContent(std::vector<PluginParamExtended> plugin_vec)
{
    std::vector<std::string> retval;
    std::stringstream ss;
    mfxU32 counter = 1;
    for (const PluginParamExtended& plugin : plugin_vec)
    {
        ss << "[Test Plugin " << counter << "] " << std::endl;
        ss << "GUID = " << plugin.PluginUID << std::endl;
        ss << "PluginVersion = " << plugin.PluginVersion << std::endl;
        ss << "APIVersion = " << plugin.APIVersion.Version << std::endl;
        ss << "Path = " << plugin.Path << std::endl;
        ss << "Type = " << plugin.Type << std::endl;
        ss << "CodecID = " << plugin.CodecIDString << std::endl;
        ss << "Default = " << plugin.Default << std:: endl;
        ++counter;
    }

    std::string str;
    while(std::getline(ss, str))
    {
        retval.emplace_back(str);
    }
    return retval;
}

std::vector<PluginParamExtended> DispatcherPluginsTest::GenerateTestPluginParamVec(size_t plugin_num)
{
    std::vector<PluginParamExtended> retval;

    for (size_t i = 0; i < plugin_num; ++i)
    {
        PluginParamExtended plugin;
        std::fill(plugin.PluginUID.Data, plugin.PluginUID.Data + sizeof(mfxPluginUID), 0xB0 + (mfxU8) i);
        plugin.PluginVersion = i + 1;
        plugin.APIVersion.Version = i + 1;
        plugin.Path = std::string("/test/path/to/") + "plugin_" + std::to_string(i + 1) + ".so";
        plugin.Type = m_plugin_types[i % m_plugin_types.size()];
        plugin.CodecIDString = m_codec_ids[i % m_plugin_types.size()];
        plugin.Default = 0;
        retval.emplace_back(plugin);
    }
    return retval;
}
