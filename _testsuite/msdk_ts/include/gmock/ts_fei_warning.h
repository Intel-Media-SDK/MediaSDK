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

#pragma once

#define CHECK_FEI_SUPPORT()                                                                                                                     \
{                                                                                                                                               \
    if(g_tsOSFamily == MFX_OS_FAMILY_WINDOWS)                                                                                                   \
    {                                                                                                                                           \
        g_tsLog << "[ SKIPPED ] FEI is not supported for windows platform\n";                                                                   \
        return 0;                                                                                                                               \
    }                                                                                                                                           \
    else if(g_tsOSFamily == MFX_OS_FAMILY_UNKNOWN)                                                                                              \
    {                                                                                                                                           \
        g_tsLog << "WARNING:\n";                                                                                                                \
        g_tsLog << "The platform is not specified.\n";                                                                                          \
        g_tsLog << "If you are using Windows platform you have to get FEI tests failure because FEI is not supported for Windows platform!\n";  \
        g_tsLog << "To skip the tests on the Windows platform you need to set environment variable \"TS_PLATFORM=win\" before running tests.\n";\
        g_tsLog << "You can do it in the msdk_gmock project properties: Configuration Properties - Debbuging tab - Environment field\n";        \
    }                                                                                                                                           \
}

#define CHECK_HEVC_FEI_SUPPORT()                                                                                                                \
{                                                                                                                                               \
    if (g_tsImpl == MFX_IMPL_HARDWARE) {                                                                                                        \
        if (g_tsHWtype < MFX_HW_SKL) {                                                                                                          \
            g_tsLog << "[ SKIPPED ] HEVC FEI is not supported on current platform (SKL+ required)\n";                                           \
            return 0;                                                                                                                           \
        }                                                                                                                                       \
    }                                                                                                                                           \
    CHECK_FEI_SUPPORT();                                                                                                                        \
}

#define SKIP_IF_LINUX                                                                                                                           \
{                                                                                                                                               \
    if(g_tsOSFamily != MFX_OS_FAMILY_WINDOWS)                                                                                                   \
    {                                                                                                                                           \
        g_tsLog << "[ SKIPPED ] This test is not supported for linux platform\n";                                                               \
        return 0;                                                                                                                               \
    }                                                                                                                                           \
}
#define SKIP_IF_WINDOWS                                                                                                                         \
{                                                                                                                                               \
    if(g_tsOSFamily == MFX_OS_FAMILY_WINDOWS)                                                                                                   \
    {                                                                                                                                           \
        g_tsLog << "[ SKIPPED ] This test is not supported for windows platform\n";                                                             \
        return 0;                                                                                                                               \
    }                                                                                                                                           \
}
