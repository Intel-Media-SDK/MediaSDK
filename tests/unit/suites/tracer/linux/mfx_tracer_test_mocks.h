// Copyright (c) 2017-2020 Intel Corporation
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

#ifndef mfx_tracer_MOCKS_H
#define mfx_tracer_MOCKS_H

#include <mfxvideo.h>
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

class CallObj
{
public:
    virtual ~CallObj() {};
    virtual void *dlopen(const char *filename, int flag) = 0;
    virtual int dlclose(void* handle) = 0;
    virtual void *dlsym(void *handle, const char *symbol) = 0;

    virtual mfxStatus MFXInit(mfxIMPL implParam, mfxVersion *ver, mfxSession *session) = 0;
    virtual mfxStatus MFXInitEx(mfxInitParam par, mfxSession *session) = 0;
    virtual mfxStatus MFXClose(mfxSession session) = 0;
};

class MockCallObj : public CallObj
{
public:
    MockCallObj()
    {
        ON_CALL(*this, MFXInit(_,_,_)).WillByDefault(Return(MFX_ERR_UNSUPPORTED));
        ON_CALL(*this, MFXInitEx(_,_)).WillByDefault(Return(MFX_ERR_UNSUPPORTED));
    }
    MOCK_METHOD2(dlopen, void* (const char *, int));
    MOCK_METHOD1(dlclose, int (void*));
    MOCK_METHOD2(dlsym, void* (void*, const char*));

    MOCK_METHOD3(MFXInit, mfxStatus (mfxIMPL, mfxVersion *, mfxSession *));
    MOCK_METHOD2(MFXInitEx, mfxStatus (mfxInitParam, mfxSession *));
    MOCK_METHOD1(MFXClose, mfxStatus(mfxSession));

    mfxVersion emulated_api_version = {{0, 0}};

    void* EmulateAPI(void *handle, const char *symbol);
    mfxStatus ReturnEmulatedVersion(mfxSession session, mfxVersion *version)
    {
        *version = emulated_api_version;
        return MFX_ERR_NONE;
    }
};

#endif /* mfx_tracer_MOCKS_H */
