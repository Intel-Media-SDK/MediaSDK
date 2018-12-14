// Copyright (c) 2017 Intel Corporation
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

#ifndef __MFX_VPX_DEC_COMMON__H__
#define __MFX_VPX_DEC_COMMON__H__

#include "mfx_config.h"
#include "mfx_common_int.h"

namespace MFX_VPX_Utility
{
    mfxStatus Query(VideoCORE*, mfxVideoParam const* in, mfxVideoParam* out, mfxU32 codecId, eMFXHWType type);

    bool CheckFrameInfo(const mfxFrameInfo &frameInfo,  mfxU32 codecId, eMFXPlatform platform = MFX_PLATFORM_HARDWARE,  eMFXHWType hwtype = MFX_HW_UNKNOWN);
    bool CheckInfoMFX(const mfxInfoMFX &mfx, mfxU32 codecId, eMFXPlatform platform = MFX_PLATFORM_HARDWARE,  eMFXHWType hwtype = MFX_HW_UNKNOWN);
    bool CheckVideoParam(mfxVideoParam const*, mfxU32 codecId, eMFXPlatform platform = MFX_PLATFORM_HARDWARE,  eMFXHWType hwtype = MFX_HW_UNKNOWN);
    mfxStatus QueryIOSurfInternal(mfxVideoParam const*, mfxFrameAllocRequest*);

}

#endif //__MFX_VPX_DEC_COMMON__H__
