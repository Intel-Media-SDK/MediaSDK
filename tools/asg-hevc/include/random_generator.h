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

#ifndef __ASG_HEVC_RANDOM_GENERATOR_H__
#define __ASG_HEVC_RANDOM_GENERATOR_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include <random>

#include "block_structures.h"


class ASGRandomGenerator
{
public:
    static ASGRandomGenerator& GetInstance();

    mfxI32 GetRandomNumber(mfxI32 min, mfxI32 max);
    mfxI32 GetRandomSign();
    bool   GetRandomBit();
    mfxI32 GetRandomPicField();

    // This method is considered as DEPRECATED - DO NOT USE IT IN THE FUTURE CHANGES
    // The only purpose of this function to support previous code
    // in order to sync with data generated with the previous ASG versions
    void SeedGenerator(mfxU32 extSeed);

private:
    static constexpr mfxU32 DEFAULT_SEED = 1;

    ASGRandomGenerator(mfxU32 seed = DEFAULT_SEED) : m_Gen(seed) {}

    std::mt19937 m_Gen; // random number generator
};

// Alias for static GetInstance to use in client code
constexpr auto GetRandomGen = &ASGRandomGenerator::GetInstance;

#endif // MFX_VERSION

#endif // __ASG_HEVC_RANDOM_GENERATOR_H__
