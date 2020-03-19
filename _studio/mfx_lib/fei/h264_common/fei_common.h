// Copyright (c) 2017-2019 Intel Corporation
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

#ifndef _MFX_H264_FEI_COMMON_H_
#define _MFX_H264_FEI_COMMON_H_

#include "mfx_common.h"

#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && (defined(MFX_ENABLE_H264_VIDEO_FEI_ENC) || defined(MFX_ENABLE_H264_VIDEO_FEI_PAK))

#include "mfxfei.h"

#include "mfxpak.h"
#include "mfxenc.h"

#include "mfx_session.h"
#include "mfx_task.h"
#include "libmfx_core.h"
#include "libmfx_core_hw.h"
#include "libmfx_core_interface.h"
#include "mfx_ext_buffers.h"

#include "mfx_h264_encode_hw.h"
#include "mfx_enc_common.h"
#include "mfx_h264_enc_common_hw.h"
#include "mfx_h264_encode_hw_utils.h"

#include "mfx_h264_encode_vaapi.h"

#include <memory>
#include <list>
#include <vector>
#include <algorithm>
#include <numeric>

namespace MfxH264FEIcommon
{
#if MFX_VERSION >= 1023

    template <typename T>
    void ConfigureTaskFEI(
        MfxHwH264Encode::DdiTask             & task,
        MfxHwH264Encode::DdiTask       const & prevTask,
        MfxHwH264Encode::MfxVideoParam       & video,
        T * inParams);
#else
    template <typename T, typename U>
    void ConfigureTaskFEI(
        MfxHwH264Encode::DdiTask             & task,
        MfxHwH264Encode::DdiTask       const & prevTask,
        MfxHwH264Encode::MfxVideoParam       & video,
        T * inParams,
        U * outParams,
        std::map<mfxU32, mfxU32> &      frameOrder_frameNum);
#endif //MFX_VERSION >= 1023

    template <typename T>
    bool FirstFieldProcessingDone(T* inParams, const MfxHwH264Encode::DdiTask & task);


    mfxStatus Change_DPB(
        MfxHwH264Encode::ArrayDpbFrame & dpb,
        mfxMemId                 const * mids,
        std::vector<mfxU32>      const & fo);

    mfxStatus CheckInitExtBuffers(const MfxHwH264Encode::MfxVideoParam & owned_video, const mfxVideoParam & passed_video);

    template <typename T, typename U>
    mfxStatus CheckRuntimeExtBuffers(T* input, U* output, const MfxHwH264Encode::MfxVideoParam & owned_video, const MfxHwH264Encode::DdiTask & task);

    bool IsRunTimeInputExtBufferIdSupported(const MfxHwH264Encode::MfxVideoParam & owned_video, mfxU32 id);
    bool IsRunTimeOutputExtBufferIdSupported(const MfxHwH264Encode::MfxVideoParam & owned_video, mfxU32 id);

    bool IsRunTimeExtBufferPairRequired(const MfxHwH264Encode::MfxVideoParam & owned_video, mfxU32 id);

    bool CheckSliceHeaderReferenceList(mfxExtFeiSliceHeader::mfxSlice::mfxSliceRef * ref, mfxU16 num_idx_active);

#if MFX_VERSION >= 1023
    template <typename T, typename U>
    mfxStatus CheckDPBpairCorrectness(T* input, U* output, mfxExtFeiPPS* extFeiPPSinRuntime, const MfxHwH264Encode::MfxVideoParam & owned_video);

    mfxStatus CheckOneDPBCorrectness(mfxExtFeiPPS::mfxExtFeiPpsDPB* DPB, const MfxHwH264Encode::MfxVideoParam & owned_video, bool is_IDR_field = false);
#endif // MFX_VERSION >= 1023
};

#endif
#endif // _MFX_H264_FEI_COMMON_H_
