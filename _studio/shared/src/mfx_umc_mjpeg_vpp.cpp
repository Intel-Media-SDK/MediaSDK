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

#include "umc_defs.h"

#include "mfx_umc_alloc_wrapper.h"
#include "mfx_common.h"
#include "libmfx_core.h"
#include "mfx_common_int.h"


#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
#include "mfx_vpp_jpeg_d3d9.h"

UMC::Status mfx_UMC_FrameAllocator_D3D_Converter::InitMfx(UMC::FrameAllocatorParams *,
                                                          VideoCORE* mfxCore,
                                                          const mfxVideoParam *params,
                                                          const mfxFrameAllocRequest *request,
                                                          mfxFrameAllocResponse *response,
                                                          bool isUseExternalFrames,
                                                          bool isSWplatform)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    m_isSWDecode = isSWplatform;

    if (!mfxCore || !params)
        return UMC::UMC_ERR_NULL_PTR;

    if (!isUseExternalFrames && (!request || !response))
        return UMC::UMC_ERR_NULL_PTR;

    m_pCore = mfxCore;
    m_IsUseExternalFrames = isUseExternalFrames;

    UMC::ColorFormat color_format;

    switch (params->mfx.FrameInfo.FourCC)
    {
    case MFX_FOURCC_NV12:
        color_format = UMC::NV12;
        break;
    case MFX_FOURCC_RGB4:
        color_format = UMC::RGB32;
        break;
    //case MFX_FOURCC_YV12:
    //    color_format = UMC::YUV420;
    //    break;
    case MFX_FOURCC_YUV400:
        color_format = UMC::GRAY;
        break;
    case MFX_FOURCC_IMC3:
        color_format = UMC::IMC3;
        break;
    case MFX_FOURCC_YUV422H:
    case MFX_FOURCC_YUV422V:
        color_format = UMC::YUV422;
        break;
    case MFX_FOURCC_YUV444:
        color_format = UMC::YUV444;
        break;
    case MFX_FOURCC_YUV411:
        color_format = UMC::YUV411;
        break;
    case MFX_FOURCC_RGBP:
        color_format = UMC::YUV444;
        break;
    case MFX_FOURCC_YUY2:
        color_format = UMC::YUY2;
        break;
    default:
        return UMC::UMC_ERR_UNSUPPORTED;
    }

    UMC::Status umcSts = m_info.Init(params->mfx.FrameInfo.Width, params->mfx.FrameInfo.Height, color_format, 8);

    m_surface_info = params->mfx.FrameInfo;

    if (umcSts != UMC::UMC_OK)
        return umcSts;

    if (!m_IsUseExternalFrames ||
        !m_isSWDecode)
    {
        m_frameDataInternal.Resize(response->NumFrameActual);
        m_extSurfaces.resize(response->NumFrameActual);

        for (mfxU32 i = 0; i < response->NumFrameActual; i++)
        {
            mfxFrameSurface1 & surface = m_frameDataInternal.GetSurface(i);
            surface.Data.MemId = response->mids[i];

            MFX_INTERNAL_CPY(&surface.Info, &request->Info, sizeof(mfxFrameInfo));

            // fill UMC frameData
            UMC::FrameData& frameData = m_frameDataInternal.GetFrameData(i);

            // set correct width & height to planes
            frameData.Init(&m_info, (UMC::FrameMemID)i, this);
        }
    }

    mfxCore->SetWrapper(this);

    return UMC::UMC_OK;
}

UMC::Status mfx_UMC_FrameAllocator_D3D_Converter::Reset()
{
    m_pCc.reset();
    return mfx_UMC_FrameAllocator_D3D::Reset();
}

mfxStatus mfx_UMC_FrameAllocator_D3D_Converter::InitVideoVppJpegD3D9(const mfxVideoParam *params)
{
    bool isD3DToSys = false;

    if(params->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)
    {
        isD3DToSys = true;
    }
    else if (params->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
    {
        mfxExtOpaqueSurfaceAlloc *pOpaqAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(params->ExtParam, params->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        MFX_CHECK(pOpaqAlloc, MFX_ERR_INVALID_VIDEO_PARAM);

        isD3DToSys = (pOpaqAlloc->Out.Type & MFX_MEMTYPE_SYSTEM_MEMORY) != 0;
    }

    m_pCc.reset(new VideoVppJpegD3D9(m_pCore, isD3DToSys, params->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY));

    mfxStatus mfxSts;
    if (params->mfx.Rotation == MFX_ROTATION_90 || params->mfx.Rotation == MFX_ROTATION_270)
    {
        mfxVideoParam localParams = *params;

        // Frame allocation is possible inside VideoVppJpegD3D9::Init().
        // Those frames must have width/height of target image, so the swapping.
        std::swap(localParams.mfx.FrameInfo.Width, localParams.mfx.FrameInfo.Height);
        std::swap(localParams.mfx.FrameInfo.CropW, localParams.mfx.FrameInfo.CropH);

        mfxSts = m_pCc->Init(&localParams);
    }
    else
    {
        mfxSts = m_pCc->Init(params);
    }
    MFX_CHECK_STS( mfxSts );
    return mfxSts;
}

mfxStatus mfx_UMC_FrameAllocator_D3D_Converter::FindSurfaceByMemId(const UMC::FrameData* in, bool isOpaq,
                                                                   const mfxHDLPair &hdlPair,
                                                                   // output param
                                                                   mfxFrameSurface1 &surface)
{
    MFX_CHECK_NULL_PTR1(in);

    UMC::FrameMemID index = in->GetFrameMID();
    mfxMemId memInter = m_frameDataInternal.GetSurface(index).Data.MemId;
    mfxMemId memId = isOpaq?(memInter):(m_pCore->MapIdx(memInter));

    // if memid of in is same as memid of surface_work, StartPreparingToOutput() must not be called
    MFX_CHECK_WITH_ASSERT(!hdlPair.first || hdlPair.first != memId, MFX_ERR_UNSUPPORTED);

    surface = m_frameDataInternal.GetSurface(index);
    return MFX_ERR_NONE;
}

mfxStatus mfx_UMC_FrameAllocator_D3D_Converter::StartPreparingToOutput(mfxFrameSurface1 *surface_work,
                                                                       UMC::FrameData* in,
                                                                       const mfxVideoParam *par,
                                                                       mfxU16 *taskId,
                                                                       bool isOpaq)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    mfxStatus sts = MFX_ERR_NONE;

    if (!m_pCc)
    {
        MFX_SAFE_CALL( InitVideoVppJpegD3D9(par) );
    }

    mfxHDLPair hdlPair;
    if(isOpaq)
        sts = m_pCore->GetFrameHDL(surface_work->Data.MemId, (mfxHDL*)&hdlPair);
    else
        sts = m_pCore->GetExternalFrameHDL(surface_work->Data.MemId, (mfxHDL*)&hdlPair);
    if (sts == MFX_ERR_UNDEFINED_BEHAVIOR // nothing found by Get*FrameHDL()
        || sts == MFX_ERR_UNSUPPORTED)    // Get*FrameHDL() does not support obtaining OS-specific handle
    {
        hdlPair.first = nullptr;
    }
    else
    {
        MFX_CHECK_STS(sts);
    }

    // for interlaced case, [0] is top and [1] is bottom; for progressive only [0] is used
    mfxFrameSurface1 srcSurface[2];
    for (int i = 0; i < 1 + (par->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE); ++i)
    {
        MFX_SAFE_CALL( FindSurfaceByMemId(&in[i], isOpaq, hdlPair, srcSurface[i]) );

        /* JPEG standard does not support crops as it is done in AVC, so:
           - CropX and CropY are always 0,
           - CropW and CropH represents picture size for current frame (in case of rotation,
           surface_work has rotated CropW and CropH),
           - Width and Height represents surface allocation size (they are initialized
           in decoder Init and are correct).
        */
        if (par->mfx.Rotation == MFX_ROTATION_0 || par->mfx.Rotation == MFX_ROTATION_180)
        {
            srcSurface[i].Info.CropW = surface_work->Info.CropW;
            srcSurface[i].Info.CropH = surface_work->Info.CropH;
        }
        else
        {
            srcSurface[i].Info.CropW = surface_work->Info.CropH;
            srcSurface[i].Info.CropH = surface_work->Info.CropW;
        }
        if (par->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
        {
            srcSurface[i].Info.CropH /= 2;
        }
    }
    return par->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE?
        m_pCc->BeginHwJpegProcessing(&srcSurface[0], surface_work, taskId) :
        m_pCc->BeginHwJpegProcessing(&srcSurface[0], &srcSurface[1], surface_work, taskId);
}

mfxStatus mfx_UMC_FrameAllocator_D3D_Converter::CheckPreparingToOutput(mfxFrameSurface1 *surface_work,
                                                                       UMC::FrameData* in,
                                                                       const mfxVideoParam * par,
                                                                       mfxU16 taskId)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    MFX_CHECK_NULL_PTR1(m_pCc);
    mfxStatus sts = m_pCc->QueryTaskRoutine(taskId);
    if (sts == MFX_TASK_BUSY)
    {
        return sts;
    }
    if (sts != MFX_TASK_DONE)
        return sts;

    if(par->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE)
    {
        UMC::FrameMemID index = in->GetFrameMID();

        mfxFrameSurface1 src = m_frameDataInternal.GetSurface(index);
        //Performance issue. We need to unlock mutex to let decoding thread run async.
        guard.Unlock();
        sts = m_pCc->EndHwJpegProcessing(&src, surface_work);
        guard.Lock();
        if (sts < MFX_ERR_NONE)
            return sts;

        if (!m_IsUseExternalFrames)
        {
            m_pCore->DecreaseReference(&surface_work->Data);
            m_extSurfaces[index].FrameSurface = 0;
        }
    }
    else
    {
        UMC::FrameMemID indexTop = in[0].GetFrameMID();
        UMC::FrameMemID indexBottom = in[1].GetFrameMID();

        mfxFrameSurface1 srcTop, srcBottom;

        srcTop = m_frameDataInternal.GetSurface(indexTop);
        srcBottom = m_frameDataInternal.GetSurface(indexBottom);

        //Performance issue. We need to unlock mutex to let decoding thread run async.
        guard.Unlock();
        sts = m_pCc->EndHwJpegProcessing(&srcTop, &srcBottom, surface_work);
        guard.Lock();
        if (sts < MFX_ERR_NONE)
            return sts;

        if (!m_IsUseExternalFrames)
        {
            m_pCore->DecreaseReference(&surface_work->Data);
            m_extSurfaces[indexTop].FrameSurface = 0;
        }
    }

    return MFX_ERR_NONE;
}

void mfx_UMC_FrameAllocator_D3D_Converter::SetJPEGInfo(mfx_UMC_FrameAllocator_D3D_Converter::JPEG_Info * jpegInfo)
{
    m_jpegInfo = *jpegInfo;
}

#endif //defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
