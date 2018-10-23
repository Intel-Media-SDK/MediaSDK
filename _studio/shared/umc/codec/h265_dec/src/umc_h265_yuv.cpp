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
#ifdef MFX_ENABLE_H265_VIDEO_DECODE
#include "umc_h265_yuv.h"
#include "umc_h265_frame.h"

namespace UMC_HEVC_DECODER
{
H265DecYUVBufferPadded::H265DecYUVBufferPadded()
    : m_chroma_format(CHROMA_FORMAT_420)
    , m_pYPlane(NULL)
    , m_pUVPlane(NULL)
    , m_pUPlane(NULL)
    , m_pVPlane(NULL)
    , m_pMemoryAllocator(0)
    , m_midAllocatedBuffer(0)
    , m_pAllocatedBuffer(0)
    , m_lumaSize()
    , m_chromaSize()
    , m_pitch_luma(0)
    , m_pitch_chroma(0)
    , m_color_format(UMC::NV12)
{
}

H265DecYUVBufferPadded::H265DecYUVBufferPadded(UMC::MemoryAllocator *pMemoryAllocator)
    : m_chroma_format(CHROMA_FORMAT_420)
    , m_pYPlane(NULL)
    , m_pUVPlane(NULL)
    , m_pUPlane(NULL)
    , m_pVPlane(NULL)
    , m_pMemoryAllocator(pMemoryAllocator)
    , m_midAllocatedBuffer(0)
    , m_pAllocatedBuffer(0)
    , m_pitch_luma(0)
    , m_pitch_chroma(0)
    , m_color_format(UMC::NV12)
{
    m_lumaSize.width = 0;
    m_lumaSize.height = 0;

    m_chromaSize.width = 0;
    m_chromaSize.height = 0;
}

H265DecYUVBufferPadded::~H265DecYUVBufferPadded()
{
    deallocate();
}

// Returns pointer to FrameData instance
UMC::FrameData * H265DecYUVBufferPadded::GetFrameData()
{
    return &m_frameData;
}

const UMC::FrameData * H265DecYUVBufferPadded::GetFrameData() const
{
    return &m_frameData;
}

// Deallocate all memory
void H265DecYUVBufferPadded::deallocate()
{
    if (m_frameData.GetFrameMID() != UMC::FRAME_MID_INVALID)
    {
        m_frameData.Close();
        return;
    }

    m_pYPlane = m_pUPlane = m_pVPlane = m_pUVPlane = NULL;

    m_lumaSize = {0, 0};
    m_pitch_luma = 0;
    m_pitch_chroma = 0;
}

// Initialize variables to default values
void H265DecYUVBufferPadded::Init(const UMC::VideoDataInfo *info)
{
    if (info == nullptr)
        throw h265_exception(UMC::UMC_ERR_NULL_PTR);
    if (info->GetNumPlanes() == 0)
        throw h265_exception(UMC::UMC_ERR_NULL_PTR);


    m_color_format = info->GetColorFormat();
    m_chroma_format = GetH265ColorFormat(info->GetColorFormat());
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

// Allocate YUV frame buffer planes and initialize pointers to it.
// Used to contain decoded frames.
void H265DecYUVBufferPadded::allocate(const UMC::FrameData * frameData, const UMC::VideoDataInfo *info)
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

    m_chroma_format = GetH265ColorFormat(info->GetColorFormat());
    m_lumaSize = info->GetPlaneInfo(0)->m_ippSize;
    m_pitch_luma = (int32_t)m_frameData.GetPlaneMemoryInfo(0)->m_pitch / info->GetPlaneSampleSize(0);

    m_pYPlane = (PlanePtrY)m_frameData.GetPlaneMemoryInfo(0)->m_planePtr;

    if ((m_chroma_format > 0 || GetH265ColorFormat(frameData->GetInfo()->GetColorFormat()) > 0) &&
        (info->GetNumPlanes() >= 2))
    {
        if (m_chroma_format == 0)
            info = frameData->GetInfo();
        m_chromaSize = info->GetPlaneInfo(1)->m_ippSize;
        m_pitch_chroma = (int32_t)m_frameData.GetPlaneMemoryInfo(1)->m_pitch / info->GetPlaneSampleSize(1);

        if (m_frameData.GetInfo()->GetNumPlanes() == 2)
        {
            m_pUVPlane = (PlanePtrUV)m_frameData.GetPlaneMemoryInfo(1)->m_planePtr;
            m_pUPlane = 0;
            m_pVPlane = 0;
        }
        else
        {
            VM_ASSERT(m_frameData.GetInfo()->GetNumPlanes() == 3);
            m_pUPlane = (PlanePtrUV)m_frameData.GetPlaneMemoryInfo(1)->m_planePtr;
            m_pVPlane = (PlanePtrUV)m_frameData.GetPlaneMemoryInfo(2)->m_planePtr;
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

// Returns color formap of allocated frame
UMC::ColorFormat H265DecYUVBufferPadded::GetColorFormat() const
{
    return m_color_format;
}

// Allocate memory and initialize frame plane pointers and pitches.
// Used for temporary picture buffers, e.g. residuals.
void H265DecYUVBufferPadded::createPredictionBuffer(const H265SeqParamSet * sps)
{
    uint32_t ElementSizeY = sizeof(int16_t);
    uint32_t ElementSizeUV = sizeof(int16_t);
    m_lumaSize.width = sps->MaxCUSize;
    m_lumaSize.height = sps->MaxCUSize;

    m_chroma_format = sps->ChromaArrayType;
    m_chromaSize.width = sps->MaxCUSize / sps->SubWidthC();
    m_chromaSize.height = sps->MaxCUSize / sps->SubHeightC();

    m_pitch_luma = sps->MaxCUSize;
    m_pitch_chroma = sps->MaxCUSize;

    size_t allocationSize = (m_lumaSize.height) * m_pitch_luma * ElementSizeY +
        (m_chromaSize.height) * m_pitch_chroma * ElementSizeUV*2 + 512;

    m_pAllocatedBuffer = h265_new_array_throw<uint8_t>((int32_t)allocationSize);
    m_pYPlane = UMC::align_pointer<PlanePtrY>(m_pAllocatedBuffer, 64);

    m_pUVPlane = m_pUPlane = UMC::align_pointer<PlanePtrY>(m_pYPlane + (m_lumaSize.height) * m_pitch_luma * ElementSizeY + 128, 64);
    m_pVPlane = m_pUPlane + m_chromaSize.height * m_chromaSize.width * ElementSizeUV;
}

// Deallocate planes memory
void H265DecYUVBufferPadded::destroy()
{
    delete [] m_pAllocatedBuffer;
    m_pAllocatedBuffer = NULL;
}

} // namespace UMC_HEVC_DECODER
#endif // MFX_ENABLE_H265_VIDEO_DECODE
