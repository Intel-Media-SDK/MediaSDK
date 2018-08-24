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

// This file should be included by exactly one cpp in each MediaSDK plugin

#pragma once

/* These string constants set Media SDK version information for Linux, Android, OSX. */
#ifndef MFX_PLUGIN_FILE_VERSION
#define MFX_PLUGIN_FILE_VERSION "0.0.0.0"
#endif
#ifndef MFX_PLUGIN_PRODUCT_NAME
#define MFX_PLUGIN_PRODUCT_NAME "Intel(R) Media SDK Plugin"
#endif
#ifndef MFX_PLUGIN_PRODUCT_VERSION
#define MFX_PLUGIN_PRODUCT_VERSION "0.0.000.0000"
#endif
#ifndef MFX_FILE_DESCRIPTION
#define MFX_FILE_DESCRIPTION "File info ought to be here"
#endif

const char* g_MfxProductName = "mediasdk_product_name: " MFX_PLUGIN_PRODUCT_NAME;
const char* g_MfxCopyright = "mediasdk_copyright: Copyright(c) 2007-2018 Intel Corporation";
const char* g_MfxProductVersion = "mediasdk_product_version: " MFX_PLUGIN_PRODUCT_VERSION;
const char* g_MfxFileVersion = "mediasdk_file_version: " MFX_PLUGIN_FILE_VERSION;
const char* g_MfxFileDescription = "mediasdk_file_description: " MFX_FILE_DESCRIPTION;

