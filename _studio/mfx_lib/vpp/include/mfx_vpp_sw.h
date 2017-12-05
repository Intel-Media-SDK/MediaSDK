// Copyright (c) 2017 Intel Corporation
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

/* ****************************************************************************** */

#include "mfx_common.h"

#if defined (MFX_ENABLE_VPP)

#ifndef __MFX_VPP_SW_H
#define __MFX_VPP_SW_H

#include <memory>

#include "mfxvideo++int.h"

#include "mfx_vpp_defs.h"
#include "mfx_vpp_base.h"
#include "mfx_task.h"


/* ******************************************************************** */
/*                 Core Class of MSDK VPP                               */
/* ******************************************************************** */

#include "mfx_vpp_hw.h"

class VideoVPPBase
{
public:

  static mfxStatus Query( VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out);
  static mfxStatus QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request);
  
  VideoVPPBase(VideoCORE *core, mfxStatus* sts);
  virtual ~VideoVPPBase();

  // VideoBase methods
  virtual mfxStatus Reset(mfxVideoParam *par);
  virtual mfxStatus Close(void);
  virtual mfxStatus Init(mfxVideoParam *par);

  virtual mfxStatus GetVideoParam(mfxVideoParam *par);
  virtual mfxStatus GetVPPStat(mfxVPPStat *stat);
  virtual mfxStatus VppFrameCheck(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux,
                                  MFX_ENTRY_POINT pEntryPoint[], mfxU32 &numEntryPoints);

  virtual mfxStatus VppFrameCheck(mfxFrameSurface1 *, mfxFrameSurface1 *)
  {
      return MFX_ERR_UNSUPPORTED;
  }

  virtual mfxStatus RunFrameVPP(mfxFrameSurface1* in, mfxFrameSurface1* out, mfxExtVppAuxData *aux) = 0;

  virtual mfxTaskThreadingPolicy GetThreadingPolicy(void);

protected:

  typedef struct
  {
    mfxFrameInfo    In;
    mfxFrameInfo    Out;
    mfxFrameInfo    Deffered;

    mfxU16          IOPattern;
    mfxU16          AsyncDepth;

    bool            isInited;
    bool            isFirstFrameProcessed;
    bool            isCompositionModeEnabled;

  } sErrPrtctState;

  virtual mfxStatus InternalInit(mfxVideoParam *par) = 0;

  // methods of sync part of VPP for check info/params
  mfxStatus CheckIOPattern( mfxVideoParam* par );

  static mfxStatus QueryCaps(VideoCORE * core, MfxHwVideoProcessing::mfxVppCaps& caps);
  static mfxStatus CheckPlatformLimitations( VideoCORE* core, mfxVideoParam & param, bool bCorrectionEnable );
  mfxU32    GetNumUsedFilters();

  std::vector<mfxU32> m_pipelineList; // list like DO_USE but contains some internal filter names (ex RESIZE, DEINTERLACE etc) 

  //
  bool               m_bDynamicDeinterlace;

  VideoCORE*  m_core;
  mfxVPPStat  m_stat;

  // protection state. Keeps Init/Reset params that allows checking that RunFrame params
  // do not violate Init/Reset params
  sErrPrtctState m_errPrtctState;

  // State that keeps Init params. They are changed on Init only
  sErrPrtctState m_InitState;

  // opaque processing
  bool                  m_bOpaqMode[2];
  mfxFrameAllocRequest  m_requestOpaq[2];

  //
  // HW VPP Support
  //
  std::unique_ptr<MfxHwVideoProcessing::VideoVPPHW>     m_pHWVPP;
};

class VideoVPP_HW : public VideoVPPBase
{
public:
    VideoVPP_HW(VideoCORE *core, mfxStatus* sts);

    virtual mfxStatus InternalInit(mfxVideoParam *par);
    virtual mfxStatus Close(void);
    virtual mfxStatus Reset(mfxVideoParam *par);

    virtual mfxStatus GetVideoParam(mfxVideoParam *par);

    virtual mfxStatus VppFrameCheck(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux,
                                    MFX_ENTRY_POINT pEntryPoints[], mfxU32 &numEntryPoints);

    virtual mfxStatus RunFrameVPP(mfxFrameSurface1* in, mfxFrameSurface1* out, mfxExtVppAuxData *aux);

    mfxStatus PassThrough(mfxFrameInfo* In, mfxFrameInfo* Out, mfxU32 taskIndex);
};


mfxStatus RunFrameVPPRoutine(void *pState, void *pParam, mfxU32 threadNumber, mfxU32 callNumber);
mfxStatus CompleteFrameVPPRoutine(void *pState, void *pParam, mfxStatus taskRes);

VideoVPPBase* CreateAndInitVPPImpl(mfxVideoParam *par, VideoCORE *core, mfxStatus *mfxSts);

#endif // __MFX_VPP_SW_H

#endif // MFX_ENABLE_VPP
/* EOF */
