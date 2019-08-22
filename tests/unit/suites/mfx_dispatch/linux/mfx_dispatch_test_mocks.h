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

#ifndef MFX_DISPATCH_MOCKS_H
#define MFX_DISPATCH_MOCKS_H

#include <mfxvideo.h>
#include <mfxplugin.h>
#include <gmock/gmock.h>

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Matches;
using ::testing::StrEq;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::InSequence;

extern "C"
{
    mfxStatus GetPluginParamWrap(mfxHDL pthis, mfxPluginParam *par);
}

class CallObj
{
public:
    virtual ~CallObj() {};
    virtual void *dlopen(const char *filename, int flag) = 0;
    virtual int dlclose(void* handle) = 0;
    virtual void *dlsym(void *handle, const char *symbol) = 0;
    virtual FILE * fopen(const char *filename, const char *opentype) = 0;
    virtual char* fgets(char * str, int num, FILE * stream) = 0;
    virtual int fclose(FILE* stream) = 0;
    virtual mfxStatus MFXInitEx(mfxInitParam par, mfxSession *session) = 0;
    virtual mfxStatus MFXQueryVersion(mfxSession session, mfxVersion *version) = 0;
    virtual mfxStatus MFXQueryIMPL(mfxSession session, mfxIMPL *impl) = 0;
    virtual mfxStatus MFXClose(mfxSession session) = 0;
    virtual mfxStatus CreatePlugin(mfxPluginUID uid, mfxPlugin* pPlugin) = 0;
    virtual mfxStatus GetPluginParam(mfxHDL pthis, mfxPluginParam *par) = 0;
    virtual mfxStatus MFXVideoUSER_Register(mfxSession session, mfxU32 type, const mfxPlugin *par) = 0;
    virtual mfxStatus MFXVideoUSER_Unregister(mfxSession session, mfxU32 type) = 0;
};

class MockCallObj : public CallObj
{
public:
    MockCallObj()
    {
        ON_CALL(*this, MFXInitEx(_,_)).WillByDefault(Return(MFX_ERR_UNSUPPORTED));
        ON_CALL(*this, MFXQueryVersion(_,_)).WillByDefault(Return(MFX_ERR_UNSUPPORTED));
        ON_CALL(*this, MFXQueryIMPL(_,_)).WillByDefault(Return(MFX_ERR_UNSUPPORTED));
        ON_CALL(*this, CreatePlugin(_,_)).WillByDefault(Return(MFX_ERR_UNSUPPORTED));
        ON_CALL(*this, GetPluginParam(_,_)).WillByDefault(Return(MFX_ERR_UNSUPPORTED));
        ON_CALL(*this, MFXVideoUSER_Register(_,_,_)).WillByDefault(Return(MFX_ERR_UNSUPPORTED));
        ON_CALL(*this, MFXVideoUSER_Unregister(_,_)).WillByDefault(Return(MFX_ERR_UNSUPPORTED));

        m_mock_plugin.GetPluginParam = GetPluginParamWrap;
        m_mock_plugin.pthis = nullptr;
        m_mock_plugin.PluginInit = nullptr;
        m_mock_plugin.PluginClose = nullptr;
        m_mock_plugin.Submit = nullptr;
        m_mock_plugin.Execute = nullptr;
        m_mock_plugin.FreeResources = nullptr;
    }
    MOCK_METHOD2(dlopen, void* (const char *, int));
    MOCK_METHOD1(dlclose, int (void*));
    MOCK_METHOD2(dlsym, void* (void*, const char*));
    MOCK_METHOD2(fopen, FILE* (const char*, const char*));
    MOCK_METHOD3(fgets, char*(char*, int, FILE*));
    MOCK_METHOD1(fclose, int (FILE*));
    MOCK_METHOD2(MFXInitEx, mfxStatus (mfxInitParam, mfxSession *));
    MOCK_METHOD2(MFXQueryVersion, mfxStatus (mfxSession , mfxVersion *));
    MOCK_METHOD2(MFXQueryIMPL, mfxStatus (mfxSession session, mfxIMPL *impl));
    MOCK_METHOD1(MFXClose, mfxStatus(mfxSession));
    MOCK_METHOD2(CreatePlugin, mfxStatus(mfxPluginUID, mfxPlugin*));
    MOCK_METHOD2(GetPluginParam, mfxStatus (mfxHDL, mfxPluginParam*));
    MOCK_METHOD3(MFXVideoUSER_Register, mfxStatus (mfxSession, mfxU32, const mfxPlugin *));
    MOCK_METHOD2(MFXVideoUSER_Unregister, mfxStatus(mfxSession, mfxU32));

    mfxVersion emulated_api_version = {{0, 0}};

    void* EmulateAPI(void *handle, const char *symbol);
    mfxStatus ReturnEmulatedVersion(mfxSession session, mfxVersion *version)
    {
        *version = emulated_api_version;
        return MFX_ERR_NONE;
    }

    void SetEmulatedPluginsCfgContent(std::vector<std::string> content)
    {
        m_emulated_plugins_cfg = content;
        m_next_plugin_cfg_line_idx = 0;
        return;
    }
    char* FeedEmulatedPluginsCfgLines(char * str, int num, FILE * stream);
    mfxStatus ReturnMockPlugin(mfxPluginUID uid, mfxPlugin* pPlugin)
    {
        if (pPlugin == nullptr)
            return MFX_ERR_UNSUPPORTED;
        *pPlugin = m_mock_plugin;
        return MFX_ERR_NONE;
    }
    mfxPlugin m_mock_plugin;
private:
    std::vector<std::string> m_emulated_plugins_cfg;
    size_t m_next_plugin_cfg_line_idx = 0;
};

#endif /* MFX_DISPATCH_MOCKS_H */
