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

#include <memory>
#include "umc_h265_au_splitter.h"
#include "umc_h265_nal_spl.h"

namespace UMC_HEVC_DECODER
{

AU_Splitter_H265::AU_Splitter_H265()
    : m_Headers(&m_ObjHeap)
{
}

AU_Splitter_H265::~AU_Splitter_H265()
{
    Close();
}

void AU_Splitter_H265::Init(UMC::VideoDecoderParams *)
{
    Close();

    m_pNALSplitter.reset(new NALUnitSplitter_H265());
    m_pNALSplitter->Init();
}

void AU_Splitter_H265::Close()
{
    m_pNALSplitter.reset(0);
    m_Headers.Reset(false);
    m_ObjHeap.Release();
}

void AU_Splitter_H265::Reset()
{
    if (m_pNALSplitter.get())
        m_pNALSplitter->Reset();

    m_Headers.Reset(false);
    m_ObjHeap.Release();
}

// Wrapper for NAL unit splitter CheckNalUnitType
int32_t AU_Splitter_H265::CheckNalUnitType(UMC::MediaData * src)
{
    return m_pNALSplitter->CheckNalUnitType(src);
}

// Wrapper for NAL unit splitter CheckNalUnitType GetNalUnit
UMC::MediaDataEx * AU_Splitter_H265::GetNalUnit(UMC::MediaData * src)
{
    return m_pNALSplitter->GetNalUnits(src);
}

// Returns internal NAL unit splitter
NALUnitSplitter_H265 * AU_Splitter_H265::GetNalUnitSplitter()
{
    return m_pNALSplitter.get();
}


} // namespace UMC_HEVC_DECODER
#endif // MFX_ENABLE_H265_VIDEO_DECODE
