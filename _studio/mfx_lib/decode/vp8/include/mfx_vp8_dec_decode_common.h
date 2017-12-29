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

#ifndef _MFX_VP8_DEC_DECODE_COMMON_H_
#define _MFX_VP8_DEC_DECODE_COMMON_H_

#include "mfx_common.h"

namespace VP8DecodeCommon 
{

    mfxStatus DecodeHeader(VideoCORE *p_core, mfxBitstream *p_bs, mfxVideoParam *p_params);

    typedef struct _IVF_FRAME
    {
        uint32_t frame_size;
        unsigned long long time_stamp;
        uint8_t *p_frame_data;

    } IVF_FRAME;

}

class MFX_VP8_Utility
{
public:

    static eMFXPlatform GetPlatform(VideoCORE *pCore, mfxVideoParam *pPar);
    static mfxStatus Query(VideoCORE *pCore, mfxVideoParam *pIn, mfxVideoParam *pOut, eMFXHWType type);

private:

    static bool IsNeedPartialAcceleration(mfxVideoParam * pPar);
};


#endif
