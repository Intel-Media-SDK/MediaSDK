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

#ifndef __UMC_H265_HEADERS_H
#define __UMC_H265_HEADERS_H

#include "umc_h265_dec_defs.h"
#include "umc_media_data_ex.h"
#include "umc_h265_heap.h"

namespace UMC_HEVC_DECODER
{

// Headers container class
template <typename T>
class HeaderSet
{
public:

    HeaderSet(Heap_Objects  *pObjHeap)
        : m_Header()
        , m_pObjHeap(pObjHeap)
        , m_currentID(-1)
    {
    }

    virtual ~HeaderSet()
    {
        Reset(false);
    }

    T * AddHeader(T* hdr)
    {
        uint32_t id = hdr->GetID();

        if (id >= m_Header.size())
        {
            m_Header.resize(id + 1);
        }

        m_currentID = id;

        if (m_Header[id])
        {
            m_Header[id]->DecrementReference();
        }

        T * header = m_pObjHeap->AllocateObject<T>();
        *header = *hdr;

        //ref. counter may not be 0 here since it can be copied from given [hdr] object
        header->ResetRefCounter();
        header->IncrementReference();

        m_Header[id] = header;
        return header;
    }

    T * GetHeader(int32_t id)
    {
        if ((uint32_t)id >= m_Header.size())
        {
            return 0;
        }

        return m_Header[id];
    }

    const T * GetHeader(int32_t id) const
    {
        if ((uint32_t)id >= m_Header.size())
        {
            return 0;
        }

        return m_Header[id];
    }

    void RemoveHeader(void * hdr)
    {
        T * tmp = (T *)hdr;
        if (!tmp)
        {
            VM_ASSERT(false);
            return;
        }

        uint32_t id = tmp->GetID();

        if (id >= m_Header.size())
        {
            VM_ASSERT(false);
            return;
        }

        if (!m_Header[id])
        {
            VM_ASSERT(false);
            return;
        }

        VM_ASSERT(m_Header[id] == hdr);
        m_Header[id]->DecrementReference();
        m_Header[id] = 0;
    }

    void Reset(bool isPartialReset = false)
    {
        if (!isPartialReset)
        {
            for (uint32_t i = 0; i < m_Header.size(); i++)
            {
                m_pObjHeap->FreeObject(m_Header[i]);
            }

            m_Header.clear();
            m_currentID = -1;
        }
    }

    void SetCurrentID(int32_t id)
    {
        if (GetHeader(id))
            m_currentID = id;
    }

    int32_t GetCurrentID() const
    {
        return m_currentID;
    }

    T * GetCurrentHeader()
    {
        if (m_currentID == -1)
            return 0;

        return GetHeader(m_currentID);
    }

    const T * GetCurrentHeader() const
    {
        if (m_currentID == -1)
            return 0;

        return GetHeader(m_currentID);
    }

private:
    std::vector<T*>           m_Header;
    Heap_Objects             *m_pObjHeap;

    int32_t                    m_currentID;
};

/****************************************************************************************************/
// Headers stuff
/****************************************************************************************************/
class Headers
{
public:

    Headers(Heap_Objects  *pObjHeap)
        : m_VideoParams(pObjHeap)
        , m_SeqParams(pObjHeap)
        , m_PicParams(pObjHeap)
        , m_SEIParams(pObjHeap)
    {
    }

    void Reset(bool isPartialReset = false)
    {
        m_SeqParams.Reset(isPartialReset);
        m_PicParams.Reset(isPartialReset);
        m_SEIParams.Reset(isPartialReset);
        m_VideoParams.Reset(isPartialReset);
    }

    HeaderSet<H265VideoParamSet>           m_VideoParams;
    HeaderSet<H265SeqParamSet>             m_SeqParams;
    HeaderSet<H265PicParamSet>             m_PicParams;
    HeaderSet<H265SEIPayLoad>              m_SEIParams;
};

} // namespace UMC_HEVC_DECODER

#endif // __UMC_H265_HEADERS_H
#endif // MFX_ENABLE_H265_VIDEO_DECODE
