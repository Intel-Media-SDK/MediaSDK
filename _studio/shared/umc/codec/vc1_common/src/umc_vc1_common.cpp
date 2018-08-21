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

#if defined (MFX_ENABLE_VC1_VIDEO_DECODE)
#include "umc_vc1_common.h"

namespace UMC
{
    namespace VC1Common
    {

        void SwapData(uint8_t *src, uint32_t dataSize)
        {
            uint32_t i;
            uint32_t counter = 0;
            uint32_t* pDst = (uint32_t*)src;
            uint32_t  iCur = 0;

            for(i = 0; i < dataSize+4; i++)
            {
                if (4 == counter)
                {
                    counter = 0;
                    *pDst = iCur;
                    pDst++;
                    iCur = 0;
                }

                if (0 == counter)
                    iCur = src[i];
                iCur <<= 8;
                iCur |= src[i];
                ++counter;
            }
        }



    }
}
#endif //MFX_ENABLE_VC1_VIDEO_DECODE
