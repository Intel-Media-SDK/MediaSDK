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

#define CHECK_MVC_SUPPORT()                                                                                                               \
{                                                                                                                                         \
    if(g_tsOSFamily == MFX_OS_FAMILY_LINUX)                                                                                               \
    {                                                                                                                                     \
        g_tsLog << "[ SKIPPED ] MVC is not supported for linux platform\n";                                                               \
        return 0;                                                                                                                         \
    }                                                                                                                                     \
    else if(g_tsOSFamily == MFX_OS_FAMILY_UNKNOWN)                                                                                        \
    {                                                                                                                                     \
        g_tsLog << "WARNING:\n";                                                                                                          \
        g_tsLog << "The platform is not specified.\n";                                                                                    \
        g_tsLog << "If you are using Linux platform you have to get MVC tests failure because MVC is not supported for Linux platform!\n";\
        g_tsLog << "To skip the tests on the Linux platform you need to set environment variable \"TS_PLATFORM\" before running tests.\n";\
        g_tsLog << "For example for Linux paltform the variable can be \"TS_PLATFORM=c7.2_bdw_64\"\n";                                    \
    }                                                                                                                                     \
}

