// Copyright (c) 2018 Intel Corporation
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

#pragma once

#include "ts_common.h"
#include <vector>
#include <algorithm>

#if MFX_VERSION >= MFX_VERSION_NEXT && defined(LIBVA_SUPPORT)
#include <va/va.h>
#include "mfxfeihevc.h"
#endif

template<class T> struct tsExtBufTypeToId { enum { id = 0 }; };

#define EXTBUF(TYPE, ID) template<> struct tsExtBufTypeToId<TYPE> { enum { id = ID }; };
#include "ts_ext_buffers_decl.h"
#undef EXTBUF

struct tsCmpExtBufById
{
    mfxU32 m_id;

    tsCmpExtBufById(mfxU32 id)
        : m_id(id)
    {
    };

    bool operator () (mfxExtBuffer* b)
    {
        return  (b && b->BufferId == m_id);
    };
};

template <typename TB> class tsExtBufType : public TB
{
private:
    typedef std::vector<mfxExtBuffer*> EBStorage;
    typedef EBStorage::iterator        EBIterator;

    EBStorage m_buf;

public:
    tsExtBufType()
    {
        TB& base = *this;
        memset(&base, 0, sizeof(TB));
    }

    tsExtBufType(const TB& that)
    {
        TB& base = *this;
        base = that;
        ReserveBuffers(that.NumExtParam);
        RefreshBuffers();
        for (mfxU16 i = 0; i < that.NumExtParam; i++)
        {
            auto b = that.ExtParam[i];
            if (!b || !b->BufferSz) continue;
            AddExtBuffer(b->BufferId, b->BufferSz);
            memcpy(GetExtBuffer(b->BufferId), b, b->BufferSz);
        }
    }

    tsExtBufType(const tsExtBufType& that)
    {
        TB& base = *this;
        memset(&base, 0, sizeof(TB));
        *this = (const TB&)that;
    }

    ~tsExtBufType()
    {
        for(EBIterator it = m_buf.begin(); it != m_buf.end(); it++ )
        {
            delete [] (mfxU8*)(*it);
        }
    }

    void ReserveBuffers(mfxU32 n_buffers)
    {
        m_buf.reserve(n_buffers);
    }

    void RefreshBuffers()
    {
        this->NumExtParam = (mfxU32)m_buf.size();
        this->ExtParam    = this->NumExtParam ? m_buf.data() : 0;
    }

    mfxExtBuffer* GetExtBuffer(mfxU32 id)
    {
        EBIterator it = std::find_if(m_buf.begin(), m_buf.end(), tsCmpExtBufById(id));
        if(it != m_buf.end())
        {
            return *it;
        }
        return 0;
    }

    void AddExtBuffer(mfxU32 id, mfxU32 size, mfxU32 nbBytesExtraSize = 0)
    {
        if(!size)
        {
             m_buf.push_back(0);
        } else
        {
            EBIterator it = std::find_if(m_buf.begin(), m_buf.end(), tsCmpExtBufById(id));
            if(it == m_buf.end())
            {
                m_buf.push_back( (mfxExtBuffer*)new mfxU8[size+ nbBytesExtraSize] );
                mfxExtBuffer& eb = *m_buf.back();

                memset(&eb, 0, size);
                eb.BufferId = id;
                eb.BufferSz = size;
#if MFX_VERSION >= MFX_VERSION_NEXT && defined(LIBVA_SUPPORT)
                // HEVC FEI buffers uses direct exposure. 0 is correct libva buffer id, so id should be explicitly set to invalid
                switch (eb.BufferId)
                {
                case MFX_EXTBUFF_HEVCFEI_ENC_QP:
                    (reinterpret_cast<mfxExtFeiHevcEncQP*>(&eb))->VaBufferID = VA_INVALID_ID;
                    break;
                case MFX_EXTBUFF_HEVCFEI_ENC_CTU_CTRL:
                    (reinterpret_cast<mfxExtFeiHevcEncCtuCtrl*>(&eb))->VaBufferID = VA_INVALID_ID;
                    break;
                // TODO: Add more HEVC FEI buffers
                default:
                    break;
                }
#endif
            }
        }

        RefreshBuffers();
    }

    template <typename T> void AddExtBuffer(mfxU32 nbBytesExtraSize = 0)
    {
        return AddExtBuffer(tsExtBufTypeToId<T>::id, sizeof(T), nbBytesExtraSize);
    }

    void RemoveExtBuffer(mfxU32 id)
    {
        EBIterator it = std::find_if(m_buf.begin(), m_buf.end(), tsCmpExtBufById(id));
        if(it != m_buf.end())
        {
            delete [] (mfxU8*)(*it);
            m_buf.erase(it);
            RefreshBuffers();
        }
    }

    template <typename T> void RemoveExtBuffer()
    {
        return RemoveExtBuffer(tsExtBufTypeToId<T>::id);
    }

    template <typename T> operator T&()
    {
        mfxU32 id = tsExtBufTypeToId<T>::id;
        mfxExtBuffer * p = GetExtBuffer(id);

        if(!p)
        {
            AddExtBuffer(id, sizeof(T));
            p = GetExtBuffer(id);
        }

        return *(T*)p;
    }

    template <typename T> operator T*()
    {
        return (T*)GetExtBuffer(tsExtBufTypeToId<T>::id);
    }

    tsExtBufType<TB>& operator=(const tsExtBufType<TB>& other) // copy assignment
    {
        //remove all existing extended buffers
        if(0 != m_buf.size())
        {
            for(EBIterator it = m_buf.begin(); it != m_buf.end(); it++ )
            {
                delete [] (mfxU8*)(*it);
            }
            m_buf.clear();
            RefreshBuffers();
        }
        //copy content of main buffer
        TB& base = *this;
        const TB& other_base = other;
        memcpy(&base, &other_base, sizeof(TB));
        this->NumExtParam = 0;
        this->ExtParam = 0;

        //reproduce list of extended buffers and copy its content
        for(size_t i(0); i < other.NumExtParam; ++i)
        {
            const mfxExtBuffer* those_buffer = other.ExtParam[i];
            if(those_buffer)
            {
                AddExtBuffer(those_buffer->BufferId, those_buffer->BufferSz);
                mfxExtBuffer* this_buffer = GetExtBuffer(those_buffer->BufferId);

                memcpy((void*) this_buffer, (void*) those_buffer, those_buffer->BufferSz);
            }
            else
            {
                AddExtBuffer(0,0);
            }
        }

        return *this;
    }
};

template <typename TB>
inline bool operator==(const tsExtBufType<TB>& lhs, const tsExtBufType<TB>& rhs)
{
    const TB& this_base = lhs;
    const TB& that_base = rhs;
    TB tmp_this = this_base;
    TB tmp_that = that_base;

    tmp_this.ExtParam = 0;
    tmp_that.ExtParam = 0;

    if( memcmp(&tmp_this, &tmp_that, sizeof(TB)) )
        return false; //base structure differes

    //order of ext buffers should not matter
    for(size_t i(0); i < rhs.NumExtParam; ++i)
    {
        const mfxExtBuffer* those_buffer = rhs.ExtParam[i];
        if(those_buffer)
        {
            const mfxExtBuffer* this_buffer = const_cast<tsExtBufType<TB>& >(lhs).GetExtBuffer(those_buffer->BufferId);
            if( memcmp(this_buffer, those_buffer, (std::min)(those_buffer->BufferSz,this_buffer->BufferSz)) )
                return false; //ext buffer structure differes
        }
        else
        {
            //check for nullptr presence
            for(size_t j(0); j < this_base.NumExtParam; ++j)
            {
                if(0 == this_base.ExtParam[j])
                    break;
            }
            //one has nullptr buffer while another not
            return false;
        }
    }

    return true;
}

class GetExtBufferPtr
{
private:
    mfxExtBuffer ** m_b;
    mfxU16          m_n;
public:
    GetExtBufferPtr(mfxExtBuffer ** b, mfxU16 n)
        : m_b(b)
        , m_n(n)
    {
    }
    template <class T> GetExtBufferPtr(T& storage)
        : GetExtBufferPtr(storage.ExtParam, storage.NumExtParam)
    {}

    template <class T> operator T*()
    {
        if (m_b)
            for (mfxU16 i = 0; i < m_n; i++)
                if (m_b[i] && m_b[i]->BufferId == tsExtBufTypeToId<T>::id)
                    return (T*)m_b[i];
        return 0;
    }
};

template <typename TB>
inline bool operator!=(const tsExtBufType<TB>& lhs, const tsExtBufType<TB>& rhs){return !(lhs == rhs);}
