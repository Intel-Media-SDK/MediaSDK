// Copyright (c) 2017-2020 Intel Corporation
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

#ifndef __MFX_MPEG2_ENC_COMMON_HW_H__
#define __MFX_MPEG2_ENC_COMMON_HW_H__

#include "mfx_common.h"
#include "mfx_ext_buffers.h"

#if defined (MFX_ENABLE_MPEG2_VIDEO_ENCODE)

#define D3DDDIFORMAT        D3DFORMAT
#define DXVADDI_VIDEODESC   DXVA2_VideoDesc

#if defined(MFX_VA_LINUX)
    #include "mfx_h264_encode_struct_vaapi.h"
#endif

#include "mfxstructures.h"

#include <vector>
#include <list>





#define ENCODE_ENC_CTRL_CAPS ENCODE_ENC_CTRL_CAPS

//#define __SW_ENC


//#define MPEG2_ENCODE_HW_PERF
//#define MPEG2_ENC_HW_PERF
//#define MPEG2_ENCODE_DEBUG_HW


    enum
    {
      MFX_MPEG2_TOP_FIELD     = 1,
      MFX_MPEG2_BOTTOM_FIELD  = 2,
      MFX_MPEG2_FRAME_PICTURE = 3
    };

    struct mfxVideoParamEx_MPEG2
    {
        mfxVideoParam           mfxVideoParams;
        bool                    bFieldCoding;
        mfxU32                  MVRangeP[2];
        mfxU32                  MVRangeB[2][2];
        bool                    bAllowFieldPrediction;
        bool                    bAllowFieldDCT;
        bool                    bAddEOS;
        bool                    bRawFrames;
        mfxFrameAllocResponse*  pRecFramesResponse_hw;
        mfxFrameAllocResponse*  pRecFramesResponse_sw;

        mfxExtVideoSignalInfo   videoSignalInfo;
        bool                    bAddDisplayExt;
        bool                    bMbqpMode;
        bool                    bDisablePanicMode;
    };

namespace MfxHwMpeg2Encode
{
   #define NUM_FRAMES 800


    struct ExtVASurface
    {
        VASurfaceID surface;
        mfxU32 number;
        mfxU32 idxBs;
    };
    
    typedef std::vector<ExtVASurface> mfxRecFrames;
    typedef std::vector<ExtVASurface> mfxRawFrames;



    typedef std::vector<ExtVASurface> mfxFeedback;


    mfxStatus QueryHwCaps(VideoCORE* core,
        ENCODE_CAPS & hwCaps,
        mfxU16 codecProfile);

}; // namespace MfxHwMpeg2Encode

#endif
#endif
/* EOF */
