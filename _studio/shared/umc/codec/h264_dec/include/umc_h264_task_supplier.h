// Copyright (c) 2017-2020 Intel Corporation
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
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#ifndef __UMC_H264_TASK_SUPPLIER_H
#define __UMC_H264_TASK_SUPPLIER_H

#include "umc_va_base.h"

#include <vector>
#include <list>
#include "umc_h264_dec_defs_dec.h"
#include "umc_media_data_ex.h"
#include "umc_h264_heap.h"
#include "umc_h264_slice_decoding.h"
#include "umc_h264_frame_info.h"
#include "umc_h264_frame_list.h"

#include "umc_h264_segment_decoder_base.h"
#include "umc_h264_headers.h"

#include "umc_frame_allocator.h"

#include "umc_h264_au_splitter.h"


namespace UMC
{
class TaskBroker;

class H264DBPList;
class H264DecoderFrame;
class MediaData;

class BaseCodecParams;
class H264SegmentDecoderMultiThreaded;
class TaskBrokerSingleThreadDXVA;

class MemoryAllocator;
struct H264IntraTypesProp;
class LocalResources;

enum
{
    BASE_VIEW                   = 0,
    INVALID_VIEW_ID             = 0xffffffff
};

class ErrorStatus
{
public:

    bool IsExistHeadersError() const
    {
        return isSPSError || isPPSError;
    }

protected:
    int32_t isSPSError;
    int32_t isPPSError;

    ErrorStatus()
    {
        Reset();
    }

    void Reset()
    {
        isSPSError = 0;
        isPPSError = 0;
    }
};

/****************************************************************************************************/
// DPBOutput class routine
/****************************************************************************************************/
class DPBOutput
{
public:
    DPBOutput();

    void Reset(bool disableDelayFeature = false);

    bool IsUseSEIDelayOutputValue() const;
    bool IsUsePicOrderCnt() const;

    bool IsUseDelayOutputValue() const;

    int32_t GetDPBOutputDelay(UMC_H264_DECODER::H264SEIPayLoad * payload);

    void OnNewSps(UMC_H264_DECODER::H264SeqParamSet * sps);

private:

    struct
    {
        uint32_t use_payload_sei_delay : 1;
        uint32_t use_pic_order_cnt_type : 1;
    } m_isUseFlags;
};
/****************************************************************************************************/
// Skipping class routine
/****************************************************************************************************/
class Skipping
{
public:
    Skipping();
    virtual ~Skipping();

    void PermanentDisableDeblocking(bool disable);
    bool IsShouldSkipDeblocking(H264DecoderFrame * pFrame, int32_t field);
    bool IsShouldSkipFrame(H264DecoderFrame * pFrame, int32_t field);
    void ChangeVideoDecodingSpeed(int32_t& num);
    void Reset();

    struct SkipInfo
    {
        bool isDeblockingTurnedOff;
        int32_t numberOfSkippedFrames;
    };

    SkipInfo GetSkipInfo() const;

private:

    int32_t m_VideoDecodingSpeed;
    int32_t m_SkipCycle;
    int32_t m_ModSkipCycle;
    int32_t m_PermanentTurnOffDeblocking;
    int32_t m_SkipFlag;

    int32_t m_NumberOfSkippedFrames;
};

/****************************************************************************************************/
// POCDecoder
/****************************************************************************************************/
class POCDecoder
{
public:

    POCDecoder();

    virtual ~POCDecoder();
    virtual void DecodePictureOrderCount(const H264Slice *slice, int32_t frame_num);

    int32_t DetectFrameNumGap(const H264Slice *slice, bool ignore_gaps_allowed_flag = false);

    // Set the POCs when frame gap is processed
    void DecodePictureOrderCountFrameGap(H264DecoderFrame *pFrame,
                                         const UMC_H264_DECODER::H264SliceHeader *pSliceHeader,
                                         int32_t frameNum);
    // Set the POCs when fake frames are imagined
    void DecodePictureOrderCountFakeFrames(H264DecoderFrame *pFrame,
                                           const UMC_H264_DECODER::H264SliceHeader *pSliceHeader);
    // Set the POCs when the frame is initialized
    void DecodePictureOrderCountInitFrame(H264DecoderFrame *pFrame,
                                          int32_t fieldIdx);

    void Reset(int32_t IDRFrameNum = 0);

protected:
    int32_t                     m_PrevFrameRefNum;
    int32_t                     m_FrameNum;
    int32_t                     m_PicOrderCnt;
    int32_t                     m_PicOrderCntMsb;
    int32_t                     m_PicOrderCntLsb;
    int32_t                     m_FrameNumOffset;
    uint32_t                     m_TopFieldPOC;
    uint32_t                     m_BottomFieldPOC;
};

/****************************************************************************************************/
// TaskSupplier
/****************************************************************************************************/
class SEI_Storer
{
public:

    struct SEI_Message
    {
        size_t             msg_size;
        size_t             offset;
        uint8_t            * data;
        double             timestamp;
        SEI_TYPE           type;
        int32_t             isUsed;
        int32_t             auID;
        int32_t             inputID;
        H264DecoderFrame * frame;

        SEI_Message()
            : msg_size(0)
            , offset(0)
            , data(0)
            , timestamp(0)
            , type(SEI_RESERVED)
            , isUsed(0)
            , auID(0)
            , inputID(0)
            , frame(0)
        {
        }
    };

    SEI_Storer();

    virtual ~SEI_Storer();

    void Init();

    void Close();

    void Reset();

    void SetTimestamp(H264DecoderFrame * frame);

    SEI_Message* AddMessage(UMC::MediaDataEx *nalUnit, SEI_TYPE type, int32_t auIndex);

    const SEI_Message * GetPayloadMessage();

    void SetFrame(H264DecoderFrame * frame, int32_t auIndex);
    void SetAUID(int32_t auIndex);

private:

    enum
    {
        MAX_BUFFERED_SIZE = 16 * 1024, // 16 kb
        START_ELEMENTS = 10,
        MAX_ELEMENTS = 128
    };

    std::vector<uint8_t>  m_data;
    std::vector<SEI_Message> m_payloads;

    size_t m_offset;
    int32_t m_lastUsed;
};

/****************************************************************************************************/
// ViewItem class routine
/****************************************************************************************************/
struct ViewItem
{
    // Default constructor
    ViewItem(void);

    // Copy constructor
    ViewItem(const ViewItem &src);

    // Copy assignment
    ViewItem &operator =(const ViewItem &src);

    ~ViewItem();

    // Initialize th view, allocate resources
    Status Init(uint32_t view_id);

    // Close the view and release all resources
    void Close(void);

    // Reset the view and reset all resource
    void Reset(void);

    // Reset the size of DPB for particular view item
    void SetDPBSize(UMC_H264_DECODER::H264SeqParamSet *pSps, uint8_t & level_idc);

    H264DBPList *GetDPBList(int32_t dIdRev = 0)
    {
        return pDPB[dIdRev].get();
    }

    POCDecoder *GetPOCDecoder(int32_t dIdRev = 0)
    {
        return pPOCDec[dIdRev].get();
    }

    // Keep view ID to identify initialized views
    int32_t viewId;

    // Pointer to the view's DPB
    mutable std::unique_ptr<H264DBPList> pDPB[MAX_NUM_LAYERS];

    // Pointer to the POC processing object
    mutable std::unique_ptr<POCDecoder> pPOCDec[MAX_NUM_LAYERS];

    bool m_isDisplayable;

    // Maximum number frames used semultaneously
    uint32_t maxDecFrameBuffering;

    // Maximum possible long term reference index
    int32_t MaxLongTermFrameIdx[MAX_NUM_LAYERS];

    // Pointer to the frame being processed
    H264DecoderFrame *pCurFrame;

    double localFrameTime;

    // SPS.VUI.(bitstream_restriction_flag == 1).max_num_reorder_frames
    uint32_t maxNumReorderFrames;
};

typedef std::list<ViewItem> ViewList;

/****************************************************************************************************/
// MVC extension class routine
/****************************************************************************************************/
class MVC_Extension
{
public:
    MVC_Extension();
    virtual ~MVC_Extension();

    Status Init();
    virtual void Close();
    virtual void Reset();

    virtual bool IsShouldSkipSlice(H264Slice * slice);

    virtual bool IsShouldSkipSlice(UMC_H264_DECODER::H264NalExtension *nal_ext);

    uint32_t GetLevelIDC() const;

    void SetTemporalId(uint32_t temporalId);

    Status SetViewList(const std::vector<uint32_t> & targetView, const std::vector<uint32_t> & dependencyList);

    ViewItem *FindView(int32_t viewId);

    ViewItem &GetView(int32_t viewId = -1);

    ViewItem &GetViewByNumber(int32_t viewNum);

    void MoveViewToHead(int32_t view_id);

    int32_t GetViewCount() const;

    bool IncreaseCurrentView();

    int32_t GetBaseViewId() const;

    bool IsExtension() const; // MVC or SVC

protected:
    uint32_t m_temporal_id;
    uint32_t m_priority_id;

    uint8_t  m_level_idc;
    uint32_t m_currentDisplayView;

    uint32_t m_currentView;

    enum DecodingMode
    {
        UNKNOWN_DECODING_MODE,
        AVC_DECODING_MODE,
        MVC_DECODING_MODE,
        SVC_DECODING_MODE
    };

    DecodingMode m_decodingMode;

    typedef std::list<uint32_t> ViewIDsList;
    ViewIDsList  m_viewIDsList;

    // Array of pointers to views and their components
    ViewList m_views;

    void ChooseLevelIdc(const UMC_H264_DECODER::H264SeqParamSetMVCExtension * extension, uint8_t baseViewLevelIDC, uint8_t extensionLevelIdc);
    void AnalyzeDependencies(const UMC_H264_DECODER::H264SeqParamSetMVCExtension * extension);
    Status AllocateView(int32_t view_id);
    ViewItem & AllocateAndInitializeView(H264Slice * slice);
};

/****************************************************************************************************/
// SVC extension class routine
/****************************************************************************************************/
class SVC_Extension : public MVC_Extension
{
public:

    SVC_Extension();
    virtual ~SVC_Extension();

    virtual void Reset();
    virtual void Close();

    virtual bool IsShouldSkipSlice(H264Slice * slice);
    virtual bool IsShouldSkipSlice(UMC_H264_DECODER::H264NalExtension *nal_ext);
    void ChooseLevelIdc(const UMC_H264_DECODER::H264SeqParamSetSVCExtension * extension, uint8_t baseViewLevelIDC, uint8_t extensionLevelIdc);
    void SetSVCTargetLayer(uint32_t dependency_id, uint32_t quality_id, uint32_t temporal_id);

protected:
    uint32_t m_dependency_id;
    uint32_t m_quality_id;
};

/****************************************************************************************************/
// DecReferencePictureMarking
/****************************************************************************************************/
class DecReferencePictureMarking
{
public:

    DecReferencePictureMarking();

    Status UpdateRefPicMarking(ViewItem &view, H264DecoderFrame * pFrame, H264Slice * pSlice, int32_t field_index);

    void SlideWindow(ViewItem &view, H264Slice * pSlice, int32_t field_index);

    void Reset();

    void CheckSEIRepetition(ViewItem &view, H264DecoderFrame * frame);
    Status CheckSEIRepetition(ViewItem &view, UMC_H264_DECODER::H264SEIPayLoad *payload);

    uint32_t  GetDPBError() const;

protected:

    uint32_t  m_isDPBErrorFound;

    enum ChangeItemFlags
    {
        SHORT_TERM      = 0x00000001,
        LONG_TERM       = 0x00000002,

        FULL_FRAME      = 0x00000010,
        BOTTOM_FIELD    = 0x00000020,
        TOP_FIELD       = 0x00000040,

        SET_REFERENCE   = 0x00000100,
        UNSET_REFERENCE = 0x00000200
    };

    struct DPBChangeItem
    {
        struct
        {
            uint8_t isShortTerm   : 1; // short or long
            uint8_t isSet         : 1; // set or free
            uint8_t isFrame       : 1; // full frame
            uint8_t isBottom      : 1; // if doesn't full frame is it top or bottom?
        } m_type;

        H264DecoderFrame *  m_pRefFrame;
        H264DecoderFrame *  m_pCurrentFrame;
    };

    typedef std::list<DPBChangeItem> DPBCommandsList;
    DPBCommandsList m_commandsList;
    int32_t   m_frameCount;

    DPBChangeItem* AddItem(DPBCommandsList & list, H264DecoderFrame * currentFrame, H264DecoderFrame * refFrame, uint32_t flags);
    DPBChangeItem* AddItemAndRun(H264DecoderFrame * currentFrame, H264DecoderFrame * refFrame, uint32_t flags);

    void Undo(const DPBChangeItem * item);
    void Redo(const DPBChangeItem * item);
    void MakeChange(bool isUndo, const DPBChangeItem * item);

    void Undo(const H264DecoderFrame * frame);
    void Redo(const H264DecoderFrame * frame);
    void MakeChange(bool isUndo, const H264DecoderFrame * frame);
    void Remove(H264DecoderFrame * frame);
    void RemoveOld();

#ifdef _DEBUG
    void DebugPrintItems();
#endif

    bool CheckUseless(DPBChangeItem * item);
    void ResetError();
};

/****************************************************************************************************/
// TaskSupplier
/****************************************************************************************************/
class TaskSupplier : public Skipping, public AU_Splitter, public SVC_Extension, public DecReferencePictureMarking, public DPBOutput,
        public ErrorStatus
{
    friend class TaskBrokerSingleThreadDXVA;

public:

    TaskSupplier();
    virtual ~TaskSupplier();

    virtual Status Init(VideoDecoderParams *pInit);

    virtual Status PreInit(VideoDecoderParams *pInit);

    virtual void Reset();
    virtual void Close();

    Status GetInfo(VideoDecoderParams *lpInfo);

    virtual Status AddSource(MediaData * pSource);


#if (MFX_VERSION >= 1025)
    Status ProcessNalUnit(NalUnit *nalUnit, mfxExtDecodeErrorReport *pDecodeErrorReport);
#else
    Status ProcessNalUnit(NalUnit *nalUnit);
#endif

    void SetMemoryAllocator(MemoryAllocator *pMemoryAllocator)
    {
        m_pMemoryAllocator = pMemoryAllocator;
    }

    MemoryAllocator * GetMemoryAllocator() const { return m_pMemoryAllocator; };

    void SetFrameAllocator(FrameAllocator *pFrameAllocator)
    {
        m_pFrameAllocator = pFrameAllocator;
    }

    virtual H264DecoderFrame *GetFrameToDisplayInternal(bool force);

    Status GetUserData(MediaData * pUD);

    H264DBPList *GetDPBList(uint32_t viewId, int32_t dIdRev)
    {
        ViewItem *pView = FindView(viewId);
        if (!pView)
            return NULL;
        return pView->GetDPBList(dIdRev);
    }

    TaskBroker * GetTaskBroker()
    {
        return m_pTaskBroker;
    }

    virtual Status RunDecoding();
    virtual H264DecoderFrame * FindSurface(FrameMemID id);

    void PostProcessDisplayFrame(H264DecoderFrame *pFrame);

    virtual void AfterErrorRestore();

    SEI_Storer * GetSEIStorer() const { return m_sei_messages;}

    Headers * GetHeaders() { return &m_Headers;}

    inline
    const UMC_H264_DECODER::H264SeqParamSet *GetCurrentSequence(void) const
    {
        return m_Headers.m_SeqParams.GetCurrentHeader();
    }

    virtual H264Slice * DecodeSliceHeader(NalUnit *nalUnit);
    virtual H264Slice * CreateSlice();

    H264_Heap_Objects * GetObjHeap()
    {
        return &m_ObjHeap;
    }

    // Initialize MB ordering for the picture using slice groups as
    // defined in the picture parameter set.
    virtual void SetMBMap(const H264Slice * slice, H264DecoderFrame *frame, LocalResources * localRes);

protected:

    virtual void CreateTaskBroker() = 0;

    void AddSliceToFrame(H264DecoderFrame *pFrame, H264Slice *pSlice);

    // Initialize just allocated frame with slice parameters
    virtual void InitFreeFrame(H264DecoderFrame *pFrame, const H264Slice *pSlice);
    // Initialize frame's counter and corresponding parameters
    virtual
    void InitFrameCounter(H264DecoderFrame *pFrame, const H264Slice *pSlice);

    Status AddSlice(H264Slice * pSlice, bool force);
    virtual Status CompleteFrame(H264DecoderFrame * pFrame, int32_t m_field_index);
    virtual void OnFullFrame(H264DecoderFrame * pFrame);
    virtual bool ProcessNonPairedField(H264DecoderFrame * pFrame) = 0;

    void DPBSanitize(H264DecoderFrame * pDPBHead, const H264DecoderFrame * pFrame);
    void DBPUpdate(H264DecoderFrame * pFrame, int32_t field);

    virtual void AddFakeReferenceFrame(H264Slice * pSlice);

    virtual Status AddOneFrame(MediaData * pSource);

    virtual Status AllocateFrameData(H264DecoderFrame * pFrame) = 0;

    virtual Status DecodeHeaders(NalUnit *nalUnit);
    virtual Status DecodeSEI(NalUnit *nalUnit);

    Status ProcessFrameNumGap(H264Slice *slice, int32_t field, int32_t did, int32_t maxDid);

    // Obtain free frame from queue
    virtual H264DecoderFrame *GetFreeFrame(const H264Slice *pSlice = NULL) = 0;

    Status CompleteDecodedFrames(H264DecoderFrame ** decoded);

    H264DecoderFrame *GetAnyFrameToDisplay(bool force);

    void PreventDPBFullness();

    Status AllocateNewFrame(const H264Slice *pSlice, H264DecoderFrame **frame);

    Status InitializeLayers(AccessUnit *accessUnit, H264DecoderFrame * pFrame, int32_t field);
    void ApplyPayloadsToFrame(H264DecoderFrame * frame, H264Slice *slice, SeiPayloadArray * payloads);

    H264_Heap_Objects           m_ObjHeap;

    H264SegmentDecoderBase **m_pSegmentDecoder;
    uint32_t m_iThreadNum;

    double      m_local_delta_frame_time;
    bool        m_use_external_framerate;

    H264Slice * m_pLastSlice;

    H264DecoderFrame *m_pLastDisplayed;

    MemoryAllocator *m_pMemoryAllocator;
    FrameAllocator  *m_pFrameAllocator;

    // Keep track of which parameter set is in use.
    bool              m_WaitForIDR;

    uint32_t            m_DPBSizeEx;
    int32_t            m_frameOrder;

    TaskBroker * m_pTaskBroker;

    VideoDecoderParams     m_initializationParams;

    int32_t m_UIDFrameCounter;

    UMC_H264_DECODER::H264SEIPayLoad m_UserData;
    SEI_Storer *m_sei_messages;

    AccessUnit m_accessUnit;

    bool m_isInitialized;

    Mutex m_mGuard;

    bool m_ignoreLevelConstrain;

private:
    TaskSupplier & operator = (TaskSupplier &)
    {
        return *this;

    } // TaskSupplier & operator = (TaskSupplier &)

};

// can increase level_idc to hold num_ref_frames
inline int32_t CalculateDPBSize(uint8_t & level_idc, int32_t width, int32_t height, uint32_t num_ref_frames)
{
    uint32_t dpbSize;
    auto constexpr INTERNAL_MAX_LEVEL = H264VideoDecoderParams::H264_LEVEL_MAX + 1;

    num_ref_frames = std::min(16u, num_ref_frames);

    for (;;)
    {
        uint32_t MaxDPBMbs;
        // MaxDPB, per Table A-1, Level Limits
        switch (level_idc)
        {
        case H264VideoDecoderParams::H264_LEVEL_1:
            MaxDPBMbs = 396;
            break;
        case H264VideoDecoderParams::H264_LEVEL_11:
            MaxDPBMbs = 900;
            break;
        case H264VideoDecoderParams::H264_LEVEL_12:
        case H264VideoDecoderParams::H264_LEVEL_13:
        case H264VideoDecoderParams::H264_LEVEL_2:
            MaxDPBMbs = 2376;
            break;
        case H264VideoDecoderParams::H264_LEVEL_21:
            MaxDPBMbs = 4752;
            break;
        case H264VideoDecoderParams::H264_LEVEL_22:
        case H264VideoDecoderParams::H264_LEVEL_3:
            MaxDPBMbs = 8100;
            break;
        case H264VideoDecoderParams::H264_LEVEL_31:
            MaxDPBMbs = 18000;
            break;
        case H264VideoDecoderParams::H264_LEVEL_32:
            MaxDPBMbs = 20480;
            break;
        case H264VideoDecoderParams::H264_LEVEL_4:
        case H264VideoDecoderParams::H264_LEVEL_41:
            MaxDPBMbs = 32768;
            break;
        case H264VideoDecoderParams::H264_LEVEL_42:
            MaxDPBMbs = 34816;
            break;
        case H264VideoDecoderParams::H264_LEVEL_5:
            MaxDPBMbs = 110400;
            break;
        case H264VideoDecoderParams::H264_LEVEL_51:
        case H264VideoDecoderParams::H264_LEVEL_52:
            MaxDPBMbs = 184320;
            break;
#if (MFX_VERSION >= 1035)
        case H264VideoDecoderParams::H264_LEVEL_6:
        case H264VideoDecoderParams::H264_LEVEL_61:
        case H264VideoDecoderParams::H264_LEVEL_62:
            MaxDPBMbs = 696320;
            break;
#endif
        default:
            // Relax resolution constraints up to 4K when
            // level_idc reaches 5.1+.  That is,
            // use value 696320 which is from level 6+ for
            // the calculation of the DPB size when level_idc
            // reaches 5.1+ but dpbSize is still less
            // than num_ref_frames
            MaxDPBMbs = 696320;
            break;
        }

        if (width == 0 || height == 0)
        {
            throw h264_exception(UMC_ERR_INVALID_PARAMS);
        }

        uint32_t dpbLevel = MaxDPBMbs*256 / (width * height);
        dpbSize = std::min(16u, dpbLevel);

        if (num_ref_frames <= dpbSize || level_idc == INTERNAL_MAX_LEVEL)
            break;

        switch (level_idc) // increase level_idc
        {
        case H264VideoDecoderParams::H264_LEVEL_1:
            level_idc = H264VideoDecoderParams::H264_LEVEL_11;
            break;
        case H264VideoDecoderParams::H264_LEVEL_11:
            level_idc = H264VideoDecoderParams::H264_LEVEL_12;
            break;
        case H264VideoDecoderParams::H264_LEVEL_12:
        case H264VideoDecoderParams::H264_LEVEL_13:
        case H264VideoDecoderParams::H264_LEVEL_2:
            level_idc = H264VideoDecoderParams::H264_LEVEL_21;
            break;
        case H264VideoDecoderParams::H264_LEVEL_21:
            level_idc = H264VideoDecoderParams::H264_LEVEL_22;
            break;
        case H264VideoDecoderParams::H264_LEVEL_22:
        case H264VideoDecoderParams::H264_LEVEL_3:
            level_idc = H264VideoDecoderParams::H264_LEVEL_31;
            break;
        case H264VideoDecoderParams::H264_LEVEL_31:
            level_idc = H264VideoDecoderParams::H264_LEVEL_32;
            break;
        case H264VideoDecoderParams::H264_LEVEL_32:
            level_idc = H264VideoDecoderParams::H264_LEVEL_4;
            break;
        case H264VideoDecoderParams::H264_LEVEL_4:
        case H264VideoDecoderParams::H264_LEVEL_41:
        case H264VideoDecoderParams::H264_LEVEL_42:
            level_idc = H264VideoDecoderParams::H264_LEVEL_5;
            break;
        case H264VideoDecoderParams::H264_LEVEL_5:
            level_idc = H264VideoDecoderParams::H264_LEVEL_51;
            break;

        // Set level_idc to INTERNAL_MAX_LEVEL so that MaxDPBMbs = 696320
        // can be used to calculate the DPB size.
        case H264VideoDecoderParams::H264_LEVEL_51:
        case H264VideoDecoderParams::H264_LEVEL_52:
#if (MFX_VERSION >= 1035)
            level_idc = H264VideoDecoderParams::H264_LEVEL_6;
#else
            level_idc = INTERNAL_MAX_LEVEL;
#endif
            break;

#if (MFX_VERSION >= 1035)
        case H264VideoDecoderParams::H264_LEVEL_6:
        case H264VideoDecoderParams::H264_LEVEL_61:
        case H264VideoDecoderParams::H264_LEVEL_62:
            level_idc = INTERNAL_MAX_LEVEL;
            break;
#endif

        default:
            throw h264_exception(UMC_ERR_FAILED);
        }
    }

    // Restore level_idc to H264_LEVEL_MAX
    if (level_idc == INTERNAL_MAX_LEVEL)
      level_idc = H264VideoDecoderParams::H264_LEVEL_MAX;

    return dpbSize;
}


inline H264DBPList *GetDPB(ViewList &views, int32_t viewId, int32_t dIdRev = 0)
{
    ViewList::iterator iter = views.begin();
    ViewList::iterator iter_end = views.end();

    for (; iter != iter_end; ++iter)
    {
        if (viewId == (*iter).viewId)
        {
            return (*iter).GetDPBList(dIdRev);
        }
    }

    throw h264_exception(UMC_ERR_FAILED);
} // H264DBPList *GetDPB(ViewList &views)

inline uint32_t GetVOIdx(const UMC_H264_DECODER::H264SeqParamSetMVCExtension *pSeqParamSetMvc, uint32_t viewId)
{
    auto it = std::find_if(pSeqParamSetMvc->viewInfo.begin(), pSeqParamSetMvc->viewInfo.end(),
                           [viewId](const UMC_H264_DECODER::H264ViewRefInfo & item){ return item.view_id == viewId; });

    return (pSeqParamSetMvc->viewInfo.end() != it) ? (it - pSeqParamSetMvc->viewInfo.begin()) : 0;

} // uint32_t GetVOIdx(const H264SeqParamSetMVCExtension *pSeqParamSetMvc, uint32_t viewId)

inline uint32_t GetInterViewFrameRefs(ViewList &views, int32_t viewId, int32_t auIndex, uint32_t bottomFieldFlag)
{
    ViewList::iterator iter = views.begin();
    ViewList::iterator iter_end = views.end();
    uint32_t numInterViewRefs = 0;

    for (; iter != iter_end; ++iter)
    {
        if (iter->viewId == viewId)
            break;

        // take into account DPBs with lower view ID
        H264DecoderFrame *pInterViewRef = iter->GetDPBList()->findInterViewRef(auIndex, bottomFieldFlag);
        if (pInterViewRef)
        {
            ++numInterViewRefs;
        }
    }

    return numInterViewRefs;

}

} // namespace UMC

#endif // __UMC_H264_TASK_SUPPLIER_H
#endif // MFX_ENABLE_H264_VIDEO_DECODE
