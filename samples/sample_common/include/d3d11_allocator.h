/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#ifndef __D3D11_ALLOCATOR_H__
#define __D3D11_ALLOCATOR_H__

#include "base_allocator.h"
#include <limits>

#ifdef __gnu_linux__
#include <stdint.h> // for uintptr_t on Linux
#endif

//application can provide either generic mid from surface or this wrapper
//wrapper distinguishes from generic mid by highest 1 bit
//if it set then remained pointer points to extended structure of memid
//64 bits system layout
/*----+-----------------------------------------------------------+
|b63=1|63 bits remained for pointer to extended structure of memid|
|b63=0|63 bits from original mfxMemId                             |
+-----+----------------------------------------------------------*/
//32 bits system layout
/*--+---+--------------------------------------------+
|b31=1|31 bits remained for pointer to extended memid|
|b31=0|31 bits remained for surface pointer          |
+---+---+-------------------------------------------*/
//#pragma warning (disable:4293)
class MFXReadWriteMid
{
    static const uintptr_t bits_offset = std::numeric_limits<uintptr_t>::digits - 1;
    static const uintptr_t clear_mask = ~((uintptr_t)1 << bits_offset);
public:
    enum
    {
        //if flag not set it means that read and write
        not_set = 0,
        reuse   = 1,
        read    = 2,
        write   = 4,
    };
    //here mfxmemid might be as MFXReadWriteMid or mfxMemId memid
    MFXReadWriteMid(mfxMemId mid, mfxU8 flag = not_set)
    {
        //setup mid
        m_mid_to_report = (mfxMemId)((uintptr_t)&m_mid | ((uintptr_t)1 << bits_offset));
        if (0 != ((uintptr_t)mid >> bits_offset))
        {
            //it points to extended structure
            mfxMedIdEx * pMemIdExt = reinterpret_cast<mfxMedIdEx *>((uintptr_t)mid & clear_mask);
            m_mid.pId = pMemIdExt->pId;
            if (reuse == flag)
            {
                m_mid.read_write = pMemIdExt->read_write;
            }
            else
            {
                m_mid.read_write = flag;
            }
        }
        else
        {
            m_mid.pId = mid;
            if (reuse == flag)
                m_mid.read_write = not_set;
            else
                m_mid.read_write = flag;
        }

    }
    bool isRead() const
    {
        return 0 != (m_mid.read_write & read) || !m_mid.read_write;
    }
    bool isWrite() const
    {
        return 0 != (m_mid.read_write & write) || !m_mid.read_write;
    }
    /// returns original memid without read write flags
    mfxMemId raw() const
    {
        return m_mid.pId;
    }
    operator mfxMemId() const
    {
        return m_mid_to_report;
    }

private:
    struct mfxMedIdEx
    {
        mfxMemId pId;
        mfxU8 read_write;
    };

    mfxMedIdEx m_mid;
    mfxMemId   m_mid_to_report;
};

#endif // __D3D11_ALLOCATOR_H__
