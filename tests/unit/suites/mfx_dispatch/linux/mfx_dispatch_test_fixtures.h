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

#ifndef MFX_DISPATCH_TEST_FIXTURES_H
#define MFX_DISPATCH_TEST_FIXTURES_H

#include "mfx_dispatch_test_main.h"
#include <gtest/gtest.h>
#include <map>
#include <list>
#include <mfxvideo.h>


class DispatcherLibsTest : public ::testing::Test
{
public:
    DispatcherLibsTest()
    {
        ResetMockCallObj();
    }
    virtual ~DispatcherLibsTest()
    {
        g_call_obj_ptr.reset(nullptr);
    }
protected:
    mfxIMPL impl = 0;
    mfxVersion ver{};
    mfxSession session = NULL;
};

class DispatcherLibsTestParametrized : public DispatcherLibsTest, public ::testing::WithParamInterface<mfxIMPL>
{
};

struct PluginParamExtended : mfxPluginParam
{
    std::string Path;
    std::string CodecIDString;
    mfxU32 Default = 0;
};

class DispatcherPluginsTest : public DispatcherLibsTest
{
public:
    DispatcherPluginsTest()
    {
        // Need MFXInit to return a session handle since
        // it is actually a ptr to LoaderCtx, which must be
        // valid for MFXVideoUSER_Load to work

        MockCallObj& mock = *g_call_obj_ptr;

        ver = {{MFX_VERSION_MINOR, MFX_VERSION_MAJOR}};
        mock.emulated_api_version = ver;

        SetupForGoodLibInit(mock);
        mfxStatus sts = MFXInit(impl, &ver, &session);
        EXPECT_EQ(sts, MFX_ERR_NONE);

        ResetMockCallObj(); // Reset the mock object so that the expectations
                            // made in the SetupForGoodLibInit do not propagate
                            // into plugin tests
    }

protected:

    void SetEmulatedPluginsCfgContent(MockCallObj& mock, size_t plugin_num)
    {
        m_plugin_vec = GenerateTestPluginParamVec(plugin_num);
        mock.SetEmulatedPluginsCfgContent(GeneratePluginsCfgContent(m_plugin_vec));
    }
    void SetupForGoodLibInit(MockCallObj& mock)
    {
        EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
        EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
        EXPECT_CALL(mock, MFXInitEx).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<1>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
        EXPECT_CALL(mock, MFXQueryIMPL).Times(AtLeast(1)).WillRepeatedly(Return(MFX_ERR_NONE));
        EXPECT_CALL(mock, MFXQueryVersion).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::ReturnEmulatedVersion));
        return;
    }

    std::vector<std::string> GeneratePluginsCfgContent(std::vector<PluginParamExtended> plugin_vec);
    std::vector<PluginParamExtended> GenerateTestPluginParamVec(size_t plugin_num);

    std::vector<mfxPluginType> m_plugin_types {
            MFX_PLUGINTYPE_VIDEO_GENERAL,
            MFX_PLUGINTYPE_VIDEO_DECODE,
            MFX_PLUGINTYPE_VIDEO_ENCODE,
            MFX_PLUGINTYPE_VIDEO_VPP,
            MFX_PLUGINTYPE_VIDEO_ENC,
            MFX_PLUGINTYPE_AUDIO_DECODE,
            MFX_PLUGINTYPE_AUDIO_ENCODE
        };

    std::vector<std::string> m_codec_ids {
        "AVC ",
        "HEVC",
        "MPG2",
        "VC1 ",
        "CAPT",
        "VP9 ",
        "AV1 "
    };
    std::vector<PluginParamExtended> m_plugin_vec;
};

class DispatcherPluginsTestParametrized : public DispatcherPluginsTest, public ::testing::WithParamInterface<size_t>
{

};

#endif /* MFX_DISPATCH_TEST_FIXTURES_H */
