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

#pragma once

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base.h"
#include "hevcehw_base_data.h"

namespace HEVCEHW
{
namespace Base
{

class ExtBRC
    : public FeatureBase
{
public:
    //TODO: to align with legacy code behavior, disable after investigation effect on SWBRC
    static const bool IGNORE_P_PYRAMID_LEVEL = true; 

#define DECL_BLOCK_LIST\
    DECL_BLOCK(Check)\
    DECL_BLOCK(Init)\
    DECL_BLOCK(ResetCheck)\
    DECL_BLOCK(Reset)\
    DECL_BLOCK(GetFrameCtrl)\
    DECL_BLOCK(Update)\
    DECL_BLOCK(Close)\
    DECL_BLOCK(SetDefaults)
#define DECL_FEATURE_NAME "Base_ExtBRC"
#include "hevcehw_decl_blocks.h"

    ExtBRC(mfxU32 FeatureId)
        : FeatureBase(FeatureId)
    {}

protected:
    virtual void SetSupported(ParamSupport& par) override;
    virtual void SetInherited(ParamInheritance& par) override;
    virtual void Query1NoCaps(const FeatureBlocks& blocks, TPushQ1 Push) override;
    virtual void SetDefaults(const FeatureBlocks& blocks, TPushSD Push) override;
    virtual void InitAlloc(const FeatureBlocks& blocks, TPushIA Push) override;
    virtual void Reset(const FeatureBlocks& blocks, TPushR Push) override;
    virtual void ResetState(const FeatureBlocks& blocks, TPushRS Push) override;
    virtual void SubmitTask(const FeatureBlocks& blocks, TPushST Push) override;
    virtual void QueryTask(const FeatureBlocks& blocks, TPushQT Push) override;
    virtual void Close(const FeatureBlocks& blocks, TPushCLS Push) override;

    mfxBRCFrameParam MakeFrameParam(const TaskCommonPar& task);

    mfxExtBRC m_brc = {};
    OnExit    m_destroy;
    bool      m_bUseLevel = true;
    NotNull<mfxExtLAFrameStatistics*> m_pLAStat;
};

} //Base
} //namespace HEVCEHW

#endif