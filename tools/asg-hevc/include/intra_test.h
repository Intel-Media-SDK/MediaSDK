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

//Some consts and enums for intra prediction

#ifndef __INTRA_TEST_H__
#define __INTRA_TEST_H__

#include "mfxvideo.h"
#include <string>

#if MFX_VERSION >= MFX_VERSION_NEXT

#include "mfxdefs.h"

enum INTRA_PART_MODE {
    INTRA_NONE = -1,
    INTRA_2Nx2N     = 0,
    INTRA_NxN       = 1
};

enum INTRA_MODE {
    NONE      = -1,
    PLANAR    = 0,
    DC        = 1,
    ANG2      = 2,
    ANG3      = 3,
    ANG4      = 4,
    ANG5      = 5,
    ANG6      = 6,
    ANG7      = 7,
    ANG8      = 8,
    ANG9      = 9,
    ANG10_HOR = 10,
    ANG11     = 11,
    ANG12     = 12,
    ANG13     = 13,
    ANG14     = 14,
    ANG15     = 15,
    ANG16     = 16,
    ANG17     = 17,
    ANG18     = 18,
    ANG19     = 19,
    ANG20     = 20,
    ANG21     = 21,
    ANG22     = 22,
    ANG23     = 23,
    ANG24     = 24,
    ANG25     = 25,
    ANG26_VER = 26,
    ANG27     = 27,
    ANG28     = 28,
    ANG29     = 29,
    ANG30     = 30,
    ANG31     = 31,
    ANG32     = 32,
    ANG33     = 33,
    ANG34     = 34
};
const mfxU32 INTRA_MODE_NUM = 35; //Planar + DC + 33 angular modes (2 thru 34)

enum INTRA_DIR {
    HORIZONTAL,
    VERTICAL,
    DIR_NOT_SPECIFIED
};

enum FILTER_TYPE {
    NO_FILTER,
    THREE_TAP_FILTER,
    STRONG_INTRA_SMOOTHING_FILTER
};

const mfxI8 angParam[] = { 32, 26, 21, 17, 13, 9, 5, 2, 0, -2, -5, -9, -13, -17, -21, -26, -32 };
const mfxI32 inverseAngParam[] = { -256, -315, -390, -482, -630, -910, -1638, -4096 };

inline bool isValidIntraMode(INTRA_MODE mode)
{
    // NONE is not a valid mode to operate
    return PLANAR <= mode && mode <= ANG34;
}

struct  IntraParams {
    INTRA_MODE intraMode = NONE;
    INTRA_DIR direction  = DIR_NOT_SPECIFIED;
    mfxI8 intraPredAngle = 0;
    mfxI32 invAngle      = 0;

    IntraParams(INTRA_MODE mode)
        : intraMode(mode)
    {
        if (!isValidIntraMode(mode))
        {
            throw std::string("ERROR: IntraParams: incorrect intra mode");
        }

        if (intraMode == PLANAR || intraMode == DC)
        {
            return;
        }

        if (intraMode < ANG18)
        {
            direction = HORIZONTAL;
            intraPredAngle = angParam[intraMode - 2];

            if (intraMode > ANG10_HOR)
                invAngle = inverseAngParam[18 - intraMode];
        }
        else
        {
            direction = VERTICAL;
            intraPredAngle = -angParam[intraMode - 18];

            if (intraMode < ANG26_VER)
                invAngle = inverseAngParam[intraMode - 18];
        }
    }
};

#endif // MFX_VERSION

#endif // !__INTRA_TEST_H__

