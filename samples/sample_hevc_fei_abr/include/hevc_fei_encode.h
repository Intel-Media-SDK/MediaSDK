/******************************************************************************\
Copyright (c) 2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#ifndef __SAMPLE_FEI_ENCODE_H__
#define __SAMPLE_FEI_ENCODE_H__

#include "task.h"
#include "fei_buffer_allocator.h"
#include "abr_brc.h"
#include "fei_worker.h"

class IEncoder
{
public:
    virtual ~IEncoder() {};
    virtual mfxStatus Init() = 0;
    virtual mfxStatus Query() = 0;
    virtual mfxStatus Reset(mfxVideoParam& par) = 0;
    virtual mfxStatus QueryIOSurf(mfxFrameAllocRequest* request) = 0;

    virtual mfxStatus PreInit() = 0;
    virtual MfxVideoParamsWrapper   GetVideoParam() = 0;
    virtual mfxStatus SubmitFrame(std::shared_ptr<HevcTaskDSO> & task) = 0;
    // BRC part
    BRC* CreateBRC(const sBrcParams& brc_params, const mfxVideoParam& video_param)
    {
        switch (brc_params.eBrcType)
        {
        case MSDKSW:
            return static_cast<BRC*>(new SW_BRC(video_param, brc_params.TargetKbps));
            break;
        case LOOKAHEAD:
            return static_cast<BRC*>(new LA_BRC(video_param, brc_params.TargetKbps, brc_params.LookAheadDepth, brc_params.LookBackDepth, brc_params.AdaptationLength));
            break;
        case NONE:
        default:
            return nullptr;
            break;
        }
    }
    // Submit new data to BRC
    virtual void UpdateBRCStat(FrameStatData& stat_data) = 0;
};


class FEI_Encode : public IEncoder
{
public:
    FEI_Encode(MFXVideoSession* session, MfxVideoParamsWrapper& par,
        const mfxExtFeiHevcEncFrameCtrl& frame_ctrl, const PerFrameTypeCtrl& frametype_ctrl,
        const msdk_char* outFile, const sBrcParams& brc_params = sBrcParams());

    ~FEI_Encode();

    mfxStatus Init();
    mfxStatus Query();
    mfxStatus Reset(mfxVideoParam& par);
    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request);

    mfxStatus PreInit();
    MfxVideoParamsWrapper   GetVideoParam();
    mfxStatus SubmitFrame(std::shared_ptr<HevcTaskDSO> & task)
    {
        if (task.get() && task->m_surf)
            msdk_atomic_inc16((volatile mfxU16*)&task->m_surf->Data.Locked);

        m_working_queue.Push( [ task, this ] () mutable
        {
            DoWork(task);

            if (task.get() && task->m_surf)
                msdk_atomic_dec16((volatile mfxU16*)&task->m_surf->Data.Locked);

            task.reset();
        });

        return MFX_ERR_NONE;
    }

    void UpdateBRCStat(FrameStatData& stat_data)
    {
        if (m_pBRC.get())
        {
            m_pBRC->SubmitNewStat(stat_data);
        }
    };

private:
    MFXVideoSession*      m_pmfxSession = nullptr;
    MFXVideoENCODE        m_mfxENCODE;

    MfxVideoParamsWrapper m_videoParams;
    mfxEncodeCtrlWrap     m_encodeCtrl;

    mfxBitstreamWrapper   m_bitstream;
    mfxSyncPoint          m_syncPoint;

    std::string           m_dstFileName;
    CSmplBitstreamWriter  m_FileWriter;

    mfxExtFeiHevcEncFrameCtrl m_defFrameCtrl; // contain default and user-specified options per frame
    PerFrameTypeCtrl          m_ctrlPerFrameType; // contain default and user-specified options per frame type

    mfxStatus DoWork(std::shared_ptr<HevcTaskDSO> & task);
    mfxStatus EncodeFrame(mfxFrameSurface1* pSurf);
    mfxStatus SetCtrlParams(const HevcTaskDSO& task);
    mfxStatus SyncOperation();

    mfxStatus AllocateSufficientBuffer();

    std::unique_ptr<BRC> m_pBRC;

    Worker               m_working_queue;

    DISALLOW_COPY_AND_ASSIGN(FEI_Encode);
};

// Class draws MVPs on NV12 picture
class MVPOverlay : public IEncoder
{
public:
    MVPOverlay(IEncoder * pBase, MFXFrameAllocator * allocator, std::shared_ptr<FeiBufferAllocator> & bufferAllocator, std::string & file)
        : m_pBase(pBase)
        , m_allocator(allocator)
        , m_buffAlloc(bufferAllocator)
        , m_yuvName(file) {}

    virtual ~MVPOverlay() {};
    virtual mfxStatus Init();
    virtual mfxStatus Query() { return m_pBase->Query(); }
    virtual mfxStatus Reset(mfxVideoParam& par) { return m_pBase->Reset(par); }
    virtual mfxStatus QueryIOSurf(mfxFrameAllocRequest* request) { return m_pBase->QueryIOSurf(request); }

    virtual mfxStatus PreInit() { return m_pBase->PreInit(); }
    virtual MfxVideoParamsWrapper   GetVideoParam() { return m_pBase->GetVideoParam(); }
    virtual mfxStatus SubmitFrame(std::shared_ptr<HevcTaskDSO> & task);
    virtual void UpdateBRCStat(FrameStatData& stat_data) { m_pBase->UpdateBRCStat(stat_data); }
private:

    struct MfxFrameSurface1Wrap: public mfxFrameSurface1
    {
        MfxFrameSurface1Wrap() : mfxFrameSurface1()
        {}
        ~MfxFrameSurface1Wrap()
        {}

        mfxStatus Alloc(mfxU32 fourcc, mfxU32 width, mfxU32 height)
        {
            if (fourcc != MFX_FOURCC_NV12)
                return MFX_ERR_UNSUPPORTED;

            mfxFrameSurface1 & srf = *this;

            mfxU32 pitch = width;
            mfxU32 bufferSize = 0;

            srf.Info.Height = height;
            srf.Info.Width = width;
            srf.Info.FourCC = fourcc;
            srf.Data.PitchHigh = (mfxU16)(pitch >> 16);
            srf.Data.PitchLow = (mfxU16)pitch;

            bufferSize = 3 * pitch * height / 2;
            m_buffer.resize(bufferSize);

            srf.Data.Y = m_buffer.data();
            srf.Data.UV = srf.Data.Y + pitch * srf.Info.Height;

            return MFX_ERR_NONE;
        }

    private:
        std::vector<mfxU8> m_buffer;

    private:
        DISALLOW_COPY_AND_ASSIGN(MfxFrameSurface1Wrap);
    };

    std::unique_ptr<IEncoder>  m_pBase;

    MFXFrameAllocator *        m_allocator = nullptr;
    std::shared_ptr<FeiBufferAllocator> m_buffAlloc;

    MfxFrameSurface1Wrap       m_surface;

    CSmplYUVWriter             m_yuvWriter;
    std::string                m_yuvName;

    mfxU32                     m_width = 0;
    mfxU32                     m_height = 0;

private:
    DISALLOW_COPY_AND_ASSIGN(MVPOverlay);
};

#endif // __SAMPLE_FEI_ENCODE_H__
