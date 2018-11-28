// Copyright (c) 2017-2018 Intel Corporation
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

#ifndef __UMC_H264_AU_SPLITTER_H
#define __UMC_H264_AU_SPLITTER_H

#include <vector>
#include "umc_h264_dec_defs_dec.h"
#include "umc_media_data_ex.h"
#include "umc_h264_heap.h"
#include "umc_h264_slice_decoding.h"
#include "umc_h264_frame_info.h"

#include "umc_h264_headers.h"

namespace UMC
{
class NALUnitSplitter;
class NalUnit;

/****************************************************************************************************/
// SeiPayloadArray class routine
/****************************************************************************************************/
class SeiPayloadArray
{
public:
    SeiPayloadArray();
    SeiPayloadArray(const SeiPayloadArray & payloads);

    ~SeiPayloadArray();

    void Release();

    void AddPayload(UMC_H264_DECODER::H264SEIPayLoad*);

    void MovePayloadsFrom(SeiPayloadArray &messages);

    UMC_H264_DECODER::H264SEIPayLoad* GetPayload(size_t pos) const;
    size_t GetPayloadCount() const;

    UMC_H264_DECODER::H264SEIPayLoad* FindPayload(SEI_TYPE type) const;

protected:
    typedef std::vector<UMC_H264_DECODER::H264SEIPayLoad*> PayloadArray;
    PayloadArray m_payloads;

    int32_t FindPayloadPos(SEI_TYPE type) const;
};

/****************************************************************************************************/
// SetOfSlices class routine
/****************************************************************************************************/
class SetOfSlices
{
public:
    SetOfSlices();
    ~SetOfSlices();

    SetOfSlices(const SetOfSlices& set);

    H264Slice * GetSlice(size_t pos) const;
    size_t GetSliceCount() const;
    void AddSlice(H264Slice * slice);
    void Release();
    void Reset();
    void SortSlices();
    void CleanUseless();

    void AddSet(const SetOfSlices *set);

    size_t GetViewId() const;

    H264DecoderFrame * m_frame;
    bool m_isCompleted;
    bool m_isFull;

    SeiPayloadArray  m_payloads;

    SetOfSlices& operator=(const SetOfSlices& set);

protected:
    std::vector<H264Slice*> m_pSliceQueue;
};

/****************************************************************************************************/
// AccessUnit class routine
/****************************************************************************************************/
class AccessUnit
{
public:

    AccessUnit();
    virtual ~AccessUnit();

    bool AddSlice(H264Slice * slice);

    void CombineSets();

    void Release();

    void Reset();

    bool IsItSliceOfThisAU(H264Slice * slice);

    size_t GetLayersCount() const;
    SetOfSlices * GetLayer(size_t pos);
    SetOfSlices * GetLastLayer();

    void CompleteLastLayer();

    int32_t FindLayerByDependency(int32_t dependency);

    uint32_t GetAUIndentifier() const;

    void SortforASO();

    void CleanUseless();

    bool IsFullAU() const;

    bool m_isInitialized;

    SeiPayloadArray  m_payloads;

protected:
    SetOfSlices * GetLayerBySlice(H264Slice * slice);
    SetOfSlices * AddLayer(H264Slice * slice);

    std::vector<SetOfSlices>  m_layers;

    bool m_isFullAU;

    uint32_t m_auCounter;
};

class AU_Splitter
{
public:
    AU_Splitter(H264_Heap_Objects *objectHeap);
    virtual ~AU_Splitter();

    void Init();
    void Close();

    void Reset();

    NalUnit * GetNalUnit(MediaData * src);

    NALUnitSplitter * GetNalUnitSplitter();

protected:

    Headers     m_Headers;
    H264_Heap_Objects   *m_objHeap;

protected:

    //Status DecodeHeaders(MediaDataEx *nalUnit);

    std::unique_ptr<NALUnitSplitter> m_pNALSplitter;
};

} // namespace UMC

#endif // __UMC_H264_AU_SPLITTER_H
#endif // MFX_ENABLE_H264_VIDEO_DECODE
