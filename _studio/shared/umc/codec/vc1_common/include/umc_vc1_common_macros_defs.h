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

#if defined (MFX_ENABLE_VC1_VIDEO_DECODE)

#ifndef __UMC_VC1_COMMON_MACROS_DEFS_H__
#define __UMC_VC1_COMMON_MACROS_DEFS_H__

#include <stdio.h>

#define VC1_SIGN(x) ((x<0)?-1:(x>0?1:0))
#define VC1_LUT_SET(value,lut) (lut[value])

namespace UMC
{
    class VC1BitstreamParser
    {
    public:
        template <class T>
        inline static void GetNBits(uint32_t* &pCurrentData, int32_t& offset, uint32_t nbits, T &data)
        {
            uint32_t x;
            offset -= (nbits);

            if(offset >= 0)
            {
                x = pCurrentData[0] >> (offset + 1);
            }
            else
            {
                offset += 32;

                x = pCurrentData[1] >> (offset);
                x >>= 1;
                x += pCurrentData[0] << (31 - offset);
                pCurrentData++;
            }
            data = T(x & ((0x00000001 << (nbits&0x1F)) - 1));
        }
        template <class T>
        inline static void CheckNBits(uint32_t* pCurrentData, uint32_t offset, uint32_t nbits, T &data)
        {
            int32_t bp;
            uint32_t x;


            bp = offset - (nbits);

            if(bp < 0)
            {
                bp += 32;
                x = pCurrentData[1] >> bp;
                x >>= 1;
                x += pCurrentData[0] << (31 - bp);
            }
            else
            {
                x = pCurrentData[0] >> bp;
                x >>= 1;
            }

            data = T(x & ((0x00000001 << (nbits&0x1F)) - 1));
        }

    };

}
// from histrorical reasons. Unsafety work with bitstream
#define VC1NextNBits( current_data, offset, nbits, data) __VC1NextBits(current_data, offset, nbits, data)
#define __VC1GetBits(current_data, offset, nbits, data)                 \
{                                                                       \
    uint32_t _x;                                                           \
                                                                        \
    VM_ASSERT((nbits) >= 0 && (nbits) <= 32);                           \
    VM_ASSERT(offset >= 0 && offset <= 31);                             \
                                                                        \
    offset -= (nbits);                                                  \
                                                                        \
    if(offset >= 0)                                                     \
    {                                                                   \
        _x = current_data[0] >> (offset + 1);                            \
    }                                                                   \
    else                                                                \
    {                                                                   \
        offset += 32;                                                   \
                                                                        \
        _x = current_data[1] >> (offset);                                \
        _x >>= 1;                                                        \
        _x += current_data[0] << (31 - offset);                          \
        current_data++;                                                 \
    }                                                                   \
                                                                        \
    VM_ASSERT(offset >= 0 && offset <= 31);                             \
                                                                        \
    (data) = _x & ((0x00000001 << (nbits&0x1F)) - 1);                    \
}

#define VC1CheckDataLen(currLen,nbits)                                  \
{                                                                       \
    currLen-=nbits;                                                     \
    if(currLen< 0) return MFX_ERR_NOT_INITIALIZED;                      \
}                                                                       \

#define VC1GetNBits( current_data, offset, nbits, data) __VC1GetBits(current_data, offset, nbits, data)
#define __VC1NextBits(current_data, offset, nbits, data)                  \
{                                                                       \
    int32_t bp;                                                          \
    uint32_t x;                                                           \
                                                                        \
    VM_ASSERT((nbits) >= 0 && (nbits) <= 32);                           \
    VM_ASSERT(offset >= 0 && offset <= 31);                             \
                                                                        \
    bp = offset - (nbits);                                              \
                                                                        \
    if(bp < 0)                                                          \
    {                                                                   \
        bp += 32;                                                       \
        x = current_data[1] >> bp;                                      \
        x >>= 1;                                                        \
        x += current_data[0] << (31 - bp);                              \
    }                                                                   \
    else                                                                \
    {                                                                   \
        x = current_data[0] >> bp;                                      \
        x >>= 1;                                                        \
    }                                                                   \
                                                                        \
    (data) = x & ((0x00000001 << (nbits&0x1F)) - 1);                    \
}

#endif //__VC1_DEC_MACROS_DEFS_H__
#endif //MFX_ENABLE_VC1_VIDEO_DECODE
