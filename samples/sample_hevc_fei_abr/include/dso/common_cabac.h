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

#include "bs_reader2.h"
#include "hevc_cabac.h"

#define BS_AVC2_ADE_MODE 1 //0 - literal standard implementation, 1 - optimized(2-byte buffering affects SE offsets in trace)

namespace COMMON_CABAC
{
    class ADE
    {
    private:
        BsReader2::Reader& m_bs;
        Bs64u m_bpos  = 0;
        bool  m_pcm   = false;
#if (BS_AVC2_ADE_MODE == 1)
        Bs32u m_val   = 0;
        Bs32u m_range = 0;
        Bs32s m_bits  = 0;

        inline Bs32u B(Bs16u n) { m_bpos += n * 8; return m_bs.GetBytes(n, true); }
#else
        Bs16u m_codIRange;
        Bs16u m_codIOffset;

        inline Bs16u b(Bs16u n) { m_bpos += n; return m_bs.GetBits(n); }
        inline Bs16u b() { m_bpos++; return m_bs.GetBit(); }
#endif

    public:
        ADE(BsReader2::Reader& r) : m_bs(r) {}
        ~ADE() {}

        inline void Init()
        {
            #if (BS_AVC2_ADE_MODE == 1)
            m_val = B(3);
            m_range = 510;
            m_bits = 15;

            if (!m_pcm)
                m_bpos = 15;
            #else
            m_codIRange = 510;
            m_codIOffset = b(9);

            if (!m_pcm)
                m_bpos = 0;
            #endif
            m_pcm = false;
        }

        Bs8u DecodeDecision(Bs8u& ctxState);
        Bs8u DecodeBypass();
        Bs8u DecodeTerminate();

        #if (BS_AVC2_ADE_MODE == 1)
        inline Bs16u GetR() { return Bs16u(m_range); }
        inline Bs16u GetV() { return Bs16u((m_val >> (m_bits))); }
        inline Bs64u BitCount() { return m_bpos - m_bits; }
        inline void  InitPCMBlock() { m_bs.ReturnBits(m_bits); m_bpos -= m_bits; m_bits = 0; m_pcm = true; }
        #else
        inline Bs16u GetR() { return m_codIRange; }
        inline Bs16u GetV() { return m_codIOffset; }
        inline Bs64u BitCount() { return m_bpos; }
        inline void  InitPCMBlock() { m_pcm = true; };
        #endif

    };

}