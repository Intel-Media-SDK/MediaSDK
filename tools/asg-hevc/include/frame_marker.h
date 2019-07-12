// Copyright (c) 2018-2019 Intel Corporation
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

#ifndef __ASG_FRAME_MAKER_H__
#define __ASG_FRAME_MAKER_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include "inputparameters.h"

inline bool GenerateInterMBs(mfxU16 test_type) { return !!(test_type & (~(GENERATE_INTRA | GENERATE_SPLIT))); }
inline bool HasBFramesInGOP(mfxU16 refDist) { return refDist > 1; }

class FrameMarker
{
public:
    void PreProcessStreamConfig(InputParams & params);
private:
    void BuildRefLists(const InputParams & params);
    void TagFrames(const InputParams & params);
    bool HasAlreadyUsedRefs(mfxU32 frame, mfxU8 list);
    void SetRefList(mfxU32 frame, mfxU8 list, std::vector<mfxU32> & refFrames, mfxU32 & num_gen);

    std::vector<FrameProcessingParam> m_vProcessingParams; // FrameProcessingParam for entire stream
};

#endif // MFX_VERSION

#endif // __ASG_FRAME_MAKER_H__
