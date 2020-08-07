// Copyright (c) 2019-2020 Intel Corporation
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

// enctools.cpp : Defines the exported functions for the DLL application.

#include "mfxenctools-int.h"
#include "mfx_enctools.h"
#include "mfx_enctools_brc.h"

mfxEncTools* MFXVideoENCODE_CreateEncTools()
{
    std::unique_ptr<mfxEncTools> et(new mfxEncTools);
    et->Context = new EncTools;
    et->Init = EncToolsFuncs::Init;
    et->Reset = EncToolsFuncs::Reset;
    et->Close = EncToolsFuncs::Close;
    et->Submit = EncToolsFuncs::Submit;
    et->Query = EncToolsFuncs::Query;
    et->Discard = EncToolsFuncs::Discard;
    et->GetSupportedConfig = EncToolsFuncs::GetSupportedConfig;
    et->GetActiveConfig = EncToolsFuncs::GetActiveConfig;
    et->GetDelayInFrames = EncToolsFuncs::GetDelayInFrames;

    return et.release();
}

void MFXVideoENCODE_DestroyEncTools(mfxEncTools *et)
{
    if (et != NULL)
    {
        delete (EncTools*)et->Context;
        delete et;
    }
}


mfxExtBRC* MFXVideoENCODE_CreateExtBRC()
{
    std::unique_ptr <mfxExtBRC> ebrc(new mfxExtBRC);
    ebrc->Header.BufferId = MFX_EXTBUFF_BRC;
    ebrc->Header.BufferSz = sizeof(*ebrc);
    ebrc->pthis = new ExtBRC;
    ebrc->Init = ExtBRCFuncs::Init;
    ebrc->Reset = ExtBRCFuncs::Reset;
    ebrc->Close = ExtBRCFuncs::Close;
    ebrc->GetFrameCtrl = ExtBRCFuncs::GetFrameCtrl;
    ebrc->Update = ExtBRCFuncs::Update;

    return ebrc.release();
}

void MFXVideoENCODE_DestroyExtBRC(mfxExtBRC *ebrc)
{
    if (ebrc != NULL)
    {
        delete (ExtBRC*)ebrc->pthis;
        delete ebrc;
    }
}

