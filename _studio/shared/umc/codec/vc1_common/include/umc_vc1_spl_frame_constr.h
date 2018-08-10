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

#ifndef __UMC_VC1_SPL_FRAME_CONSTR_H__
#define __UMC_VC1_SPL_FRAME_CONSTR_H__

#include "umc_media_data_ex.h"
#include "umc_structures.h"

namespace UMC
{
    struct VC1FrameConstrInfo
    {
        MediaData* in;
        MediaData* out;
        MediaDataEx::_MediaDataEx *stCodes;
        uint32_t splMode;
    };
    class vc1_frame_constructor
    {
    public:
            vc1_frame_constructor(){};
            virtual ~vc1_frame_constructor(){};

            virtual Status GetNextFrame(VC1FrameConstrInfo& pInfo) = 0;  //return 1 frame, 1 header....
            virtual Status GetFirstSeqHeader(VC1FrameConstrInfo& pInfo) = 0;
            virtual Status GetData(VC1FrameConstrInfo& pInfo) = 0;
            virtual Status ParseVC1SeqHeader (uint8_t *data, uint32_t* bufferSize, VideoStreamInfo* info) = 0;
            virtual void Reset() = 0;
    };

    class vc1_frame_constructor_rcv: public vc1_frame_constructor
    {
        public:
            vc1_frame_constructor_rcv();
            ~vc1_frame_constructor_rcv();

            Status GetNextFrame(VC1FrameConstrInfo& Info);
            Status GetData(VC1FrameConstrInfo& Info);
            Status GetFirstSeqHeader(VC1FrameConstrInfo& Info);
            void Reset();
            Status ParseVC1SeqHeader (uint8_t *data, uint32_t* bufferSize, VideoStreamInfo* info);
    };

    class vc1_frame_constructor_vc1: public vc1_frame_constructor
    {
         public:
            vc1_frame_constructor_vc1();
            ~vc1_frame_constructor_vc1();

            Status GetNextFrame(VC1FrameConstrInfo& Info);
            Status GetData(VC1FrameConstrInfo& Info);
            Status GetFirstSeqHeader(VC1FrameConstrInfo& Info);
            void Reset();
            Status ParseVC1SeqHeader (uint8_t *data, uint32_t* bufferSize, VideoStreamInfo* info);
    };
}

#endif//__UMC_VC1_SPL_FRAME_CONSTR_H__

#endif //UMC_ENABLE_VC1_SPLITTER || MFX_ENABLE_VC1_VIDEO_DECODE
