// Copyright (c) 2018-2020 Intel Corporation
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

#include "mfx_dispatch_test_main.h"
#include "mfx_dispatch_test_fixtures.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
#include <sstream>
#include "mfx_dispatch_test_mocks.h"

constexpr size_t TEST_PLUGIN_COUNT = 7;

TEST_F(DispatcherPluginsTest, ShouldSearchForPluginsCfgByCorrectPath)
{
    MockCallObj& mock = *g_call_obj_ptr;

    EXPECT_CALL(mock, fopen(StrEq(MFX_PLUGINS_CONF_DIR "/plugins.cfg"),_)).Times(1);

    mfxPluginUID uid{};
    std::fill(uid.Data, uid.Data + sizeof(mfxPluginUID), MOCK_PLUGIN_UID_BYTE_BASE);
    MFXVideoUSER_Load(session, &uid, 0);
}

TEST_F(DispatcherPluginsTest, ShouldFailIfPluginIsNotFound)
{
    MockCallObj& mock = *g_call_obj_ptr;

    EXPECT_CALL(mock, fopen(StrEq(MFX_PLUGINS_CONF_DIR "/plugins.cfg"), _)).Times(1).WillRepeatedly(Return(MOCK_FILE_DESCRIPTOR));
    EXPECT_CALL(mock, fgets(_, _, MOCK_FILE_DESCRIPTOR)).Times(1).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(mock, fclose).Times(1);

    mfxPluginUID uid{};
    std::fill(uid.Data, uid.Data + sizeof(mfxPluginUID), MOCK_PLUGIN_UID_BYTE_BASE);
    mfxStatus sts = MFXVideoUSER_Load(session, &uid, 0);
    ASSERT_EQ(sts, MFX_ERR_NOT_FOUND);
}

static bool successfully_parsed_at_least_once = false;

// The parameter will determine the order of the plugin to be loaded.
TEST_P(DispatcherPluginsTestParametrized, ShouldLoadFromGoodPluginsCfgCorrectly)
{
    MockCallObj& mock = *g_call_obj_ptr;
    SetEmulatedPluginsCfgContent(mock, TEST_PLUGIN_COUNT);

    size_t plugin_idx = GetParam();
    ASSERT_TRUE(plugin_idx < m_plugin_vec.size());

    const PluginParamExtended& plugin = m_plugin_vec[plugin_idx];

    if (!successfully_parsed_at_least_once)
    {   // The plugin file is only successfully parsed once by the dispatcher,
        // and the parsed plugin list is stored as a global variable in the
        // dispatcher lib. Therefore, the calls below will only be executed once.
        EXPECT_CALL(mock, fopen(StrEq(MFX_PLUGINS_CONF_DIR "/plugins.cfg"), _)).Times(1).WillRepeatedly(Return(MOCK_FILE_DESCRIPTOR));
        EXPECT_CALL(mock, fgets(_, _, MOCK_FILE_DESCRIPTOR)).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::FeedEmulatedPluginsCfgLines));
        EXPECT_CALL(mock, fclose).Times(1).WillRepeatedly(Return(MFX_ERR_NONE));
        successfully_parsed_at_least_once = true;
    }

    EXPECT_CALL(mock, dlopen(StrEq(plugin.Path.c_str()), _)).Times(1).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym(_, StrEq("CreatePlugin"))).Times(1).WillRepeatedly(Return(reinterpret_cast<void*>(CreatePluginWrap)));
    EXPECT_CALL(mock, CreatePlugin(plugin.PluginUID, _)).Times(1).WillRepeatedly(Invoke(&mock, &MockCallObj::ReturnMockPlugin));
    EXPECT_CALL(mock, GetPluginParam(mock.m_mock_plugin.pthis, _)).Times(1).WillRepeatedly(DoAll(SetArgPointee<1>(plugin), Return(MFX_ERR_NONE)));
    EXPECT_CALL(mock, MFXVideoUSER_Register(MOCK_SESSION_HANDLE, plugin.Type, _)).Times(1).WillRepeatedly(Return(MFX_ERR_NONE));

    mfxStatus sts = MFXVideoUSER_Load(session, &plugin.PluginUID, 0);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    EXPECT_CALL(mock, dlclose).Times(1);
    EXPECT_CALL(mock, MFXVideoUSER_Unregister(MOCK_SESSION_HANDLE, plugin.Type)).Times(1).WillRepeatedly(Return(MFX_ERR_NONE));
    sts = MFXVideoUSER_UnLoad(session, &plugin.PluginUID);
    ASSERT_EQ(sts, MFX_ERR_NONE);
}

INSTANTIATE_TEST_CASE_P(EnumeratingPlugins, DispatcherPluginsTestParametrized, ::testing::Range((size_t)0, TEST_PLUGIN_COUNT));

TEST_F(DispatcherPluginsTest, ShouldFailIfPluginHasNoSymbols)
{
    MockCallObj& mock = *g_call_obj_ptr;
    SetEmulatedPluginsCfgContent(mock, TEST_PLUGIN_COUNT);

    const PluginParamExtended& plugin = m_plugin_vec[0];

    if (!successfully_parsed_at_least_once)
    {
        EXPECT_CALL(mock, fopen(StrEq(MFX_PLUGINS_CONF_DIR "/plugins.cfg"), _)).Times(1).WillRepeatedly(Return(MOCK_FILE_DESCRIPTOR));
        EXPECT_CALL(mock, fgets(_, _, MOCK_FILE_DESCRIPTOR)).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::FeedEmulatedPluginsCfgLines));
        EXPECT_CALL(mock, fclose).Times(1).WillRepeatedly(Return(MFX_ERR_NONE));
        successfully_parsed_at_least_once = true;
    }

    EXPECT_CALL(mock, dlopen(StrEq(plugin.Path.c_str()), _)).Times(1).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym(_, StrEq("CreatePlugin"))).Times(1);
    EXPECT_CALL(mock, CreatePlugin(plugin.PluginUID, _)).Times(0);
    EXPECT_CALL(mock, GetPluginParam(mock.m_mock_plugin.pthis, _)).Times(0);
    EXPECT_CALL(mock, MFXVideoUSER_Register(MOCK_SESSION_HANDLE, plugin.Type, _)).Times(0);
    EXPECT_CALL(mock, dlclose).Times(1);

    mfxStatus sts = MFXVideoUSER_Load(session, &plugin.PluginUID, 0);
    ASSERT_EQ(sts, MFX_ERR_NOT_FOUND);
}

TEST_F(DispatcherPluginsTest, ShouldFailIfPluginIsNotCreated)
{
    MockCallObj& mock = *g_call_obj_ptr;
    SetEmulatedPluginsCfgContent(mock, TEST_PLUGIN_COUNT);

    const PluginParamExtended& plugin = m_plugin_vec[0];

    if (!successfully_parsed_at_least_once)
    {
        EXPECT_CALL(mock, fopen(StrEq(MFX_PLUGINS_CONF_DIR "/plugins.cfg"), _)).Times(1).WillRepeatedly(Return(MOCK_FILE_DESCRIPTOR));
        EXPECT_CALL(mock, fgets(_, _, MOCK_FILE_DESCRIPTOR)).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::FeedEmulatedPluginsCfgLines));
        EXPECT_CALL(mock, fclose).Times(1).WillRepeatedly(Return(MFX_ERR_NONE));
        successfully_parsed_at_least_once = true;
    }

    EXPECT_CALL(mock, dlopen(StrEq(plugin.Path.c_str()), _)).Times(1).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym(_, StrEq("CreatePlugin"))).Times(1).WillRepeatedly(Return(reinterpret_cast<void*>(CreatePluginWrap)));
    EXPECT_CALL(mock, CreatePlugin(plugin.PluginUID, _)).Times(1);
    EXPECT_CALL(mock, GetPluginParam(mock.m_mock_plugin.pthis, _)).Times(0);
    EXPECT_CALL(mock, MFXVideoUSER_Register(MOCK_SESSION_HANDLE, plugin.Type, _)).Times(0);
    EXPECT_CALL(mock, dlclose).Times(1);

    mfxStatus sts = MFXVideoUSER_Load(session, &plugin.PluginUID, 0);
    ASSERT_EQ(sts, MFX_ERR_UNSUPPORTED);
}

TEST_F(DispatcherPluginsTest, ShouldFailIfUnknownPluginParams)
{
    MockCallObj& mock = *g_call_obj_ptr;
    SetEmulatedPluginsCfgContent(mock, TEST_PLUGIN_COUNT);

    const PluginParamExtended& plugin = m_plugin_vec[0];

    if (!successfully_parsed_at_least_once)
    {
        EXPECT_CALL(mock, fopen(StrEq(MFX_PLUGINS_CONF_DIR "/plugins.cfg"), _)).Times(1).WillRepeatedly(Return(MOCK_FILE_DESCRIPTOR));
        EXPECT_CALL(mock, fgets(_, _, MOCK_FILE_DESCRIPTOR)).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::FeedEmulatedPluginsCfgLines));
        EXPECT_CALL(mock, fclose).Times(1).WillRepeatedly(Return(MFX_ERR_NONE));
        successfully_parsed_at_least_once = true;
    }

    EXPECT_CALL(mock, dlopen(StrEq(plugin.Path.c_str()), _)).Times(1).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym(_, StrEq("CreatePlugin"))).Times(1).WillRepeatedly(Return(reinterpret_cast<void*>(CreatePluginWrap)));
    EXPECT_CALL(mock, CreatePlugin(plugin.PluginUID, _)).Times(1).WillRepeatedly(Invoke(&mock, &MockCallObj::ReturnMockPlugin));
    EXPECT_CALL(mock, GetPluginParam(mock.m_mock_plugin.pthis, _)).Times(1);
    EXPECT_CALL(mock, MFXVideoUSER_Register(MOCK_SESSION_HANDLE, plugin.Type, _)).Times(0);
    EXPECT_CALL(mock, dlclose).Times(1);

    mfxStatus sts = MFXVideoUSER_Load(session, &plugin.PluginUID, 0);
    ASSERT_EQ(sts, MFX_ERR_UNSUPPORTED);
}

TEST_F(DispatcherPluginsTest, ShouldFailIfRegisterFailed)
{
    MockCallObj& mock = *g_call_obj_ptr;
    SetEmulatedPluginsCfgContent(mock, TEST_PLUGIN_COUNT);

    const PluginParamExtended& plugin = m_plugin_vec[0];

    if (!successfully_parsed_at_least_once)
    {
        EXPECT_CALL(mock, fopen(StrEq(MFX_PLUGINS_CONF_DIR "/plugins.cfg"), _)).Times(1).WillRepeatedly(Return(MOCK_FILE_DESCRIPTOR));
        EXPECT_CALL(mock, fgets(_, _, MOCK_FILE_DESCRIPTOR)).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::FeedEmulatedPluginsCfgLines));
        EXPECT_CALL(mock, fclose).Times(1).WillRepeatedly(Return(MFX_ERR_NONE));
        successfully_parsed_at_least_once = true;
    }

    EXPECT_CALL(mock, dlopen(StrEq(plugin.Path.c_str()), _)).Times(1).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym(_, StrEq("CreatePlugin"))).Times(1).WillRepeatedly(Return(reinterpret_cast<void*>(CreatePluginWrap)));
    EXPECT_CALL(mock, CreatePlugin(plugin.PluginUID, _)).Times(1).WillRepeatedly(Invoke(&mock, &MockCallObj::ReturnMockPlugin));
    EXPECT_CALL(mock, GetPluginParam(mock.m_mock_plugin.pthis, _)).Times(1).WillRepeatedly(DoAll(SetArgPointee<1>(plugin), Return(MFX_ERR_NONE)));
    EXPECT_CALL(mock, MFXVideoUSER_Register(MOCK_SESSION_HANDLE, plugin.Type, _)).Times(1);
    EXPECT_CALL(mock, dlclose).Times(1);

    mfxStatus sts = MFXVideoUSER_Load(session, &plugin.PluginUID, 0);
    ASSERT_EQ(sts, MFX_ERR_UNSUPPORTED);
}

TEST_F(DispatcherPluginsTest, ShouldFailIfUnregisterFailed)
{
    MockCallObj& mock = *g_call_obj_ptr;
    SetEmulatedPluginsCfgContent(mock, TEST_PLUGIN_COUNT);

    const PluginParamExtended& plugin = m_plugin_vec[0];

    if (!successfully_parsed_at_least_once)
    {
        EXPECT_CALL(mock, fopen(StrEq(MFX_PLUGINS_CONF_DIR "/plugins.cfg"), _)).Times(1).WillRepeatedly(Return(MOCK_FILE_DESCRIPTOR));
        EXPECT_CALL(mock, fgets(_, _, MOCK_FILE_DESCRIPTOR)).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::FeedEmulatedPluginsCfgLines));
        EXPECT_CALL(mock, fclose).Times(1).WillRepeatedly(Return(MFX_ERR_NONE));
        successfully_parsed_at_least_once = true;
    }

    EXPECT_CALL(mock, dlopen(StrEq(plugin.Path.c_str()), _)).Times(1).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym(_, StrEq("CreatePlugin"))).Times(1).WillRepeatedly(Return(reinterpret_cast<void*>(CreatePluginWrap)));
    EXPECT_CALL(mock, CreatePlugin(plugin.PluginUID, _)).Times(1).WillRepeatedly(Invoke(&mock, &MockCallObj::ReturnMockPlugin));
    EXPECT_CALL(mock, GetPluginParam(mock.m_mock_plugin.pthis, _)).Times(1).WillRepeatedly(DoAll(SetArgPointee<1>(plugin), Return(MFX_ERR_NONE)));
    EXPECT_CALL(mock, MFXVideoUSER_Register(MOCK_SESSION_HANDLE, plugin.Type, _)).Times(1).WillRepeatedly(Return(MFX_ERR_NONE));

    mfxStatus sts = MFXVideoUSER_Load(session, &plugin.PluginUID, 0);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    EXPECT_CALL(mock, dlclose).Times(0);
    EXPECT_CALL(mock, MFXVideoUSER_Unregister(MOCK_SESSION_HANDLE, plugin.Type)).Times(1);
    sts = MFXVideoUSER_UnLoad(session, &plugin.PluginUID);
    ASSERT_EQ(sts, MFX_ERR_UNSUPPORTED);
}

