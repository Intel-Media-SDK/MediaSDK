// Copyright (c) 2017-2019 Intel Corporation
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

#include "umc_h264_dec_defs_yuv.h"

namespace UMC
{

H264DecYUVBufferPadded::H264DecYUVBufferPadded()
    : m_bpp(0)
    , m_chroma_format(0)
    , m_pYPlane(0)
    , m_pUVPlane(0)
    , m_pUPlane(0)
    , m_pVPlane(0)
    , m_lumaSize()
    , m_chromaSize()
    , m_pitch_luma(0)
    , m_pitch_chroma(0)
    , m_color_format(NV12)
{
}

H264DecYUVBufferPadded::~H264DecYUVBufferPadded()
{
    deallocate();
}

FrameData * H264DecYUVBufferPadded::GetFrameData()
{
    return &m_frameData;
}

const FrameData * H264DecYUVBufferPadded::GetFrameData() const
{
    return &m_frameData;
}

void H264DecYUVBufferPadded::deallocate()
{
    if (m_frameData.GetFrameMID() != FRAME_MID_INVALID)
    {
        m_frameData.Close();
        return;
    }

    m_pYPlane = m_pUPlane = m_pVPlane = m_pUVPlane = 0;

    m_lumaSize = {0, 0};
    m_pitch_luma = 0;
    m_pitch_chroma = 0;
}

void H264DecYUVBufferPadded::Init(const VideoDataInfo *info)
{
    if (info == nullptr)
        throw h264_exception(UMC_ERR_NULL_PTR);
    if (info->GetNumPlanes() == 0)
        throw h264_exception(UMC_ERR_NULL_PTR);

    m_bpp = std::max(info->GetPlaneBitDepth(0), info->GetPlaneBitDepth(1));

    m_color_format = info->GetColorFormat();
    m_chroma_format = GetH264ColorFormat(info->GetColorFormat());
    m_lumaSize = info->GetPlaneInfo(0)->m_ippSize;
    m_pYPlane = 0;
    m_pUPlane = 0;
    m_pVPlane = 0;
    m_pUVPlane = 0;

    if ((m_chroma_format > 0) && (info->GetNumPlanes() >= 2))
    {
        m_chromaSize = info->GetPlaneInfo(1)->m_ippSize;
    }
    else
    {
        m_chromaSize = {0, 0};
    }
}

void H264DecYUVBufferPadded::allocate(const FrameData * frameData, const VideoDataInfo *info)
{
    if (info == nullptr || frameData == nullptr || info->GetNumPlanes() == 0)
    {
        deallocate();
        return;
    }

    m_frameData = *frameData;

    if (frameData->GetPlaneMemoryInfo(0)->m_planePtr)
        m_frameData.m_locked = true;

    m_color_format = info->GetColorFormat();
    m_bpp = std::max(info->GetPlaneBitDepth(0), info->GetPlaneBitDepth(1));

    m_chroma_format = GetH264ColorFormat(info->GetColorFormat());

    m_lumaSize = info->GetPlaneInfo(0)->m_ippSize;
    m_pitch_luma = (int32_t)m_frameData.GetPlaneMemoryInfo(0)->m_pitch / info->GetPlaneInfo(0)->m_iSampleSize;

    m_pYPlane = m_frameData.GetPlaneMemoryInfo(0)->m_planePtr;

    if ((m_chroma_format > 0 || GetH264ColorFormat(frameData->GetInfo()->GetColorFormat()) > 0) &&
        (info->GetNumPlanes() >= 2))
    {
        if (m_chroma_format == 0)
            info = frameData->GetInfo();

        m_chromaSize = info->GetPlaneInfo(1)->m_ippSize;
        m_pitch_chroma = (int32_t)m_frameData.GetPlaneMemoryInfo(1)->m_pitch / info->GetPlaneInfo(1)->m_iSampleSize;

        if (m_frameData.GetInfo()->GetNumPlanes() == 2)
        {
            m_pUVPlane = m_frameData.GetPlaneMemoryInfo(1)->m_planePtr;
            m_pUPlane = 0;
            m_pVPlane = 0;
        }
        else
        {
            VM_ASSERT(m_frameData.GetInfo()->GetNumPlanes() == 3);
            m_pUPlane = m_frameData.GetPlaneMemoryInfo(1)->m_planePtr;
            m_pVPlane = m_frameData.GetPlaneMemoryInfo(2)->m_planePtr;
            m_pUVPlane = 0;
        }
    }
    else
    {
        m_chromaSize = {0, 0};
        m_pitch_chroma = 0;
        m_pUPlane = 0;
        m_pVPlane = 0;
    }
}

ColorFormat H264DecYUVBufferPadded::GetColorFormat() const
{
    return m_color_format;
}

} // namespace UMC
#endif // MFX_ENABLE_H264_VIDEO_DECODE
