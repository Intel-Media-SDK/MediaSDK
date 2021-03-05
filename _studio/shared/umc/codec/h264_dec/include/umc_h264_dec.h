// Copyright (c) 2020 Intel Corporation
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

#include "umc_defs.h"
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#ifndef __UMC_H264_DEC_H__
#define __UMC_H264_DEC_H__

#include "umc_video_decoder.h"

namespace UMC
{

class H264VideoDecoderParams : public VideoDecoderParams
{
public:
    DYNAMIC_CAST_DECL(H264VideoDecoderParams, VideoDecoderParams);

    enum
    {
        ENTROPY_CODING_CAVLC = 0,
        ENTROPY_CODING_CABAC = 1
    };

    enum
    {
        H264_PROFILE_UNKNOWN            = 0,
        H264_PROFILE_BASELINE           = 66,
        H264_PROFILE_MAIN               = 77,
        H264_PROFILE_SCALABLE_BASELINE  = 83,
        H264_PROFILE_SCALABLE_HIGH      = 86,
        H264_PROFILE_EXTENDED           = 88,
        H264_PROFILE_HIGH               = 100,
        H264_PROFILE_HIGH10             = 110,
        H264_PROFILE_MULTIVIEW_HIGH     = 118,
        H264_PROFILE_HIGH422            = 122,
        H264_PROFILE_STEREO_HIGH        = 128,
        H264_PROFILE_HIGH444            = 144,
        H264_PROFILE_ADVANCED444_INTRA  = 166,
        H264_PROFILE_ADVANCED444        = 188,
        H264_PROFILE_HIGH444_PRED       = 244,
        H264_PROFILE_CAVLC444_INTRA     = 44
    };

    enum
    {
        H264_LEVEL_UNKNOWN = 0,
        H264_LEVEL_1    = 10,
        H264_LEVEL_11   = 11,
        H264_LEVEL_1b   = 11,
        H264_LEVEL_12   = 12,
        H264_LEVEL_13   = 13,

        H264_LEVEL_2    = 20,
        H264_LEVEL_21   = 21,
        H264_LEVEL_22   = 22,

        H264_LEVEL_3    = 30,
        H264_LEVEL_31   = 31,
        H264_LEVEL_32   = 32,

        H264_LEVEL_4    = 40,
        H264_LEVEL_41   = 41,
        H264_LEVEL_42   = 42,

        H264_LEVEL_5    = 50,
        H264_LEVEL_51   = 51,
        H264_LEVEL_52   = 52,

#if (MFX_VERSION >= 1035)
        H264_LEVEL_6    = 60,
        H264_LEVEL_61   = 61,
        H264_LEVEL_62   = 62,
#endif

#if (MFX_VERSION >= 1035)
        H264_LEVEL_MAX  = 62,
#else
        H264_LEVEL_MAX  = 52,
#endif

        H264_LEVEL_9    = 9  // for SVC profiles
    };

    H264VideoDecoderParams()
        : m_entropy_coding_type(ENTROPY_CODING_CAVLC)
        , m_DPBSize(16)
        , m_auxiliary_format_idc(0)
        , m_bufferedFrames(0)
    {
        m_fullSize.width = 0;
        m_fullSize.height = 0;

        m_cropArea.top = 0;
        m_cropArea.bottom = 0;
        m_cropArea.left = 0;
        m_cropArea.right = 0;

        m_ignore_level_constrain = false;
    }

    int32_t m_entropy_coding_type;
    int32_t m_DPBSize;
    int32_t m_auxiliary_format_idc;
    mfxSize m_fullSize;
    UMC::sRECT m_cropArea;

    bool m_ignore_level_constrain;

    int32_t m_bufferedFrames;
};

} // namespace UMC

#endif // __UMC_H264_DEC_H__
#endif // MFX_ENABLE_H264_VIDEO_DECODE
