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

#include "ts_encoder.h"
#include "ts_vpp.h"
#include "ts_decoder.h"

class tsTranscoder : public tsVideoVPP, public tsVideoDecoder, public tsVideoEncoder
{
public:
    struct SP
    {
        mfxSyncPoint sp;
        mfxBitstream bs;
    };
    tsExtBufType<mfxVideoParam>& m_parDEC;
    tsExtBufType<mfxVideoParam>& m_parVPP;
    tsExtBufType<mfxVideoParam>& m_parENC;
    tsBitstreamProcessor*&       m_bsProcIn;
    tsBitstreamProcessor*&       m_bsProcOut;
    mfxU16                       m_cId; // 0=dec, 1=vpp, 2=enc
    std::list<SP>                m_sp_free;
    std::list<SP>                m_sp_working;

    tsTranscoder(mfxU32 codecDec, mfxU32 codecEnc);

    ~tsTranscoder();

    void InitAlloc();

    void AllocSurfaces();

    void InitPipeline(mfxU16 AsyncDepth = 5);

    bool SubmitDecoding();

    bool SubmitVPP();

    bool SubmitEncoding();

    void TranscodeFrames(mfxU32 n);

    mfxFrameSurface1* GetSurface();
};
