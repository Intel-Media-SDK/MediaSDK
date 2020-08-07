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

#ifndef __MFX_ENCTOOLS_H__
#define __MFX_ENCTOOLS_H__

#include "mfxcommon.h"
#include "mfxenctools-int.h"
#include "mfx_utils_defs.h"
#include "mfx_enctools_brc.h"
#include "mfx_enctools_aenc.h"
#include "mfx_enctools_utils.h"

#include <vector>
#include <memory>
#include <assert.h>
#include <algorithm>

using namespace EncToolsUtils;
using namespace EncToolsBRC;

mfxStatus InitCtrl(mfxVideoParam const & par, mfxEncToolsCtrl *ctrl);

class EncTools
{
private:

    bool       m_bVPPInit;
    bool       m_bInit;

    BRC_EncTool  m_brc;
    AEnc_EncTool m_scd;
    mfxExtEncToolsConfig m_config;
    mfxEncToolsCtrl  m_ctrl;
    mfxHDL m_device;
    mfxU32 m_deviceType;
    mfxFrameAllocator *m_pAllocator;
    MFXVideoSession m_mfxSession;

    std::unique_ptr<MFXVideoVPP> m_pmfxVPP;
    mfxVideoParam m_mfxVppParams;
    mfxFrameAllocResponse m_VppResponse;
    std::vector<mfxFrameSurface1> m_pIntSurfaces; // internal surfaces

public:
    EncTools() :

        m_bVPPInit(false),
        m_bInit(false),
        m_config(),
        m_device(0),
        m_pAllocator(0),
        m_mfxVppParams(),
        m_VppResponse()
    {}

    virtual ~EncTools() { Close(); }

    mfxStatus Init(mfxExtEncToolsConfig const * pEncToolConfig, mfxEncToolsCtrl const * ctrl);
    mfxStatus GetSupportedConfig(mfxExtEncToolsConfig* pConfig, mfxEncToolsCtrl const * ctrl);
    mfxStatus GetActiveConfig(mfxExtEncToolsConfig* pConfig);
    mfxStatus GetDelayInFrames(mfxExtEncToolsConfig const * pConfig, mfxEncToolsCtrl const * ctrl, mfxU32 *numFrames);
    mfxStatus Reset(mfxExtEncToolsConfig const * pEncToolConfig, mfxEncToolsCtrl const * ctrl);
    mfxStatus Close();

    mfxStatus Submit(mfxEncToolsTaskParam const * par);
    mfxStatus Query(mfxEncToolsTaskParam* par, mfxU32 timeOut);
    mfxStatus Discard(mfxU32 displayOrder);

protected:
    mfxStatus InitMfxVppParams(mfxEncToolsCtrl const & ctrl);
    mfxStatus InitVPP(mfxEncToolsCtrl const & ctrl);
    mfxStatus CloseVPP();
    mfxStatus VPPDownScaleSurface(mfxFrameSurface1 *pInSurface, mfxFrameSurface1 *pOutSurface);
};

namespace EncToolsFuncs
{

    inline mfxStatus Init(mfxHDL pthis, mfxExtEncToolsConfig* config, mfxEncToolsCtrl* ctrl)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((EncTools*)pthis)->Init(config, ctrl);
    }

    inline mfxStatus GetSupportedConfig(mfxHDL pthis, mfxExtEncToolsConfig* config, mfxEncToolsCtrl* ctrl)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((EncTools*)pthis)->GetSupportedConfig(config, ctrl);
    }
    inline mfxStatus GetActiveConfig(mfxHDL pthis, mfxExtEncToolsConfig* config)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((EncTools*)pthis)->GetActiveConfig(config);
    }
    inline mfxStatus Reset(mfxHDL pthis, mfxExtEncToolsConfig* config, mfxEncToolsCtrl* ctrl)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((EncTools*)pthis)->Reset(config, ctrl);
    }
    inline mfxStatus Close(mfxHDL pthis)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((EncTools*)pthis)->Close();
    }

    // Submit info to tool
    inline mfxStatus Submit(mfxHDL pthis, mfxEncToolsTaskParam* submitParam)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((EncTools*)pthis)->Submit(submitParam);
    }

    // Query info from tool
    inline mfxStatus Query(mfxHDL pthis, mfxEncToolsTaskParam* queryParam, mfxU32 timeOut)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((EncTools*)pthis)->Query(queryParam, timeOut);
    }

    // Query info from tool
    inline mfxStatus Discard(mfxHDL pthis, mfxU32 displayOrder)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((EncTools*)pthis)->Discard(displayOrder);
    }
    inline mfxStatus GetDelayInFrames(mfxHDL pthis, mfxExtEncToolsConfig* config, mfxEncToolsCtrl* ctrl, mfxU32 *numFrames)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((EncTools*)pthis)->GetDelayInFrames(config, ctrl, numFrames);
    }

}


class ExtBRC : public EncTools
{
public:

    mfxStatus GetFrameCtrl(mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl);
    mfxStatus Update(mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl, mfxBRCFrameStatus* status);
};


namespace ExtBRCFuncs
{

    inline mfxStatus Init(mfxHDL pthis, mfxVideoParam* par)
    {
        MFX_CHECK_NULL_PTR2(pthis,par);

        mfxExtEncToolsConfig config = {};
        mfxEncToolsCtrl ctrl = {};
        mfxStatus sts = MFX_ERR_NONE;

        sts = InitCtrl(*par, &ctrl);
        MFX_CHECK_STS(sts);

        sts = ((ExtBRC*)pthis)->GetSupportedConfig(&config, &ctrl);
        MFX_CHECK_STS(sts);
        MFX_CHECK(config.BRC != 0, MFX_ERR_UNSUPPORTED);

        config = {};
        config.BRC = MFX_CODINGOPTION_ON;

        return ((ExtBRC*)pthis)->Init(&config, &ctrl);
    }
    inline mfxStatus Close(mfxHDL pthis)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((EncTools*)pthis)->Close();
    }

    inline mfxStatus GetFrameCtrl(mfxHDL pthis, mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((ExtBRC*)pthis)->GetFrameCtrl(par, ctrl);
    }

    inline mfxStatus Update(mfxHDL pthis, mfxBRCFrameParam* par,  mfxBRCFrameCtrl* ctrl, mfxBRCFrameStatus* status)
    {
        MFX_CHECK_NULL_PTR1(pthis);
        return ((ExtBRC*)pthis)->Update(par, ctrl, status);
    }
    inline mfxStatus Reset(mfxHDL pthis, mfxVideoParam* par)
    {
        mfxExtEncToolsConfig config = {};
        mfxEncToolsCtrl ctrl = {};
        mfxStatus sts = MFX_ERR_NONE;

        config = {};
        config.BRC = MFX_CODINGOPTION_ON;

        sts = InitCtrl(*par, &ctrl);
        MFX_CHECK_STS(sts);

        MFX_CHECK_NULL_PTR1(pthis);
        return ((ExtBRC*)pthis)->Reset(&config, &ctrl);
    }
}


mfxExtBuffer* Et_GetExtBuffer(mfxExtBuffer** extBuf, mfxU32 numExtBuf, mfxU32 id);

inline bool isFieldMode(mfxEncToolsCtrl const & ctrl)
{
    return ((ctrl.CodecId == MFX_CODEC_HEVC) && !(ctrl.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE));
}

struct CompareByDisplayOrder
{
    mfxU32 m_DispOrder;
    CompareByDisplayOrder(mfxU32 frameOrder) : m_DispOrder(frameOrder) {}
    bool operator ()(BRC_FrameStruct const & frameStruct) const { return frameStruct.dispOrder == m_DispOrder; }
};


#endif
