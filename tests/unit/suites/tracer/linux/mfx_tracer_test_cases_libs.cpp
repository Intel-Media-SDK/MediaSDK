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

#include "mfx_tracer_test_main.h"
#include "mfx_tracer_test_fixtures.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mfxvideo.h>
#include <algorithm>
#include <type_traits>
#include "mfx_tracer_test_mocks.h"

enum
{
    MFX_INIT     = 0x0001,
    MFX_INIT_EX  = 0x0002,
};

auto func_case_list = ::testing::Values(
        MFX_INIT,
        MFX_INIT_EX);

TEST_P(TracerLibsTestParametrizedFunc, ShouldSucceedForSeeminglyGoodMockLibrary)
{
    MockCallObj& mock = *g_call_obj_ptr;
    mfxStatus sts = MFX_ERR_UNKNOWN;

    mfxI32 func = GetParam();

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, dlclose).Times(1);

    if (func & MFX_INIT) 
    {
        EXPECT_CALL(mock, MFXInit).Times(1).WillRepeatedly(DoAll(SetArgPointee<2>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
        sts = MFXInit(impl, &ver, &session);
    }
    else if (func & MFX_INIT_EX)
    {
        EXPECT_CALL(mock, MFXInitEx).Times(1).WillRepeatedly(DoAll(SetArgPointee<1>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
        sts = MFXInitEx(par, &session);
    }
    ASSERT_EQ(sts, MFX_ERR_NONE);

    EXPECT_CALL(mock, MFXClose(MOCK_SESSION_HANDLE)).Times(1);
    sts = MFXClose(session);
    ASSERT_EQ(sts, MFX_ERR_NONE);
}

TEST_P(TracerLibsTestParametrizedFunc, MFXInitShouldFailIfNoLibraryIsFound)
{
    MockCallObj& mock = *g_call_obj_ptr;
    mfxStatus sts = MFX_ERR_UNKNOWN;

    mfxI32 func = GetParam();

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1));
    EXPECT_CALL(mock, dlsym).Times(0);
    EXPECT_CALL(mock, dlclose).Times(0);
    
    if (func & MFX_INIT) 
    {
        sts = MFXInit(impl, &ver, &session);
    }
    else if (func & MFX_INIT_EX)
    {
        sts = MFXInitEx(par, &session);
    }
    ASSERT_EQ(sts, MFX_ERR_NOT_FOUND);
}


auto impl_case_list = ::testing::Values(
        MFX_IMPL_AUTO,
        MFX_IMPL_HARDWARE,
        MFX_IMPL_HARDWARE_ANY,
        MFX_IMPL_HARDWARE2,
        MFX_IMPL_HARDWARE3,
        MFX_IMPL_HARDWARE4);

TEST_P(TracerLibsTestParametrizedImpl, MFXInitShouldEnumerateCorrectLibNames)
{
    MockCallObj& mock = *g_call_obj_ptr;

#if defined(__i386__)
    const std::string LIBMFXHW("libmfxhw32.so.1");
#elif defined(__x86_64__)
    const std::string LIBMFXHW("libmfxhw64.so.1");
#endif

    std::string modules_dir(MFX_MODULES_DIR);
    mfxIMPL impl = std::get<0>(GetParam());
    mfxI32  func = std::get<1>(GetParam());
    std::vector<std::string> libs;

    if ((impl & MFX_IMPL_HARDWARE) ||
             (impl & MFX_IMPL_HARDWARE_ANY))
    {
        libs.emplace_back(LIBMFXHW);
        libs.emplace_back(modules_dir + "/" + LIBMFXHW);
    }

    for (const std::string& lib : libs)
    {
        EXPECT_CALL(mock, dlopen(StrEq(lib.c_str()),_)).Times(1);
    }

    if (func & MFX_INIT) 
    {
        MFXInit(impl, &this->ver, &this->session);
    }
    else if (func & MFX_INIT_EX)
    {
        MFXInitEx(par, &this->session);
    }
}

INSTANTIATE_TEST_CASE_P(EnumeratingLibs, TracerLibsTestParametrizedImpl,
    ::testing::Combine(impl_case_list, func_case_list));

TEST_P(TracerLibsTestParametrizedFunc, ShouldFailIfAvailLibHasNoSymbols)
{
    MockCallObj& mock = *g_call_obj_ptr;
    mfxStatus sts = MFX_ERR_UNKNOWN;
 
    mfxI32 func = GetParam();

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(mock, dlclose).Times(AtLeast(1));

    if (func & MFX_INIT) 
    {
        sts = MFXInit(impl, &ver, &session);
    }
    else if (func & MFX_INIT_EX)
    {
        sts = MFXInitEx(par, &session);
    }
    EXPECT_EQ(sts, MFX_ERR_NOT_FOUND);
}

TEST_P(TracerLibsTestParametrizedFunc, ShouldSucceedIfFirstLibBadSecondLibGood)
{
    MockCallObj& mock = *g_call_obj_ptr;
    mfxStatus sts;
 
    mfxI32 func = GetParam();

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillOnce(Return(nullptr)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, dlclose).Times(1);

    if (func & MFX_INIT)
    {
        EXPECT_CALL(mock, MFXInit).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<2>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
        sts = MFXInit(impl, &ver, &session);
    }
    else if (func & MFX_INIT_EX)
    {
        EXPECT_CALL(mock, MFXInitEx).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<1>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
        sts = MFXInitEx(par, &session);
    }
    EXPECT_EQ(sts, MFX_ERR_NONE);

    EXPECT_CALL(mock, MFXClose(MOCK_SESSION_HANDLE)).Times(1);
    sts = MFXClose(session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillOnce(Return(nullptr)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, dlclose).Times(AtLeast(1));

    if (func & MFX_INIT)
    {
        EXPECT_CALL(mock, MFXInit).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<2>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
        sts = MFXInit(impl, &ver, &session);
    }
    else if (func & MFX_INIT_EX)
    {
        EXPECT_CALL(mock, MFXInitEx).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<1>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
        sts = MFXInitEx(par, &session);
    }
    EXPECT_EQ(sts, MFX_ERR_NONE);

    EXPECT_CALL(mock, MFXClose(MOCK_SESSION_HANDLE)).Times(1);
    sts = MFXClose(session);
    ASSERT_EQ(sts, MFX_ERR_NONE);
}

INSTANTIATE_TEST_CASE_P(TracerFuncTest, TracerLibsTestParametrizedFunc, func_case_list);
