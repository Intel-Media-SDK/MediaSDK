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

#include "mfx_common.h"

#if defined(MFX_ENABLE_VP9_VIDEO_DECODE_HW)

#include <stdexcept>
#include <string>

#include "mfx_vp9_dec_decode_utils.h"

#include "umc_vp9_bitstream.h"
#include "umc_vp9_frame.h"

namespace MfxVP9Decode
{
    void ParseSuperFrameIndex(const mfxU8 *data, size_t data_sz,
                              mfxU32 sizes[8], mfxU32 *count)
    {
        assert(data_sz);
        mfxU8 marker = ReadMarker(data + data_sz - 1);
        *count = 0;

        if ((marker & 0xe0) == 0xc0)
        {
            const mfxU32 frames = (marker & 0x7) + 1;
            const mfxU32 mag = ((marker >> 3) & 0x3) + 1;
            const size_t index_sz = 2 + mag * frames;

            mfxU8 marker2 = ReadMarker(data + data_sz - index_sz);

            if (data_sz >= index_sz && marker2 == marker)
            {
                // found a valid superframe index
                const mfxU8 *x = &data[data_sz - index_sz + 1];

                for (mfxU32 i = 0; i < frames; i++)
                {
                    mfxU32 this_sz = 0;

                    for (mfxU32 j = 0; j < mag; j++)
                        this_sz |= (*x++) << (j * 8);
                    sizes[i] = this_sz;
                }

                *count = frames;
            }
        }
    }

}; // namespace MfxVP9Decode

#endif // _MFX_VP9_DECODE_UTILS_H_
