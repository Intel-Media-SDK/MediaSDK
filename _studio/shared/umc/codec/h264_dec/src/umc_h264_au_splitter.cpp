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

#include <memory>

#include "umc_h264_au_splitter.h"
#include "umc_h264_nal_spl.h"

namespace UMC
{

/****************************************************************************************************/
// SeiPayloadArray class routine
/****************************************************************************************************/
SeiPayloadArray::SeiPayloadArray()
{
    m_payloads.reserve(3);
}

SeiPayloadArray::SeiPayloadArray(const SeiPayloadArray & payloads)
:m_payloads()
{
    size_t count = payloads.GetPayloadCount();
    for (size_t i = 0; i < count; i++)
    {
        AddPayload(payloads.GetPayload(i));
    }
}

SeiPayloadArray::~SeiPayloadArray()
{
    Release();
}

size_t SeiPayloadArray::GetPayloadCount() const
{
    return m_payloads.size();
}

UMC_H264_DECODER::H264SEIPayLoad* SeiPayloadArray::GetPayload(size_t pos) const
{
    if (pos >= m_payloads.size())
        return 0;

    return m_payloads[pos];
}

UMC_H264_DECODER::H264SEIPayLoad* SeiPayloadArray::FindPayload(SEI_TYPE type) const
{
    int32_t pos = FindPayloadPos(type);
    return (pos < 0) ? 0 : GetPayload(pos);
}

int32_t SeiPayloadArray::FindPayloadPos(SEI_TYPE type) const
{
    size_t count = m_payloads.size();
    for (size_t i = 0; i < count; i++)
    {
        UMC_H264_DECODER::H264SEIPayLoad* payload = m_payloads[i];
        if (payload->payLoadType == type)
            return (int32_t)i;
    }

    return -1;
}

void SeiPayloadArray::MovePayloadsFrom(SeiPayloadArray &payloads)
{
    size_t count = payloads.GetPayloadCount();
    for (size_t i = 0; i < count; i++)
    {
        AddPayload(payloads.GetPayload(i));
    }

    payloads.Release();
}

void SeiPayloadArray::Release()
{
    PayloadArray::iterator iter = m_payloads.begin();
    PayloadArray::iterator iter_end = m_payloads.end();
    for (; iter != iter_end; ++iter)
    {
        UMC_H264_DECODER::H264SEIPayLoad* payload = *iter;
        payload->DecrementReference();
    }

    m_payloads.clear();
}

void SeiPayloadArray::AddPayload(UMC_H264_DECODER::H264SEIPayLoad* payload)
{
    if (!payload)
        return;

    payload->IncrementReference();
    int32_t pos = FindPayloadPos(payload->payLoadType);
    if (pos >= 0) // always use last payload
    {
        m_payloads[pos]->DecrementReference();
        m_payloads[pos] = payload;
        return;
    }

    m_payloads.push_back(payload);
}

/****************************************************************************************************/
// SetOfSlices class routine
/****************************************************************************************************/
SetOfSlices::SetOfSlices()
    : m_frame(0)
    , m_isCompleted(false)
    , m_isFull(false)
{
}

SetOfSlices::SetOfSlices(const SetOfSlices& set)
    : m_frame(set.m_frame)
    , m_isCompleted(set.m_isCompleted)
    , m_isFull(set.m_isFull)
    , m_payloads(set.m_payloads)
    , m_pSliceQueue(set.m_pSliceQueue)
{
    for (auto pSlice : m_pSliceQueue)
    {
        pSlice->IncrementReference();
    }
}

SetOfSlices::~SetOfSlices()
{
    Release();
}

SetOfSlices& SetOfSlices::operator=(const SetOfSlices& set)
{
    if (this == &set)
    {
        return *this;
    }

    *this = set;
    for (auto pSlice : m_pSliceQueue)
    {
        pSlice->IncrementReference();
    }

    return *this;
}

H264Slice * SetOfSlices::GetSlice(size_t pos) const
{
    if (pos >= m_pSliceQueue.size())
        return nullptr;

    return m_pSliceQueue[pos];
}

size_t SetOfSlices::GetSliceCount() const
{
    return m_pSliceQueue.size();
}

void SetOfSlices::AddSlice(H264Slice * slice)
{
    m_pSliceQueue.push_back(slice);
}

void SetOfSlices::Release()
{
    size_t count = m_pSliceQueue.size();
    for (size_t sliceId = 0; sliceId < count; sliceId++)
    {
        H264Slice * slice = m_pSliceQueue[sliceId];
        slice->DecrementReference();
    }

    Reset();
}

void SetOfSlices::Reset()
{
    m_frame = 0;
    m_isCompleted = false;
    m_isFull = false;
    m_pSliceQueue.clear();
    m_payloads.Release();
}

void SetOfSlices::AddSet(const SetOfSlices *set)
{
    size_t count = set->GetSliceCount();
    for (size_t sliceId = 0; sliceId < count; sliceId++)
    {
        AddSlice(set->GetSlice(sliceId));
    }
}

void SetOfSlices::CleanUseless()
{
    size_t count = m_pSliceQueue.size();
    for (size_t sliceId = 0; sliceId < count; sliceId++)
    {
        H264Slice * curSlice = m_pSliceQueue[sliceId];
        if (curSlice->m_bDecoded)
        {
            m_pSliceQueue.erase(m_pSliceQueue.begin() + sliceId); // remove
            count = m_pSliceQueue.size();
            --sliceId;
            curSlice->Release();
            curSlice->DecrementReference();
        }
    }
}

void SetOfSlices::SortSlices()
{
    static int32_t MAX_MB_NUMBER = 0x7fffffff;

    if (!m_pSliceQueue.empty() && m_pSliceQueue[0]->IsSliceGroups())
        return;

    size_t count = m_pSliceQueue.size();
    for (size_t sliceId = 0; sliceId < count; sliceId++)
    {
        H264Slice * curSlice = m_pSliceQueue[sliceId];
        int32_t minFirst = MAX_MB_NUMBER;
        size_t minSlice = 0;

        for (size_t j = sliceId; j < count; j++)
        {
            H264Slice * slice = m_pSliceQueue[j];
            if (slice->GetStreamFirstMB() < curSlice->GetStreamFirstMB() && minFirst > slice->GetStreamFirstMB() &&
                curSlice->GetSliceHeader()->nal_ext.svc.dependency_id == slice->GetSliceHeader()->nal_ext.svc.dependency_id &&
                curSlice->GetSliceHeader()->nal_ext.svc.quality_id == slice->GetSliceHeader()->nal_ext.svc.quality_id)
            {
                minFirst = slice->GetStreamFirstMB();
                minSlice = j;
            }
        }

        if (minFirst != MAX_MB_NUMBER)
        {
            H264Slice * temp = m_pSliceQueue[sliceId];
            m_pSliceQueue[sliceId] = m_pSliceQueue[minSlice];
            m_pSliceQueue[minSlice] = temp;
        }
    }

    for (size_t sliceId = 0; sliceId < count - 1; sliceId++)
    {
        H264Slice * slice     = m_pSliceQueue[sliceId];
        H264Slice * nextSlice = m_pSliceQueue[sliceId + 1];

        if (nextSlice->IsSliceGroups() || slice->IsSliceGroups())
            continue;

        if (nextSlice->GetSliceHeader()->nal_ext.svc.quality_id == slice->GetSliceHeader()->nal_ext.svc.quality_id)
            slice->SetMaxMB(nextSlice->GetStreamFirstMB());

        if (slice->GetStreamFirstMB() == slice->GetMaxMB())
        {
            count--;
            for (size_t i = sliceId; i < count; i++)
            {
                m_pSliceQueue[i] = m_pSliceQueue[i + 1];
            }

            m_pSliceQueue.resize(count);
            slice->DecrementReference();

            sliceId = uint32_t(-1);
            continue;
        }
    }
}

/****************************************************************************************************/
// AccessUnit class routine
/****************************************************************************************************/
AccessUnit::AccessUnit()
    : m_isInitialized(false)
    , m_isFullAU(false)
    , m_auCounter(0)
{
}

AccessUnit::~AccessUnit()
{
}

uint32_t AccessUnit::GetAUIndentifier() const
{
    return m_auCounter;
}

size_t AccessUnit::GetLayersCount() const
{
    return m_layers.size();
}

void AccessUnit::CleanUseless()
{
    size_t count = m_layers.size();
    for (size_t pos = 0; pos < count; pos++)
    {
        SetOfSlices * set = &m_layers[pos];
        set->CleanUseless();
        if (!set->GetSliceCount())
        {
            m_layers.erase(m_layers.begin() + pos);
            count = m_layers.size();
            pos--;
        }
    }
}

int32_t AccessUnit::FindLayerByDependency(int32_t dependency)
{
    size_t count = m_layers.size();
    for (size_t i = 0; i < count; i++)
    {
        SetOfSlices * set = &m_layers[i];
        if (set->GetSlice(0) && set->GetSlice(0)->GetSliceHeader()->nal_ext.svc.dependency_id == dependency)
            return (int32_t)i;
    }

    return -1;
}

SetOfSlices * AccessUnit::GetLastLayer()
{
    if (m_layers.empty())
        return nullptr;

    return &m_layers.back();
}

SetOfSlices * AccessUnit::GetLayer(size_t pos)
{
    if (pos >= m_layers.size())
        return nullptr;

    return &m_layers[pos];
}

void AccessUnit::CombineSets()
{
    if (m_layers.empty())
        return;

    SetOfSlices * setOfSlices = &m_layers[0];
    size_t count = m_layers.size();
    for (size_t i = 1; i < count; i++)
    {
        SetOfSlices * set = &m_layers[i];
        setOfSlices->AddSet(set);
        set->Reset();
    }

    m_layers.resize(1);
}

bool AccessUnit::IsFullAU() const
{
    return m_isFullAU;
}

void AccessUnit::CompleteLastLayer()
{
    if (!m_layers.empty())
        m_layers.back().m_isFull = true;
}

bool AccessUnit::AddSlice(H264Slice * slice)
{
    if (!slice)
    {
        if (!m_layers.empty())
        {
            m_isFullAU = true;
        }

        return false;
    }

    SetOfSlices * setOfSlices = GetLayerBySlice(slice);
    if (!setOfSlices)
    {
        if (!m_layers.empty())
        {
            SetOfSlices * lastSetOfSlices = GetLastLayer();
            if (lastSetOfSlices->GetSlice(0)->GetSliceHeader()->nal_ext.svc.dependency_id > slice->GetSliceHeader()->nal_ext.svc.dependency_id)
            {
                m_isFullAU = true;
                return false;
            }
        }

        setOfSlices = AddLayer(slice);
        setOfSlices->m_payloads.MovePayloadsFrom(m_payloads);
    }

    H264Slice * lastSlice = setOfSlices->GetSlice(setOfSlices->GetSliceCount() - 1);
    if (!IsPictureTheSame(lastSlice, slice) || setOfSlices->m_isFull)
    {
        m_isFullAU = true;
        return false;
    }

    m_payloads.Release();

    setOfSlices->AddSlice(slice);
    return true;
}

void AccessUnit::Release()
{
    for (auto & set: m_layers)
    {
       set.Release();
    }

    Reset();
    m_payloads.Release();
}

void AccessUnit::Reset()
{
    for (auto & set: m_layers)
    {
       set.Reset();
    }

    m_layers.clear();
    m_auCounter++;
    m_isFullAU = false;
    m_isInitialized = false;
}

void AccessUnit::SortforASO()
{
    for (auto & set: m_layers)
    {
       set.SortSlices();
    }
}

SetOfSlices * AccessUnit::AddLayer(H264Slice * )
{
    SetOfSlices setofSlices;
    m_layers.push_back(setofSlices);
    return &m_layers.back();
}

bool AccessUnit::IsItSliceOfThisAU(H264Slice * slice)
{
    SetOfSlices * setOfSlices = GetLayerBySlice(slice);
    if (!setOfSlices)
        return false;

    H264Slice * lastSlice = setOfSlices->GetSlice(0);
    return IsPictureTheSame(lastSlice, slice);
}

SetOfSlices * AccessUnit::GetLayerBySlice(H264Slice * slice)
{
    if (!slice)
        return nullptr;

    size_t count = m_layers.size();
    for (size_t i = 0; i < count; i++)
    {
        H264Slice * temp = m_layers[i].GetSlice(0);
        if (!temp)
            continue;

        if (temp->GetSliceHeader()->nal_ext.mvc.view_id == slice->GetSliceHeader()->nal_ext.mvc.view_id &&
            temp->GetSliceHeader()->nal_ext.svc.dependency_id == slice->GetSliceHeader()->nal_ext.svc.dependency_id)
            return &m_layers[i];
    }

    return nullptr;
}

AU_Splitter::AU_Splitter(H264_Heap_Objects *objectHeap)
    : m_Headers(objectHeap)
    , m_objHeap(objectHeap)
{
}

AU_Splitter::~AU_Splitter()
{
    Close();
}

void AU_Splitter::Init()
{
    Close();

    m_pNALSplitter.reset(new NALUnitSplitter());
    m_pNALSplitter->Init();
}

void AU_Splitter::Close()
{
    m_pNALSplitter.reset(0);
}

void AU_Splitter::Reset()
{
    if (m_pNALSplitter.get())
        m_pNALSplitter->Reset();

    m_Headers.Reset();
}

NalUnit * AU_Splitter::GetNalUnit(MediaData * src)
{
    return m_pNALSplitter->GetNalUnits(src);
}

NALUnitSplitter * AU_Splitter::GetNalUnitSplitter()
{
    return m_pNALSplitter.get();
}

} // namespace UMC
#endif // MFX_ENABLE_H264_VIDEO_DECODE
