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

#ifndef _MFX_VP8_DECODE_HW_H_
#define _MFX_VP8_DECODE_HW_H_

#include "mfx_common.h"

#if defined(MFX_ENABLE_VP8_VIDEO_DECODE_HW)

#include "mfx_common_int.h"
#include "mfx_umc_alloc_wrapper.h"
#include "mfx_critical_error_handler.h"
#include "umc_frame_data.h"
#include "umc_media_data.h"
#include "umc_va_base.h"

#include "mfx_task.h"

#include "umc_mutex.h"

#include "mfx_vp8_dec_decode_vp8_defs.h"
#include "mfx_vp8_dec_decode_common.h"

class MFX_VP8_BoolDecoder
{
private:
    uint32_t m_range;
    uint32_t m_value;
    int32_t m_bitcount;
    uint32_t m_pos;
    uint8_t *m_input;
    int32_t m_input_size;

    static const int range_normalization_shift[64];

    int decode_bit(int probability)
    {

        uint32_t bit = 0;
        uint32_t split;
        uint32_t bigsplit;
        uint32_t count = this->m_bitcount;
        uint32_t range = this->m_range;
        uint32_t value = this->m_value;

        split = 1 +  (((range - 1) * probability) >> 8);
        bigsplit = (split << 24);

        range = split;
        if(value >= bigsplit)
        {
           range = this->m_range - split;
           value = value - bigsplit;
           bit = 1;
        }

        if(range >= 0x80)
        {
            this->m_value = value;
            this->m_range = range;
            return bit;
        }
        else
        {
            do
            {
                range += range;
                value += value;

                if (!--count)
                {
                    count = 8;
                    value |= static_cast<uint32_t>(this->m_input[this->m_pos]);
                    this->m_pos++;
                }
             }
             while(range < 0x80);
        }
        this->m_bitcount = count;
        this->m_value = value;
        this->m_range = range;
        return bit;

    }

public:
    MFX_VP8_BoolDecoder() :
        m_range(0),
        m_value(0),
        m_bitcount(0),
        m_pos(0),
        m_input(0),
        m_input_size(0)
    {}

    MFX_VP8_BoolDecoder(uint8_t *pBitStream, int32_t dataSize)
    {
        init(pBitStream, dataSize);
    }

    void init(uint8_t *pBitStream, int32_t dataSize)
    {
        dataSize = std::min(dataSize, 2);
        m_range = 255;
        m_bitcount = 8;
        m_pos = 0;
        m_value = (pBitStream[0] << 24) + (pBitStream[1] << 16) +
                  (pBitStream[2] << 8) + pBitStream[3];
        m_pos += 4;
        m_input     = pBitStream;
        m_input_size = dataSize;
    }

    uint32_t decode(int bits = 1, int prob = 128)
    {
        uint32_t z = 0;
        int bit;
        for (bit = bits - 1; bit >= 0;bit--)
        {
            z |= (decode_bit(prob) << bit);
        }
        return z;
    }

    uint8_t * input()
    {
        return &m_input[m_pos];
    }

    uint32_t pos() const
    {
        return m_pos;
    }

    int32_t bitcount() const
    {
        return m_bitcount;
    }

    uint32_t range() const
    {
        return m_range;
    }

    uint32_t value() const
    {
        return m_value;
    }
};

class VideoDECODEVP8_HW : public VideoDECODE, public MfxCriticalErrorHandler
{
public:

    VideoDECODEVP8_HW(VideoCORE *pCore, mfxStatus *sts);
    ~VideoDECODEVP8_HW();

    static mfxStatus Query(VideoCORE *pCore, mfxVideoParam *pIn, mfxVideoParam *pOut);
    static mfxStatus QueryIOSurf(VideoCORE *pCore, mfxVideoParam *pPar, mfxFrameAllocRequest *pRequest);

    virtual mfxStatus Init(mfxVideoParam *pPar);
    virtual mfxStatus Reset(mfxVideoParam *pPar);
    virtual mfxStatus Close();

    virtual mfxTaskThreadingPolicy GetThreadingPolicy();
    virtual mfxStatus GetVideoParam(mfxVideoParam *pPar);
    virtual mfxStatus GetDecodeStat(mfxDecodeStat *pStat);

    virtual mfxStatus DecodeFrameCheck(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out);
    virtual mfxStatus DecodeFrameCheck(mfxBitstream *pBs, mfxFrameSurface1 *pSurfaceWork, mfxFrameSurface1 **ppSurfaceOut, MFX_ENTRY_POINT *pEntryPoint);
    virtual mfxStatus DecodeFrame(mfxBitstream *pBs, mfxFrameSurface1 *pSurfaceWork, mfxFrameSurface1 *pSurfaceOut);

    virtual mfxStatus GetUserData(mfxU8 *pUserData, mfxU32 *pSize, mfxU64 *pTimeStamp);
    virtual mfxStatus GetPayload(mfxU64 *pTimeStamp, mfxPayload *pPayload);
    virtual mfxStatus SetSkipMode(mfxSkipMode mode);

private:

    mfxFrameSurface1 * GetOriginalSurface(mfxFrameSurface1 *);
    mfxStatus GetOutputSurface(mfxFrameSurface1 **, mfxFrameSurface1 *, UMC::FrameMemID);

    mfxStatus ConstructFrame(mfxBitstream *, mfxBitstream *, VP8DecodeCommon::IVF_FRAME&);
    mfxStatus PreDecodeFrame(mfxBitstream *, mfxFrameSurface1 *);

    mfxStatus DecodeFrameHeader(mfxBitstream *p_bistream);
    void UpdateSegmentation(MFX_VP8_BoolDecoder &);
    void UpdateLoopFilterDeltas(MFX_VP8_BoolDecoder &);
    void DecodeInitDequantization(MFX_VP8_BoolDecoder &);
    mfxStatus PackHeaders(mfxBitstream *p_bistream);

    static bool CheckHardwareSupport(VideoCORE *p_core, mfxVideoParam *p_par);

    UMC::FrameMemID GetMemIdToUnlock();

    mfxStatus GetFrame(UMC::MediaData* in, UMC::FrameData** out);

    friend mfxStatus MFX_CDECL VP8DECODERoutine(void *p_state, void *pp_param, mfxU32 thread_number, mfxU32);

    struct VP8DECODERoutineData
    {
        VideoDECODEVP8_HW* decoder;
        mfxFrameSurface1* surface_work;
        UMC::FrameMemID memId;
    };

    struct sFrameInfo
    {
        UMC::FrameType frameType;
        mfxU16 currIndex;
        mfxU16 goldIndex;
        mfxU16 altrefIndex;
        mfxU16 lastrefIndex;
        UMC::FrameMemID memId;
    };

    bool                    m_is_initialized;
    bool                    m_is_opaque_memory;
    VideoCORE*              m_p_core;
    eMFXPlatform            m_platform;

    mfxVideoParamWrapper    m_on_init_video_params,
                            m_video_params;
    mfxU32                  m_init_w,
                            m_init_h;
    mfxF64                  m_in_framerate;
    mfxU16                  m_frameOrder;

    mfxBitstream            m_bs;
    VP8Defs::vp8_FrameInfo  m_frame_info;
    unsigned                m_CodedCoeffTokenPartition;
    bool                    m_firstFrame;

    VP8Defs::vp8_RefreshInfo         m_refresh_info;
    VP8Defs::vp8_FrameProbabilities  m_frameProbs;
    VP8Defs::vp8_FrameProbabilities  m_frameProbs_saved;
    VP8Defs::vp8_QuantInfo           m_quantInfo;
    MFX_VP8_BoolDecoder     m_boolDecoder[VP8Defs::VP8_MAX_NUMBER_OF_PARTITIONS];

    mfxU16 gold_indx;
    mfxU16 altref_indx;
    mfxU16 lastrefIndex;

    std::vector<sFrameInfo>      m_frames;
    std::vector<UMC::FrameMemID> m_memIdReadyToFree;

    mfxFrameAllocResponse   m_response;
    mfxDecodeStat           m_stat;
    mfxFrameAllocRequest    m_request;

    std::unique_ptr<mfx_UMC_FrameAllocator> m_p_frame_allocator;
    UMC::VideoAccelerator *m_p_video_accelerator;

    UMC::Mutex              m_mGuard;
};

#endif // _MFX_VP8_DECODE_HW_H_
#endif // MFX_ENABLE_VP8_VIDEO_DECODE_HW && MFX_VA
/* EOF */
