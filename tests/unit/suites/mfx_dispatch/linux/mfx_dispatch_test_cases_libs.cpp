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
#include <mfxvideo.h>
#include <algorithm>
#include <type_traits>
#include "mfx_dispatch_test_mocks.h"
#include "device_ids.h"

TEST_F(DispatcherLibsTest, ShouldSucceedForSeeminglyGoodMockLibrary)
{
    MockCallObj& mock = *g_call_obj_ptr;

    ver = {{MFX_VERSION_MINOR, MFX_VERSION_MAJOR}};
    mock.emulated_api_version = ver;

    EXPECT_CALL(mock, dlopen).Times(1).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, MFXInitEx).Times(1).WillRepeatedly(DoAll(SetArgPointee<1>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
    EXPECT_CALL(mock, MFXQueryIMPL).Times(1).WillRepeatedly(Return(MFX_ERR_NONE));
    EXPECT_CALL(mock, MFXQueryVersion).Times(1).WillRepeatedly(Invoke(&mock, &MockCallObj::ReturnEmulatedVersion));
    EXPECT_CALL(mock, dlclose).Times(1);

    mfxStatus sts = MFXInit(impl, &ver, &session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    // NB: In current mfx_dispatch implementation,
    // *session* is not, in fact, the session handle,
    // but a ptr to the internal mfx_dispatch "Loader" class.
    EXPECT_CALL(mock, MFXClose(MOCK_SESSION_HANDLE)).Times(1);
    sts = MFXClose(session);
    ASSERT_EQ(sts, MFX_ERR_NONE);
}

TEST_F(DispatcherLibsTest, ShouldFailIfNoLibraryIsFound)
{
    MockCallObj& mock = *g_call_obj_ptr;
    EXPECT_CALL(mock, dlopen).Times(AtLeast(1));
    EXPECT_CALL(mock, dlsym).Times(0);
    EXPECT_CALL(mock, dlclose).Times(0);
    mfxStatus sts = MFXInit(impl, &ver, &session);
    ASSERT_EQ(sts, MFX_ERR_UNSUPPORTED);
}


auto impl_case_list = ::testing::Values(MFX_IMPL_AUTO,
        MFX_IMPL_SOFTWARE,
        MFX_IMPL_HARDWARE,
        MFX_IMPL_AUTO_ANY,
        MFX_IMPL_HARDWARE_ANY,
        MFX_IMPL_HARDWARE2,
        MFX_IMPL_HARDWARE3,
        MFX_IMPL_HARDWARE4,
        MFX_IMPL_RUNTIME);

TEST_P(DispatcherLibsTestParametrized, ShouldEnumerateCorrectLibNames)
{
    MockCallObj& mock = *g_call_obj_ptr;

#if defined(__i386__)
    const std::string LIBMFXSW("libmfxsw32.so.1");
    const std::string LIBMFXHW("libmfxhw32.so.1");
    const std::string ONEVPLRT("libmfx-gen.so.1.2");
#elif defined(__x86_64__)
    const std::string LIBMFXSW("libmfxsw64.so.1");
    const std::string LIBMFXHW("libmfxhw64.so.1");
    const std::string ONEVPLRT("libmfx-gen.so.1.2");
#endif

    std::string modules_dir(MFX_MODULES_DIR);
    mfxIMPL impl = GetParam();
    std::vector<std::string> libs;

    eMFXHWType platform = MFX_HW_UNKNOWN;
    auto devices = get_devices();
    if (devices.size()) {
        platform = devices[0].platform;
    }

    if (platform != MFX_HW_UNKNOWN) {
        if (MFX_IMPL_BASETYPE(impl) == MFX_IMPL_AUTO ||
            MFX_IMPL_BASETYPE(impl) == MFX_IMPL_AUTO_ANY) {
            libs.emplace_back(LIBMFXHW);
            libs.emplace_back(modules_dir + "/" + LIBMFXHW);
            libs.emplace_back(LIBMFXSW);
            libs.emplace_back(modules_dir + "/" + LIBMFXSW);
        }
        else if ((impl & MFX_IMPL_HARDWARE) ||
                (impl & MFX_IMPL_HARDWARE_ANY))
        {
            libs.emplace_back(LIBMFXHW);
            libs.emplace_back(modules_dir + "/" + LIBMFXHW);
        }
        else if (impl & MFX_IMPL_SOFTWARE)
        {
            libs.emplace_back(LIBMFXSW);
            libs.emplace_back(modules_dir + "/" + LIBMFXSW);
        }
    }
    else
    {
        libs.emplace_back(ONEVPLRT);
        libs.emplace_back(modules_dir + "/" + ONEVPLRT);
    }

    for (const std::string& lib : libs)
    {
        EXPECT_CALL(mock, dlopen(StrEq(lib.c_str()),_)).Times(1);
    }

    MFXInit(impl, &this->ver, &this->session);
}

INSTANTIATE_TEST_CASE_P(EnumeratingLibs, DispatcherLibsTestParametrized, impl_case_list);

TEST_F(DispatcherLibsTest, ShouldFailIfAvailLibHasNoSymbols)
{
    MockCallObj& mock = *g_call_obj_ptr;

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(mock, dlclose).Times(AtLeast(1));

    mfxStatus sts = MFXInit(impl, &ver, &session);
    EXPECT_EQ(sts, MFX_ERR_UNSUPPORTED);
}

TEST_F(DispatcherLibsTest, ShouldFailIfRequestedLibVersionTooLowForDispatch)
{
    MockCallObj& mock = *g_call_obj_ptr;

    mock.emulated_api_version = {{10, 1}}; // Should fail for no MfxInitEx
    ver = {{28, 1}};

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, dlclose).Times(AtLeast(1));

    mfxStatus sts = MFXInit(impl, &ver, &session);
    EXPECT_EQ(sts, MFX_ERR_UNSUPPORTED);
}

TEST_F(DispatcherLibsTest, ShouldFailIfAvailLibVersionLessThanRequested)
{
    MockCallObj& mock = *g_call_obj_ptr;

    mock.emulated_api_version = {{18, 1}};
    ver = {{28, 1}};

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, MFXInitEx).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<1>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
    EXPECT_CALL(mock, MFXQueryVersion).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::ReturnEmulatedVersion));
    EXPECT_CALL(mock, MFXClose(MOCK_SESSION_HANDLE)).Times(AtLeast(1));
    EXPECT_CALL(mock, dlclose).Times(AtLeast(1));

    mfxStatus sts = MFXInit(impl, &ver, &session);
    EXPECT_EQ(sts, MFX_ERR_UNSUPPORTED);
}

TEST_F(DispatcherLibsTest, ShouldSucceedIfAvailLibVersionLargerThanRequested)
{
    MockCallObj& mock = *g_call_obj_ptr;

    mock.emulated_api_version = {{28, 1}};
    ver = {{18, 1}};

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, MFXInitEx).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<1>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
    EXPECT_CALL(mock, MFXQueryVersion).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::ReturnEmulatedVersion));
    EXPECT_CALL(mock, MFXQueryIMPL).Times(AtLeast(1)).WillRepeatedly(Return(MFX_ERR_NONE));

    mfxStatus sts = MFXInit(impl, &ver, &session);
    EXPECT_EQ(sts, MFX_ERR_NONE);
}

TEST_F(DispatcherLibsTest, ShouldFailIfLibCannotMfxInitEx)
{
    MockCallObj& mock = *g_call_obj_ptr;

    ver = {{MFX_VERSION_MINOR, MFX_VERSION_MAJOR}};
    mock.emulated_api_version = ver;

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, MFXInitEx).Times(AtLeast(1)).WillRepeatedly(Return(MFX_ERR_UNSUPPORTED));
    EXPECT_CALL(mock, MFXClose).Times(AtLeast(1));
    EXPECT_CALL(mock, dlclose).Times(AtLeast(1));

    mfxStatus sts = MFXInit(impl, &ver, &session);
    EXPECT_EQ(sts, MFX_ERR_UNSUPPORTED);
}

TEST_F(DispatcherLibsTest, ShouldFailIfLibCannotMfxQueryVersion)
{
    MockCallObj& mock = *g_call_obj_ptr;

    ver = {{MFX_VERSION_MINOR, MFX_VERSION_MAJOR}};
    mock.emulated_api_version = ver;

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, MFXInitEx).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<1>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
    EXPECT_CALL(mock, MFXQueryVersion).Times(AtLeast(1)).WillRepeatedly(Return(MFX_ERR_UNSUPPORTED));
    EXPECT_CALL(mock, MFXClose).Times(AtLeast(1));
    EXPECT_CALL(mock, dlclose).Times(AtLeast(1));

    mfxStatus sts = MFXInit(impl, &ver, &session);
    EXPECT_EQ(sts, MFX_ERR_UNSUPPORTED);
}

TEST_F(DispatcherLibsTest, ShouldFailIfLibReportsWrongVersion)
{
    MockCallObj& mock = *g_call_obj_ptr;

    ver = {{MFX_VERSION_MINOR, MFX_VERSION_MAJOR}};
    mock.emulated_api_version = ver;

    mfxVersion nullversion = {{0}};

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, MFXInitEx).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<1>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
    EXPECT_CALL(mock, MFXQueryVersion).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<1>(nullversion), Return(MFX_ERR_NONE)));
    EXPECT_CALL(mock, MFXClose(MOCK_SESSION_HANDLE)).Times(AtLeast(1));
    EXPECT_CALL(mock, dlclose).Times(AtLeast(1));

    mfxStatus sts = MFXInit(impl, &ver, &session);
    EXPECT_EQ(sts, MFX_ERR_UNSUPPORTED);
}

TEST_F(DispatcherLibsTest, ShouldFailIfImplNotSupportedByLib)
{
    MockCallObj& mock = *g_call_obj_ptr;

    ver = {{MFX_VERSION_MINOR, MFX_VERSION_MAJOR}};
    mock.emulated_api_version = ver;

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, MFXInitEx).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<1>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
    EXPECT_CALL(mock, MFXQueryIMPL).Times(AtLeast(1)).WillRepeatedly(Return(MFX_ERR_UNSUPPORTED));
    EXPECT_CALL(mock, MFXQueryVersion).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::ReturnEmulatedVersion));
    EXPECT_CALL(mock, MFXClose(MOCK_SESSION_HANDLE)).Times(AtLeast(1));
    EXPECT_CALL(mock, dlclose).Times(AtLeast(1));

    mfxStatus sts = MFXInit(impl, &ver, &session);
    EXPECT_EQ(sts, MFX_ERR_UNSUPPORTED);
}

TEST_F(DispatcherLibsTest, ShouldSucceedIfFirstLibBadSecondLibGood)
{
    MockCallObj& mock = *g_call_obj_ptr;

    ver = {{MFX_VERSION_MINOR, MFX_VERSION_MAJOR}};
    mock.emulated_api_version = ver;

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillOnce(Return(nullptr)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, MFXInitEx).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<1>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
    EXPECT_CALL(mock, MFXQueryIMPL).Times(AtLeast(1)).WillRepeatedly(Return(MFX_ERR_NONE));
    EXPECT_CALL(mock, MFXQueryVersion).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::ReturnEmulatedVersion));
    EXPECT_CALL(mock, dlclose).Times(1);

    mfxStatus sts = MFXInit(impl, &ver, &session);
    EXPECT_EQ(sts, MFX_ERR_NONE);

    EXPECT_CALL(mock, MFXClose(MOCK_SESSION_HANDLE)).Times(1);
    sts = MFXClose(session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    EXPECT_CALL(mock, dlopen).Times(AtLeast(1)).WillRepeatedly(Return(MOCK_DLOPEN_HANDLE));
    EXPECT_CALL(mock, dlsym).Times(AtLeast(1)).WillOnce(Return(nullptr)).WillRepeatedly(Invoke(&mock, &MockCallObj::EmulateAPI));
    EXPECT_CALL(mock, MFXInitEx).Times(AtLeast(1)).WillRepeatedly(DoAll(SetArgPointee<1>(MOCK_SESSION_HANDLE), Return(MFX_ERR_NONE)));
    EXPECT_CALL(mock, MFXQueryIMPL).Times(AtLeast(1)).WillRepeatedly(Return(MFX_ERR_NONE));
    EXPECT_CALL(mock, MFXQueryVersion).Times(AtLeast(1)).WillRepeatedly(Invoke(&mock, &MockCallObj::ReturnEmulatedVersion));
    EXPECT_CALL(mock, dlclose).Times(AtLeast(1));

    sts = MFXInit(impl, &ver, &session);
    EXPECT_EQ(sts, MFX_ERR_NONE);

    EXPECT_CALL(mock, MFXClose(MOCK_SESSION_HANDLE)).Times(1);
    sts = MFXClose(session);
    ASSERT_EQ(sts, MFX_ERR_NONE);
}


