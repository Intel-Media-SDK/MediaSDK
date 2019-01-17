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

#include "umc_defs.h"
#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_mpeg2_defs.h"
#include "umc_mpeg2_slice.h"

namespace UMC_MPEG2_DECODER
{
    // Decode slice header and initialize slice structure with parsed values
    bool MPEG2Slice::Reset()
    {
        m_bitStream.Reset((uint8_t *) source.GetDataPointer(), (uint32_t) source.GetDataSize());

        return DecodeSliceHeader();
    }

    // Decoder slice header
    bool MPEG2Slice::DecodeSliceHeader()
    {
        UMC::Status umcRes = UMC::UMC_OK;
        try
        {
            umcRes = m_bitStream.GetSliceHeaderFull(*this, *m_seqHdr.get(), *m_seqExtHdr.get());
        }
        catch(...)
        {
            return false;
        }

        return (UMC::UMC_OK == umcRes);
    }
}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
