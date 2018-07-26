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

#include "common_cabac.h"

using namespace COMMON_CABAC;
using namespace BS_HEVC;

// ADE ////////////////////////////////////////////////////////////////////////
static const Bs8u renormTab[32] =
{
    6, 5, 4, 4, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1
};

Bs8u ADE::DecodeDecision(Bs8u& ctxState)
{
#if (BS_AVC2_ADE_MODE == 1)
    Bs32u vMPS = (ctxState & 1);
    Bs32u bin = vMPS;
    Bs32u state = (ctxState >> 1);
    Bs32u rLPS = rangeTabLpsT[(m_range >> 6) & 3][state];

    m_range -= rLPS;

    if (m_val < (m_range << m_bits))
    {
        ctxState = (transIdxMps[state] << 1) | vMPS;

        if (m_range >= 256)
            return bin;

        m_range <<= 1;
        m_bits--;
    }
    else
    {
        Bs8u renorm = renormTab[(rLPS >> 3) & 0x1F];
        m_val -= (m_range << m_bits);
        m_range = (rLPS << renorm);
        m_bits -= renorm;

        bin ^= 1;
        if (!state)
            vMPS ^= 1;

        ctxState = (transIdxLps[state] << 1) | vMPS;
    }

    if (m_bits > 0)
        return bin;

    m_val = (m_val << 16) | B(2);
    m_bits += 16;

    return bin;
#else
    Bs8u  binVal = 0;
    Bs8u  valMPS = (ctxState & 1);
    Bs8u  pStateIdx = (ctxState >> 1);
    Bs16u codIRangeLPS = rangeTabLpsT[(m_codIRange >> 6) & 3][pStateIdx];

    m_codIRange -= codIRangeLPS;

    if (m_codIOffset >= m_codIRange)
    {
        binVal = !valMPS;
        m_codIOffset -= m_codIRange;
        m_codIRange = codIRangeLPS;

        if (pStateIdx == 0)
            valMPS = !valMPS;

        pStateIdx = transIdxLps[pStateIdx];

    }
    else
    {
        binVal = valMPS;
        pStateIdx = transIdxMps[pStateIdx];
    }

    ctxState = (pStateIdx << 1) | valMPS;

    while (m_codIRange < 256)
    {
        m_codIRange <<= 1;
        m_codIOffset = (m_codIOffset << 1) | b();
    }

    return binVal;
#endif
}
Bs8u ADE::DecodeBypass()
{
#if (BS_AVC2_ADE_MODE == 1)
    if (!--m_bits)
    {
        m_val = (m_val << 16) | B(2);
        m_bits = 16;
    }

    Bs32s val = m_val - (m_range << m_bits);

    if (val < 0)
        return 0;

    m_val = Bs32u(val);
    return 1;
#else
    m_codIOffset = (m_codIOffset << 1) | b();
    //nBits++;

    if (m_codIOffset >= m_codIRange)
    {
        m_codIOffset -= m_codIRange;
        return 1;
    }
    return 0;
#endif
}
Bs8u ADE::DecodeTerminate()
{
#if (BS_AVC2_ADE_MODE == 1)
    Bs32u range = m_range - 2;
    Bs32s val = m_val - (range << m_bits);

    if (val < 0)
    {
        if (range >= 256)
        {
            m_range = range;
            return 0;
        }

        m_range = (range << 1);

        if (--m_bits)
            return 0;

        m_val = (m_val << 16) | B(2);
        m_bits = 16;

        return 0;
    }

    m_range = range;

    return 1;
#else
    m_codIRange -= 2;

    if (m_codIOffset >= m_codIRange)
        return 1;

    while (m_codIRange < 256)
    {
        m_codIRange <<= 1;
        m_codIOffset = (m_codIOffset << 1) | b();
    }

    return 0;
#endif
}
