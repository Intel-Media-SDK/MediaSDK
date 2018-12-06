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

#include "umc_defs.h"

#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_media_data.h"
#include "umc_mpeg2_bitstream.h"

namespace UMC_MPEG2_DECODER
{
    // Slice descriptor class
    class MPEG2Slice
    {
    public:

        ~MPEG2Slice(void)
        { }

        // Decode slice header and initialize slice structure with parsed values
        bool Reset();

    public:
        // Decode slice header
        bool DecodeSliceHeader();

        const MPEG2SequenceHeader & GetSeqHeader(void) const
        { return *m_seqHdr.get(); }

        void SetSeqHeader(std::shared_ptr<MPEG2SequenceHeader> & seqHdr)
        { m_seqHdr = seqHdr; }

        const MPEG2SequenceExtension & GetSeqExtHeader(void) const
        { return *m_seqExtHdr.get(); }

        void SetSeqExtHeader(std::shared_ptr<MPEG2SequenceExtension> & seqExtHdr)
        { m_seqExtHdr = seqExtHdr; }

        const MPEG2PictureHeader & GetPicHeader(void) const
        { return *m_picHdr.get(); }

        void SetPicHeader(std::shared_ptr<MPEG2PictureHeader> & picHdr)
        { m_picHdr = picHdr; }

        const MPEG2PictureCodingExtension & GetPicExtHeader(void) const
        { return *m_picExtHdr.get(); }

        void SetPicExtHeader(std::shared_ptr<MPEG2PictureCodingExtension> & picExtHdr)
        { m_picExtHdr = picExtHdr; }

        const MPEG2QuantMatrix *GetQMatrix(void) const
        { return m_qm.get(); }

        void SetQMatrix(std::shared_ptr<MPEG2QuantMatrix> & qm)
        { m_qm = qm; }

        // Slice header
        const MPEG2SliceHeader & GetSliceHeader() const
        { return sliceHeader; }

        MPEG2SliceHeader & GetSliceHeader()
        { return sliceHeader; }

        // Obtain bitstream object
        const MPEG2HeadersBitstream & GetBitStream()  const
        { return m_bitStream; }

    public:
        UMC::MediaData       source;      // slice bitstream data
        MPEG2SliceHeader     sliceHeader; // parsed slice header

    protected:
        std::shared_ptr<MPEG2SequenceHeader>         m_seqHdr;
        std::shared_ptr<MPEG2SequenceExtension>      m_seqExtHdr;
        std::shared_ptr<MPEG2PictureHeader>          m_picHdr;
        std::shared_ptr<MPEG2PictureCodingExtension> m_picExtHdr;
        std::shared_ptr<MPEG2QuantMatrix>            m_qm; // optional

        MPEG2HeadersBitstream                        m_bitStream;
    };
}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
