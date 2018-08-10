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

#include "umc_defs.h"
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#ifndef __UMC_H264_DEC_DEFS_YUV_H__
#define __UMC_H264_DEC_DEFS_YUV_H__

#include "umc_h264_dec_defs_dec.h"
#include "umc_video_decoder.h"
#include "umc_frame_data.h"

namespace UMC
{

class H264DecYUVBufferPadded
{
    DYNAMIC_CAST_DECL_BASE(H264DecYUVBufferPadded)

public:

    int32_t  m_bpp;           // should be >= 8
    int32_t  m_chroma_format; // AVC standard value chroma_format_idc

    PlanePtrYCommon               m_pYPlane;

    PlanePtrUVCommon              m_pUVPlane;  // for NV12 support

    PlanePtrUVCommon              m_pUPlane;
    PlanePtrUVCommon              m_pVPlane;

    H264DecYUVBufferPadded();
    virtual ~H264DecYUVBufferPadded();

    void Init(const VideoDataInfo *info);

    void allocate(const FrameData * frameData, const VideoDataInfo *info);
    void deallocate();

    const mfxSize& lumaSize() const { return m_lumaSize; }
    const mfxSize& chromaSize() const { return m_chromaSize; }

    uint32_t pitch_luma() const { return m_pitch_luma; }
    uint32_t pitch_chroma() const { return m_pitch_chroma; }

    void setImageSize(mfxSize dimensions, int32_t chroma_format)
    {
        m_lumaSize.width = dimensions.width;
        m_lumaSize.height = dimensions.height;

        dimensions.width = dimensions.width >> ((int32_t) (3 > chroma_format));
        dimensions.height = dimensions.height >> ((int32_t) (2 > chroma_format));

        m_chromaSize.width = dimensions.width;
        m_chromaSize.height = dimensions.height;

        m_chroma_format = chroma_format;
    }

    FrameData * GetFrameData();
    const FrameData * GetFrameData() const;

    ColorFormat GetColorFormat() const;

protected:

    mfxSize            m_lumaSize;
    mfxSize            m_chromaSize;

    int32_t  m_pitch_luma;
    int32_t  m_pitch_chroma;

    FrameData m_frameData;

    ColorFormat m_color_format;
};

} // namespace UMC

#endif // __UMC_H264_DEC_DEFS_YUV_H__
#endif // MFX_ENABLE_H264_VIDEO_DECODE
