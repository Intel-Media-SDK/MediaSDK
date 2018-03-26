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

enum HWType
{
    MFX_HW_UNKNOWN   = 0,
    MFX_HW_LAKE      = 0x100000,
    MFX_HW_LRB       = 0x200000,
    MFX_HW_SNB       = 0x300000,

    MFX_HW_IVB       = 0x400000,

    MFX_HW_HSW       = 0x500000,
    MFX_HW_HSW_ULT   = 0x500001,

    MFX_HW_VLV       = 0x600000,

    MFX_HW_BDW       = 0x700000,

    MFX_HW_CHV       = 0x800000,

    MFX_HW_SKL       = 0x900000,

    MFX_HW_APL       = 0xa00000,

    MFX_HW_KBL       = 0xb00000,
    MFX_HW_GLK       = MFX_HW_KBL + 1,
    MFX_HW_CFL       = MFX_HW_KBL + 2,

    MFX_HW_CNL       = 0xc00000,

    MFX_HW_ICL       = 0xd00000,

    MFX_HW_LKF       = MFX_HW_ICL + 10,

    MFX_HW_TGL       = 0xe00000,
};

enum OSFamily
{
    MFX_OS_FAMILY_UNKNOWN = 0,
    MFX_OS_FAMILY_WINDOWS,
    MFX_OS_FAMILY_LINUX
};

enum OSWinVersion
{
    MFX_WIN_VER_UNKNOWN = 0,
    MFX_WIN_VER_W7,
    MFX_WIN_VER_W81,
    MFX_WIN_VER_WS2012R2,
    MFX_WIN_VER_W10,
    MFX_WIN_VER_WS2016
};
