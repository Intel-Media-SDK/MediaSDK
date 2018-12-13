// Copyright (c) 2017-2018 Intel Corporation
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

#ifndef __MFX_MPEG2_DECODE_H__
#define __MFX_MPEG2_DECODE_H__


#include "mfx_common.h"
#ifdef MFX_ENABLE_MPEG2_VIDEO_DECODE
#include "mfxvideo++int.h"
#include "mfx_mpeg2_dec_common.h"
#include "umc_mpeg2_dec_base.h"
#include "mfx_umc_alloc_wrapper.h"
#include "mfx_task.h"
#include "mfx_critical_error_handler.h"

#include <memory>
#include <list>

#if defined (MFX_VA_LINUX)
#include "umc_va_base.h"
#endif

//#define MFX_PROFILE_MPEG1 8
#define DPB 10
#define NUM_FRAMES DPB
#define NUM_REST_BYTES 4
#define NUM_FRAMES_BUFFERED 0
#define END_FRAME true
#define NO_END_FRAME false
#define REMOVE_LAST_FRAME false
#define REMOVE_LAST_2_FRAMES true
#define NO_SURFACE_TO_UNLOCK -1
#define NO_TASK_TO_UNLOCK -1

enum
{
    MINIMAL_BITSTREAM_LENGTH    = 4,
};

enum FrameType
{
    I_PICTURE               = 1,
    P_PICTURE               = 2,
    B_PICTURE               = 3
};


typedef enum {
    STATUS_REPORT_OPERATION_SUCCEEDED   = 0,
    STATUS_REPORT_MINOR_PROBLEM         = 1,
    STATUS_REPORT_SIGNIFICANT_PROBLEM   = 2,
    STATUS_REPORT_SEVERE_PROBLEM        = 3,
    STATUS_REPORT_SEVERE_PROBLEM_OTHER  = 4,

} DXVAStatusReportFeedback;

typedef struct _DisplayBuffer
{
    int mid;
    int index;
    unsigned long long timestamp;
}DisplayBuffer;

typedef struct _MParam
{
    int32_t m_curr_thread_idx;
    int32_t m_thread_completed;
    int32_t task_num;
    int32_t NumThreads;
    bool m_isDecodedOrder;

    int32_t  m_frame_curr;
    unsigned long long last_timestamp;
    int32_t curr_index;
    int32_t prev_index;
    int32_t next_index;
    int32_t last_frame_count;
    int display_index;
    bool IsSWImpl;

    UMC::MediaData *in;
    mfxFrameSurface1 *surface_out;
    mfxFrameSurface1 *surface_work;
    mfx_UMC_FrameAllocator *m_FrameAllocator;
    mfxVideoParam m_vPar;
    UMC::FrameMemID *mid;
    mfxBitstream *m_frame;
    bool *m_frame_in_use;

    bool m_isSoftwareBuffer;
    bool m_isCorrupted;
} MParam;


class VideoDECODEMPEG2InternalBase : public MfxCriticalErrorHandler
{
public:
    VideoDECODEMPEG2InternalBase();

    virtual ~VideoDECODEMPEG2InternalBase();

    virtual mfxStatus AllocFrames(mfxVideoParam *par);
    static mfxStatus QueryIOSurfInternal(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request);

    virtual mfxStatus Reset(mfxVideoParam *par);

    virtual mfxStatus Init(mfxVideoParam *par, VideoCORE * core);
    virtual mfxStatus Close();

    virtual mfxStatus GetVideoParam(mfxVideoParam *par);

    virtual mfxStatus DecodeFrameCheck(mfxBitstream *bs,
                               mfxFrameSurface1 *surface_work,
                               mfxFrameSurface1 **surface_out,
                               MFX_ENTRY_POINT *pEntryPoint) = 0;

    virtual mfxStatus TaskRoutine(void *pParam) = 0;

    virtual mfxStatus ConstructFrame(mfxBitstream *bs, mfxFrameSurface1 *surface_work);
    mfxStatus ConstructFrame(mfxBitstream *in, mfxBitstream *out, mfxFrameSurface1 *surface_work);
    mfxStatus ConstructFrameImpl(mfxBitstream *in, mfxBitstream *out, mfxFrameSurface1 *surface_work);

    mfxStatus GetOutputSurface(mfxFrameSurface1 **surface_out, mfxFrameSurface1 *surface_work, UMC::FrameMemID index);
    mfxStatus SetOutputSurfaceParams(mfxFrameSurface1 *surface, int display_index);
    mfxStatus SetSurfacePicStruct(mfxFrameSurface1 *surface, int display_index);
    mfxStatus UpdateOutputSurfaceParamsFromWorkSurface(mfxFrameSurface1 *outputSurface, int display_index);
    mfxFrameSurface1 *GetOriginalSurface(mfxFrameSurface1 *pSurface);
    mfxStatus UpdateWorkSurfaceParams(int task_num);

    virtual mfxStatus CompleteTasks(void *pParam) = 0;

    bool IsOpaqueMemory() const { return m_isOpaqueMemory; }

public:
    int32_t display_frame_count;
    int32_t cashed_frame_count;
    int32_t skipped_frame_count;
    int32_t dec_frame_count;
    int32_t dec_field_count;
    int32_t last_frame_count;

    int32_t m_NumThreads;
    uint32_t maxNumFrameBuffered;

    bool m_isFrameRateFromInit;
    bool m_reset_done;
    bool m_isDecodedOrder;

    UMC::MediaData m_in[2*DPB];
    mfxBitstream m_frame[NUM_FRAMES];
    bool m_frame_in_use[NUM_FRAMES];

    mfxF64 m_time[NUM_FRAMES];
    UMC::FrameMemID mid[DPB];

    UMC::Mutex m_guard;

    int32_t m_InitW;
    int32_t m_InitH;
    int32_t m_InitPicStruct;

    std::unique_ptr<UMC::MPEG2VideoDecoderBase> m_implUmc;

    bool m_isSWBuf;
    mfxVideoParam m_vPar;
    UMC::VideoDecoderParams m_vdPar;

    mfx_UMC_FrameAllocator *m_FrameAllocator;

    mfxFrameAllocRequest allocRequest;
    mfxFrameAllocResponse allocResponse;
    mfxFrameAllocResponse m_opaqueResponse;

protected:

    VideoCORE *m_pCore;

    bool m_resizing;
    bool m_isSWDecoder;

    bool m_isOpaqueMemory;

    struct FcState
    {
        enum { NONE = 0, TOP = 1, BOT = 2, FRAME = 3 };
        mfxU8 picStart : 1;
        mfxU8 picHeader : 2;
    } m_fcState;

    bool m_found_SH;
    bool m_found_RA_Frame;
    bool m_first_SH;
    bool m_new_bs;

    uint8_t m_last_bytes[NUM_REST_BYTES];
    int32_t m_frame_curr;
    int32_t m_frame_free;
    bool m_frame_constructed;

    MParam m_task_param[2*DPB];
    int32_t m_task_num;
    int32_t m_prev_task_num;

    int32_t prev_index;
    int32_t next_index;
    int32_t curr_index;
    int32_t display_index;
    int32_t display_order;
    unsigned long long last_timestamp;
    mfxF64 last_good_timestamp;
    mfxU16 m_fieldsInCurrFrame; // first field or new frame if 3 or 0; 1 - only top etc

    int32_t m_Protected;

    void ResetFcState(FcState& state) { state.picStart = state.picHeader = 0; }
    mfxStatus UpdateCurrVideoParams(mfxFrameSurface1 *surface_work, int task_num);
    bool VerifyPictureBits(mfxBitstream* currPicBs, const mfxU8* head, const mfxU8* tail);

    //get index to read input data:
    bool SetCurr_m_frame()
    {
        m_frame_curr++;
        if(m_frame_curr == NUM_FRAMES)
            m_frame_curr = 0;
        if(!m_frame_in_use[m_frame_curr])
            return false;
        return true;
    }
    //set index to write input data:
    bool SetFree_m_frame()
    {
        int prev = m_frame_free;
        m_frame_free++;
        if(m_frame_free == NUM_FRAMES)
            m_frame_free = 0;
        if(m_frame_in_use[m_frame_free])
        {
            m_frame_free = prev;
            return false;
        }
        return true;
    }
};

#if defined (MFX_VA_LINUX)
namespace UMC
{
    class MPEG2VideoDecoderHW;
};
class VideoDECODEMPEG2Internal_HW : public VideoDECODEMPEG2InternalBase
{
public:
    VideoDECODEMPEG2Internal_HW();

    virtual mfxStatus DecodeFrameCheck(mfxBitstream *bs,
                               mfxFrameSurface1 *surface_work,
                               mfxFrameSurface1 **surface_out,
                               MFX_ENTRY_POINT *pEntryPoint);

    virtual mfxStatus TaskRoutine(void *pParam);
    virtual mfxStatus Init(mfxVideoParam *par, VideoCORE * core);
    virtual mfxStatus Reset(mfxVideoParam *par);
    virtual mfxStatus Close();

    virtual mfxStatus CompleteTasks(void *pParam);

    virtual mfxStatus GetVideoParam(mfxVideoParam *par);

protected:


    UMC::MPEG2VideoDecoderHW * m_implUmcHW;


    virtual mfxStatus ConstructFrame(mfxBitstream *bs, mfxFrameSurface1 *surface_work);
    mfxStatus PerformStatusCheck(void *pParam);
    mfxStatus GetStatusReport(int32_t current_index, UMC::FrameMemID surface_id);
    mfxStatus GetStatusReportByIndex(int32_t current_index, mfxU32 currIdx);
    virtual mfxStatus AllocFrames(mfxVideoParam *par);
    mfxStatus RestoreDecoder(int32_t frame_buffer_num, UMC::FrameMemID mem_id_to_unlock, int32_t task_num_to_unlock, bool end_frame, bool remove_2frames, int decrease_dec_field_count);
    void TranslateCorruptionFlag(int32_t disp_index, mfxFrameSurface1 * surface_work);
};

#endif // #if defined (MFX_VA_WIN) || defined (MFX_VA_LINUX)


class VideoDECODEMPEG2 : public VideoDECODE
{

public:
    std::unique_ptr<VideoDECODEMPEG2InternalBase> internalImpl;

    static mfxStatus Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out);
    static mfxStatus QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request);
    static mfxStatus DecodeHeader(VideoCORE *core, mfxBitstream *bs, mfxVideoParam *par);

    VideoDECODEMPEG2(VideoCORE *core, mfxStatus *sts);
    virtual ~VideoDECODEMPEG2();

    mfxStatus Init(mfxVideoParam *par);
    virtual mfxStatus Reset(mfxVideoParam *par);
    virtual mfxStatus Close();
    virtual mfxTaskThreadingPolicy GetThreadingPolicy(void) { return MFX_TASK_THREADING_INTRA; }

    virtual mfxStatus GetVideoParam(mfxVideoParam *par);


    virtual mfxStatus DecodeFrame(mfxBitstream * /*bs*/, mfxFrameSurface1 * /*surface_work*/, mfxFrameSurface1 * /*surface_out*/)
        { return MFX_ERR_NONE; };


    virtual mfxStatus GetDecodeStat(mfxDecodeStat *stat);
    virtual mfxStatus DecodeFrameCheck(mfxBitstream *, mfxFrameSurface1 *, mfxFrameSurface1 **)
    {
        return MFX_ERR_NONE;
    };
    virtual mfxStatus DecodeFrameCheck(mfxBitstream *bs,
                               mfxFrameSurface1 *surface_work,
                               mfxFrameSurface1 **surface_out,
                               MFX_ENTRY_POINT *pEntryPoint);

    mfxStatus CheckFrameData(const mfxFrameSurface1 *pSurface);

    virtual mfxStatus GetUserData(mfxU8 *ud, mfxU32 *sz, mfxU64 *ts,mfxU16 bufsize);
    virtual mfxStatus GetPayload(mfxU64 *ts, mfxPayload *payload);
    virtual mfxStatus SetSkipMode(mfxSkipMode mode);

protected:
    VideoCORE *m_pCore;

    bool m_isInitialized;
    bool m_isSWImpl;


    enum SkipLevel
    {
        SKIP_NONE = 0,
        SKIP_B,
        SKIP_PB,
        SKIP_ALL
    };
    int32_t m_SkipLevel;
};

#ifdef _MSVC_LANG
#pragma warning(default: 4324)
#endif

#endif //MFX_ENABLE_MPEG2_VIDEO_DECODE
#endif //__MFX_MPEG2_DECODE_H__
