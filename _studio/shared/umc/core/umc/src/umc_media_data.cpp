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
#include "umc_media_data.h"
#include "umc_defs.h"

#include <algorithm>

namespace UMC
{

MediaData::MediaData(size_t length)
{
    m_pBufferPointer   = NULL;
    m_pDataPointer     = NULL;
    m_nBufferSize      = 0;
    m_nDataSize        = 0;
    m_pts_start        = -1;
    m_pts_end          = 0;
    m_frameType        = NONE_PICTURE;
    m_isInvalid        = 0;

    m_bMemoryAllocated = 0;
    m_flags = 0;

    if (length)
    {
        m_pBufferPointer = new uint8_t[length];
        m_pDataPointer     = m_pBufferPointer;
        m_nBufferSize      = length;
        m_bMemoryAllocated = 1;
    }
} // MediaData::MediaData(size_t length) :

MediaData::MediaData(const MediaData &another)
    : m_AuxInfo(another.m_AuxInfo)
{
    m_pBufferPointer   = NULL;
    m_pDataPointer     = NULL;
    m_nBufferSize      = 0;
    m_nDataSize        = 0;
    m_pts_start        = -1;
    m_pts_end          = 0;
    m_frameType        = NONE_PICTURE;
    m_isInvalid        = 0;

    m_bMemoryAllocated = 0;
    m_flags = 0;

    operator = (another);

} // MediaData::MediaData(const MediaData &another)

MediaData::~MediaData()
{
    Close();

} // MediaData::~MediaData()

Status MediaData::Alloc(size_t length)
{
    Close();

    if (length)
    {
        m_pBufferPointer = new uint8_t[length];
        m_pDataPointer     = m_pBufferPointer;
        m_nBufferSize      = length;
        m_bMemoryAllocated = 1;
    }

    return UMC_OK;
}

Status MediaData::Close(void)
{
    if (m_bMemoryAllocated)
    {
        if (m_pBufferPointer)
            delete[] m_pBufferPointer;
    }

    m_pBufferPointer   = NULL;
    m_pDataPointer     = NULL;
    m_nBufferSize      = 0;
    m_nDataSize        = 0;
    m_frameType        = NONE_PICTURE;

    m_bMemoryAllocated = 0;

    m_AuxInfo.clear();
    return UMC_OK;

} // Status MediaData::Close(void)

Status MediaData::SetDataSize(size_t bytes)
{
    if (!m_pBufferPointer)
        return UMC_ERR_NULL_PTR;

    if (bytes > (m_nBufferSize - (m_pDataPointer - m_pBufferPointer)))
        return UMC_ERR_FAILED;

    m_nDataSize = bytes;

    return UMC_OK;

} // Status MediaData::SetDataSize(size_t bytes)

// Set the pointer to a buffer allocated by the user
// bytes define the size of buffer
// size of data is equal to buffer size after this call
Status MediaData::SetBufferPointer(uint8_t *ptr, size_t size)
{
    // release object
    MediaData::Close();

    // set new value(s)
    m_pBufferPointer  = ptr;
    m_pDataPointer    = ptr;
    m_nBufferSize     = size;
    m_nDataSize       = 0;

    return UMC_OK;

} // Status MediaData::SetBufferPointer(uint8_t *ptr, size_t size)

void MediaData::SetAuxInfo(void* ptr, size_t size, int type)
{
     AuxInfo* aux = GetAuxInfo(type);
     if (!aux)
     {
         m_AuxInfo.push_back(AuxInfo());
         aux = &m_AuxInfo.back();
     }

     aux->ptr = ptr;
     aux->size = size;
     aux->type = type;
} // void MediaData::SetAuxInfo(void* ptr, size_t size, int type)

void MediaData::ClearAuxInfo(int type)
{
    AuxInfo aux = { 0, 0, type };
    m_AuxInfo.remove(aux);
} // void MediaData::ClearAuxInfo(int type)

MediaData::AuxInfo const* MediaData::GetAuxInfo(int type) const
{
    AuxInfo aux = { 0, 0, type };
    std::list<AuxInfo>::const_iterator
        i = std::find(m_AuxInfo.begin(), m_AuxInfo.end(), aux);

    return
        i != m_AuxInfo.end() ? &(*i) : 0;
} // MediaData::AuxInfo const* MediaData::GetAuxInfo(int type) const

Status MediaData::SetTime(double start, double end)
{
 //   if (start < 0  && start != -1.0)
 //       return UMC_ERR_FAILED;

    m_pts_start = start;
    m_pts_end = end;

    return UMC_OK;

} // Status MediaData::SetTime(double start, double end)

Status MediaData::GetTime(double& start, double& end) const
{
    start = m_pts_start;
    end = m_pts_end;

    return UMC_OK;

} // Status MediaData::GetTime(double& start, double& end)

Status MediaData::MoveDataPointer(int32_t bytes)
{
    if (bytes >= 0 && m_nDataSize >= (size_t)bytes) {
        m_pDataPointer   += bytes;
        m_nDataSize      -= bytes;

        return UMC_OK;
    } else if (bytes < 0 && (size_t)(m_pDataPointer - m_pBufferPointer) >= (size_t)(-bytes)) {
        m_pDataPointer   += bytes;
        m_nDataSize      -= bytes;

        return UMC_OK;
    }

    return UMC_ERR_FAILED;
} // Status MediaData::MovePointer(int32_t bytes)

Status MediaData::Reset()
{
    m_pDataPointer = m_pBufferPointer;
    m_nDataSize = 0;
    m_pts_start = -1.0;
    m_pts_end = -1.0;
    m_frameType = NONE_PICTURE;
    m_isInvalid = 0;
    return UMC_OK;
}

MediaData& MediaData::operator = (const MediaData& src)
{
    // check for assignment for self
    if (this == &src)
    {
        return *this;
    }

    MediaData::Close();

    m_pts_start        = src.m_pts_start;
    m_pts_end          = src.m_pts_end;
    m_nBufferSize      = src.m_nBufferSize;
    m_nDataSize        = src.m_nDataSize;
    m_frameType        = src.m_frameType;
    m_isInvalid        = src.m_isInvalid;

    m_pDataPointer     = src.m_pDataPointer;
    m_pBufferPointer   = src.m_pBufferPointer;
    m_bMemoryAllocated = 0;

    m_flags            = src.m_flags;

    return *this;
} // MediaData& MediaData::operator = (const MediaData& src)

Status MediaData::MoveDataTo(MediaData* dst)
{
    MediaData *src;
    uint8_t *pDataEnd;
    uint8_t *pBufferEnd;
    size_t size;

    // check error(s)
    if (NULL == m_pDataPointer)
    {
        return UMC_ERR_NOT_INITIALIZED;
    }
    if ((NULL == dst) ||
        (NULL == dst->m_pDataPointer))
    {
        return UMC_ERR_NULL_PTR;
    }

    // get parameters
    src = this;
    pDataEnd = dst->m_pDataPointer + dst->m_nDataSize;
    pBufferEnd = dst->m_pBufferPointer + dst->m_nBufferSize;
    size = std::min<size_t>(src->m_nDataSize, pBufferEnd - pDataEnd);

    if (size)
    {
        std::copy(src->m_pDataPointer, src->m_pDataPointer + size, pDataEnd);
    }

    dst->m_nDataSize += size;
    src->MoveDataPointer((int32_t)size);

    dst->m_pts_start = src->m_pts_start;
    dst->m_pts_end   = src->m_pts_end;
    dst->m_frameType = src->m_frameType;
    dst->m_isInvalid = src->m_isInvalid;

    return UMC_OK;

} // MediaData::MoveDataTo(MediaData& src)

} // namespace UMC
