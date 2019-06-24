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

#include "mfx_common.h"
#include "mfx_ext_buffers.h"
#include "mfx_vp9_encode_hw_ddi.h"
#include "mfxvp9.h"

namespace MfxHwVP9Encode
{

mfxStatus CheckExtBufferHeaders(mfxU16 numExtParam, mfxExtBuffer** extParam, bool isRuntime = false);

mfxStatus SetSupportedParameters(mfxVideoParam & par);

mfxStatus CheckParameters(VP9MfxVideoParam &par, ENCODE_CAPS_VP9 const &caps);

mfxStatus CheckParametersAndSetDefaults(
    VP9MfxVideoParam &par,
    ENCODE_CAPS_VP9 const &caps);

mfxStatus SetDefaults(
    VP9MfxVideoParam &par,
    ENCODE_CAPS_VP9 const &caps);

void InheritDefaults(VP9MfxVideoParam& defaultsDst, VP9MfxVideoParam const & defaultsSrc);

mfxStatus CheckSurface(VP9MfxVideoParam const & video,
                       mfxFrameSurface1 const & surface,
                       mfxU32 initWidth,
                       mfxU32 initHeight,
                       ENCODE_CAPS_VP9 const &caps);

mfxStatus CheckAndFixCtrl(VP9MfxVideoParam const & video,
                          mfxEncodeCtrl & ctrl,
                          ENCODE_CAPS_VP9 const & caps);

mfxStatus CheckBitstream(VP9MfxVideoParam const & video, mfxBitstream * bs);

void SetDefaultsForProfileAndFrameInfo(VP9MfxVideoParam& par);

bool CheckAndFixQIndexDelta(mfxI16& qIndexDelta, mfxU16 qIndex);
} // MfxHwVP9Encode