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

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include "random_generator.h"

ASGRandomGenerator& ASGRandomGenerator::GetInstance()
{
    static ASGRandomGenerator instance;

    return instance;
}

// Returns random number in range [min, max]
mfxI32 ASGRandomGenerator::GetRandomNumber(mfxI32 min, mfxI32 max)
{
    std::uniform_int_distribution<> distr(min, max);
    return distr(m_Gen);
}

// Returns +1 or -1 with probability 0.5
mfxI32 ASGRandomGenerator::GetRandomSign()
{
    return GetRandomBit() ? -1 : 1;
}

// Generates 0 or 1 with probability 0.5
bool ASGRandomGenerator::GetRandomBit()
{
    std::bernoulli_distribution distr(0.5);
    return distr(m_Gen);
}

// Returns TF or BF with probability 0.5
mfxI32 ASGRandomGenerator::GetRandomPicField()
{
    static const mfxI32 cases[] = { MFX_PICSTRUCT_FIELD_TOP, MFX_PICSTRUCT_FIELD_BOTTOM };
    std::uniform_int_distribution<> distr{ 0, sizeof(cases) / sizeof(cases[0]) - 1 };
    return cases[distr(m_Gen)];
}

void ASGRandomGenerator::SeedGenerator(mfxU32 extSeed)
{
    m_Gen.seed(extSeed);
}

#endif // MFX_VERSION
