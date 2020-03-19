// Copyright (c) 2007-2020 Intel Corporation
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

// This file have specific information for Linux, Android, OSX.

#pragma once

/* These string constants set Media SDK version information for Linux, Android, OSX. */
#ifdef __linux__
#include "va/va.h"

#ifndef MFX_API_VERSION

#define STRINGIZE(s) #s
#define CONVERT_TO_STRING(s) STRINGIZE(s)

#if MFX_VERSION >= MFX_VERSION_NEXT
#define MFX_API_VERSION CONVERT_TO_STRING(MFX_VERSION_MAJOR) "." CONVERT_TO_STRING(MFX_VERSION_MINOR) "+";
#else
#define MFX_API_VERSION CONVERT_TO_STRING(MFX_VERSION_MAJOR) "." CONVERT_TO_STRING(MFX_VERSION_MINOR);
#endif //MFX_VERSION >= MFX_VERSION_NEXT

#endif //MFX_API_VERSION

#ifndef MFX_GIT_COMMIT
#define MFX_GIT_COMMIT "no git commit info"
#endif
#ifndef MFX_BUILD_INFO
#define MFX_BUILD_INFO "no build info"
#endif

#if !defined(MEDIA_VERSION_STR)
#error Build system should define MEDIA_VERSION_STR.
#endif

const char* g_MfxProductName     = "mediasdk_product_name: Intel(R) Media SDK";
const char* g_MfxCopyright       = "mediasdk_copyright: Copyright(c) 2007-2020 Intel Corporation";
const char* g_MfxApiVersion      = "mediasdk_api_version: " MFX_API_VERSION;
const char* g_MfxVersion         = "mediasdk_version: " MEDIA_VERSION_STR;
const char* g_MfxGitCommit       = "mediasdk_git_commit: " MFX_GIT_COMMIT;
const char* g_MfxBuildInfo       = "mediasdk_build_info: " MFX_BUILD_INFO;
const char* g_MfxLibvaVersion    = "mediasdk_libva_version: " VA_VERSION_S;

#endif // __linux__
