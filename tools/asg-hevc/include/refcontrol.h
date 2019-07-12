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

#ifndef __REFCONTROL_H__
#define __REFCONTROL_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include <fstream>
#include <vector>

#include "inputparameters.h"
#include "random_generator.h"

// These structures are used in asg-hevc-based PicStruct test

struct PictureInfo
{
    mfxI32 orderCount  = -1; // display order
    mfxI32 codingOrder = -1;
    mfxU32 frameType   = MFX_FRAMETYPE_UNKNOWN; // IPB, IDR, ST/LT
    mfxU32 picStruct   = MFX_PICSTRUCT_UNKNOWN; // TF/BF

    bool operator==(const PictureInfo& rhs) const
    {
        return codingOrder == rhs.codingOrder &&
            orderCount == rhs.orderCount &&
            picStruct == rhs.picStruct &&
            frameType == rhs.frameType;
    }
};

struct RefState
{
    PictureInfo picture;
    std::vector<mfxI32> DPB;              // stores FrameOrder (POC)
    std::vector<mfxI32> RefListActive[2]; // (POC), any from DPB, can be repeated

    void Dump(std::fstream& fp) const;
    bool Load(std::fstream& fp); // returns false on error

    bool operator==(const RefState& rhs) const
    {
        return
            picture == rhs.picture &&
            DPB == rhs.DPB &&
            RefListActive[0] == rhs.RefListActive[0] &&
            RefListActive[1] == rhs.RefListActive[1];
    }
};

class RefControl
{
public:
    std::vector<RefState> RefLogInfo;

    void Add(mfxI32 frameOrder);
    void Encode(mfxI32 frameOrder);
    void SetParams(const InputParams& params);
private:
    mfxU32 maxDPBSize = 1;         // max num ref + 1 for current
    mfxU32 maxDelay = 1;         // == number of allocated surfaces used in encoder reordering

    std::vector<PictureInfo> DPB;  // can temporary be maxDPBSize+1, stores all active pictures

    std::vector<PictureInfo> RPB;  // reordered picture buffer, pic goes to DPB when coded

    InputParams m_params;
    mfxI32 m_lastIDR = 0;
};

// TODO: Replace global functions with lambdas
bool IsOlder(const PictureInfo& a, const PictureInfo& b);
bool IsInCodingOrder(const RefState& a, const RefState& b);


#endif // MFX_VERSION

#endif // __REFCONTROL_H__
