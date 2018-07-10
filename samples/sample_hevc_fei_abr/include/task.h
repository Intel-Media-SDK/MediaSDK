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

#ifndef __HEVC_FEI_ABR_TASK__
#define __HEVC_FEI_ABR_TASK__

#include "buffer_pool.h"
#include "base_allocator.h"

#include <queue>
#include <cmath>
#include <map>

struct FrameStatData
{
    mfxU32 EncodedOrder                  = 0xffffffff;
    mfxU32 DisplayOrder                  = 0xffffffff;
    mfxU8  QP                            = 0xff;
    mfxF64 QstepOriginal                 = -1.0;
    mfxF64 QstepCalculated               = -1.0;
    mfxU16 FrameType                     = MFX_FRAMETYPE_UNKNOWN;
    mfxI32 POC                           = -100;
    mfxF64 ShareIntra                    = 0.0;
    mfxF64 Propagation                   = 0.0;
    mfxU32 FrameSize                     = 0;
    mfxF64 VisualDistortion              = 0.0; // MSE or any approximation like number of nonzero coefficients
    mfxF64 ComplexityOriginal            = 0.0;
    mfxF64 ComplexitySmoothed            = 0.0;

    mfxU32 NPixelsPropagated             = 0;
    mfxF64 NPixelsTransitivelyPropagated = 1.0;
    mfxU32 NPixelsInFrame                = 0;

    mfxU64 BitsEncoded                   = 0;
    mfxU64 BitsPredicted                 = 0;

    // Map of FrameDisplayOrder <=> NumOfPixels.
    // Stores amount of pixels predicted FROM reference frame.
    std::map<mfxU32, mfxU32> NumPixelsPredictedFromRef;
    // Map of FrameDisplayOrder <=> ShareOfPixels.
    // Shows share of INTER pixels predicted FROM reference frame.
    std::map<mfxU32, mfxF64> ShareOfPredictedPixelsFromRef;

    bool Contains(mfxU32 fo)
    {
        return NumPixelsPredictedFromRef.find(fo) != NumPixelsPredictedFromRef.end();
    }

    void AddPxlCount(mfxU32 fo, mfxU32 amount)
    {
        if (Contains(fo))
        {
            NumPixelsPredictedFromRef[fo] += amount;
        }
        else
        {
            NumPixelsPredictedFromRef[fo] = amount;
        }
    }

    void FillProp()
    {
        for (auto & item : NumPixelsPredictedFromRef)
        {
            mfxF64 denom = (1.0 - ShareIntra) * NPixelsInFrame;
            if (denom == 0.0)
                throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "ERROR: FillProp - zero denominator");

            ShareOfPredictedPixelsFromRef[item.first] = mfxF64(item.second) / denom;
        }
    }
};

struct HevcTaskDSO
{
    HevcTaskDSO()                              = default;
    HevcTaskDSO(HevcTaskDSO const&)            = delete;
    HevcTaskDSO(HevcTaskDSO &&)                = default;
    HevcTaskDSO& operator=(HevcTaskDSO const&) = delete;
    HevcTaskDSO& operator=(HevcTaskDSO &&)     = default;

    mfxFrameSurface1 *  m_surf       = nullptr;
    mfxU16              m_frameType  = MFX_FRAMETYPE_UNKNOWN;
    mfxU16              m_frameOrder = 0xffff;

    std::vector<mfxI32> m_dpb;
    std::vector<mfxI32> m_refListActive[2];

    MVPPool::Type       m_mvp;
    mfxU32              m_nMvPredictors[2] = {0, 0};

    CTUCtrlPool::Type   m_ctuCtrl;
    bool                m_isGPBFrame = false;

    // Data for BRC
    FrameStatData       m_statData;
};

class LA_queue
{
public:
    LA_queue(mfxU16 la_depth = 1, const msdk_char* yuv_file = nullptr)
        : m_LookAheadDepth(la_depth)
        , m_calc_mse_from_file(yuv_file[0])
    {
        if (m_calc_mse_from_file)
        {
            m_fpYUV = fopen(yuv_file, "rb");

            if (!m_fpYUV)
                throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Failed to open YUV file");
        }
    }

    ~LA_queue()
    {
        if (m_fpYUV)
            fclose(m_fpYUV);
    }

    LA_queue(LA_queue const&)            = delete;
    LA_queue(LA_queue &&)                = default;
    LA_queue& operator=(LA_queue const&) = delete;
    LA_queue& operator=(LA_queue &&)     = default;

    void AddTask(std::shared_ptr<HevcTaskDSO> && task)
    {
        if (!task.get())
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Failed to AddTask: task pointer is null");

        if (task->m_surf)
            msdk_atomic_inc16((volatile mfxU16*)&task->m_surf->Data.Locked);

        task->m_statData.EncodedOrder = m_submittedTasks;

        if (m_calc_mse_from_file)
        {
            task->m_statData.VisualDistortion = CalcMSE(*task);
        }

        la_queue.push(std::move(task));
        ++m_submittedTasks;
    }

    bool GetTask(std::shared_ptr<HevcTaskDSO> & task)
    {
        if (la_queue.empty())
        {
            if (!m_drain)
                throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Failed to GetTask: la_queue is empty");
            else
                return false;
        }

        if (!m_drain && la_queue.size() <= m_LookAheadDepth)
        {
            return false;
        }

        task = std::move(la_queue.front());
        la_queue.pop();

        if (task.get() && task->m_surf)
            msdk_atomic_dec16((volatile mfxU16*)&task->m_surf->Data.Locked);

        return true;
    }

    void SetAllocator(MFXFrameAllocator* pAllocator)
    {
        m_pAllocator = pAllocator;
    }

    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request)
    {
        if (!request)
        {
            return MFX_ERR_NULL_PTR;
        }

        request->NumFrameMin = request->NumFrameSuggested = m_LookAheadDepth + 1;

        return MFX_ERR_NONE;
    }

    std::shared_ptr<HevcTaskDSO> & Back()
    {
        if (la_queue.empty())
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Calling Back() for empty queue");

        return la_queue.back();
    }

    void StartDrain()
    {
        m_drain = true;
    }

private:
    std::queue<std::shared_ptr<HevcTaskDSO>> la_queue;

    mfxU16 m_LookAheadDepth     = 100;
    bool   m_calc_mse_from_file = false;
    mfxU32 m_submittedTasks     = 0;
    bool   m_drain              = false;

    // for MSE calculation
    MFXFrameAllocator* m_pAllocator = nullptr;

    std::vector<mfxU8> m_YUVdata;
    FILE* m_fpYUV = nullptr;

    mfxF64 CalcMSE(HevcTaskDSO& task)
    {
        if (!m_pAllocator)
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Failed to calculate MSE: m_pAllocator is null");

        if (!task.m_surf)
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Failed to calculate MSE: task.m_surf is null");

        mfxU32 planeYsize = task.m_surf->Info.CropW * task.m_surf->Info.CropH;
        mfxU64 NthFramePlaneYshift = mfxU64(task.m_statData.DisplayOrder)*(planeYsize + (((task.m_surf->Info.CropW + 1) >> 1)*((task.m_surf->Info.CropH + 1) >> 1) << 1));

        if (fseek(m_fpYUV, NthFramePlaneYshift, SEEK_SET))
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Failed to calculate MSE: failed to fseek to current frame in YUV file");

        m_YUVdata.resize(planeYsize);

        if (fread(m_YUVdata.data(), 1, planeYsize, m_fpYUV) != planeYsize)
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Failed to calculate MSE: failed to fread current frame from YUV file");

        mfxStatus sts = m_pAllocator->Lock(m_pAllocator->pthis, task.m_surf->Data.MemId, &(task.m_surf->Data));
        if (sts != MFX_ERR_NONE)
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Failed to calculate MSE: m_pAllocator->Lock failed");

        mfxU64 l2normSq = 0;
        for (mfxU32 i = 0; i < m_YUVdata.size(); ++i)
        {
            l2normSq += std::pow(m_YUVdata[i] - task.m_surf->Data.Y[(i / task.m_surf->Info.Width)*task.m_surf->Data.Pitch + (i % task.m_surf->Info.Width)], 2);
        }

        sts = m_pAllocator->Unlock(m_pAllocator->pthis, task.m_surf->Data.MemId, &(task.m_surf->Data));
        if (sts != MFX_ERR_NONE)
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Failed to calculate MSE: m_pAllocator->Unlock failed");

        return mfxF64(l2normSq) / m_YUVdata.size();
    }
};

#endif // __HEVC_FEI_ABR_TASK__
