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

#include "umc_defs.h"
#if defined (MFX_ENABLE_H264_VIDEO_DECODE)

#include "umc_h264_va_supplier.h"
#include "umc_h264_frame_list.h"
#include "umc_h264_nal_spl.h"
#include "umc_h264_bitstream_headers.h"

#include "umc_h264_dec_defs_dec.h"
#include "umc_h264_segment_decoder_base.h"

#include "umc_h264_task_broker.h"
#include "umc_structures.h"

#include "umc_h264_dec_debug.h"

#include "mfx_umc_alloc_wrapper.h"
#include "mfx_common_int.h"
#include "mfx_ext_buffers.h"
#include "umc_h264_notify.h"

#include "mfxfei.h"

#include "umc_va_linux_protected.h"

namespace UMC
{

void LazyCopier::Reset()
{
    m_slices.clear();
}

void LazyCopier::Add(H264Slice * slice)
{
    if (!slice)
        return;

    m_slices.push_back(slice);
}

void LazyCopier::Remove(H264Slice * slice)
{
    m_slices.remove(slice);
}

void LazyCopier::Remove(H264DecoderFrameInfo * info)
{
    if (!info)
        return;

    uint32_t count = info->GetSliceCount();
    for (uint32_t i = 0; i < count; i++)
    {
        H264Slice * slice = info->GetSlice(i);
        Remove(slice);
    }
}

void LazyCopier::CopyAll()
{
    SlicesList::iterator iter = m_slices.begin();
    SlicesList::iterator iter_end = m_slices.end();
    for (; iter != iter_end; ++iter)
    {
        H264Slice * slice = *iter;
        slice->m_pSource.MoveToInternalBuffer();

        // update bs ptr !!!
        H264HeadersBitstream *pBitstream = slice->GetBitStream();

        uint32_t *pbsBase, *pbs;
        uint32_t size, bitOffset;

        pBitstream->GetOrg(&pbsBase, &size);
        pBitstream->GetState(&pbs, &bitOffset);

        pBitstream->Reset(slice->m_pSource.GetPointer(), bitOffset, (uint32_t)slice->m_pSource.GetDataSize());
        pBitstream->SetState((uint32_t*)slice->m_pSource.GetPointer() + (pbs - pbsBase), bitOffset);
    }

    m_slices.clear();
}

VATaskSupplier::VATaskSupplier()
    : m_bufferedFrameNumber(0)
{
}

Status VATaskSupplier::Init(VideoDecoderParams *pInit)
{
    SetVideoHardwareAccelerator(pInit->pVideoAccelerator);
    m_pMemoryAllocator = pInit->lpMemoryAllocator;

    pInit->numThreads = 1;

    Status umsRes = TaskSupplier::Init(pInit);
    if (umsRes != UMC_OK)
        return umsRes;

    m_iThreadNum = 1;

    DXVASupport<VATaskSupplier>::Init();
    if (m_va)
    {
        static_cast<TaskBrokerSingleThreadDXVA*>(m_pTaskBroker)->DXVAStatusReportingMode(m_va->IsUseStatusReport());
    }

    H264VideoDecoderParams *initH264 = DynamicCast<H264VideoDecoderParams> (pInit);
    m_DPBSizeEx = m_iThreadNum + (initH264 ? initH264->m_bufferedFrames : 0);

    if (m_va &&
        m_va->GetProtectedVA() &&
        IS_PROTECTION_CENC(m_va->GetProtectedVA()->GetProtected()))
    {
        m_DPBSizeEx += 2;
    }

    m_sei_messages = new SEI_Storer();
    m_sei_messages->Init();
    m_lazyCopier.Reset();

    return UMC_OK;
}

void VATaskSupplier::CreateTaskBroker()
{
    m_pTaskBroker = new TaskBrokerSingleThreadDXVA(this);

    for (uint32_t i = 0; i < m_iThreadNum; i += 1)
    {
        m_pSegmentDecoder[i] = new H264_DXVA_SegmentDecoder(this);
    }
}

void VATaskSupplier::SetBufferedFramesNumber(uint32_t buffered)
{
    m_DPBSizeEx += buffered;
    m_bufferedFrameNumber = buffered;

    if (buffered)
        DPBOutput::Reset(true);
}

H264DecoderFrame * VATaskSupplier::GetFrameToDisplayInternal(bool force)
{
    ViewItem &view = GetViewByNumber(BASE_VIEW);
    view.maxDecFrameBuffering += m_bufferedFrameNumber;

    H264DecoderFrame * frame = MFXTaskSupplier::GetFrameToDisplayInternal(force);

    view.maxDecFrameBuffering -= m_bufferedFrameNumber;
    return frame;
}

void VATaskSupplier::Close()
{
    m_lazyCopier.Reset();
    MFXTaskSupplier::Close();
}

void VATaskSupplier::Reset()
{
    m_lazyCopier.Reset();

    if (m_pTaskBroker)
        m_pTaskBroker->Reset();

    MFXTaskSupplier::Reset();
    DXVASupport<VATaskSupplier>::Reset();
}

void VATaskSupplier::AfterErrorRestore()
{
    m_lazyCopier.Reset();
    MFXTaskSupplier::AfterErrorRestore();
}

Status VATaskSupplier::DecodeHeaders(NalUnit *nalUnit)
{
    Status sts = MFXTaskSupplier::DecodeHeaders(nalUnit);

    if (sts != UMC_OK && sts != UMC_WRN_REPOSITION_INPROGRESS)
        return sts;

    uint32_t nal_unit_type = nalUnit->GetNalUnitType();
    if (nal_unit_type == NAL_UT_SPS && m_firstVideoParams.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE &&
        isMVCProfile(m_firstVideoParams.mfx.CodecProfile) && m_va)
    {
        UMC_H264_DECODER::H264SeqParamSet * currSPS = m_Headers.m_SeqParams.GetCurrentHeader();
        if (currSPS && !currSPS->frame_mbs_only_flag)
        {
            return UMC_NTF_NEW_RESOLUTION;
        }
    }

    return sts;
}

H264DecoderFrame *VATaskSupplier::GetFreeFrame(const H264Slice * pSlice)
{
    AutomaticUMCMutex guard(m_mGuard);
    ViewItem &view = GetView(pSlice->GetSliceHeader()->nal_ext.mvc.view_id);

    H264DecoderFrame *pFrame = 0;

    if (view.GetDPBList(0)->countAllFrames() >= view.maxDecFrameBuffering + m_DPBSizeEx)
        pFrame = view.GetDPBList(0)->GetDisposable();

    VM_ASSERT(!pFrame || pFrame->GetRefCounter() == 0);

    // Did we find one?
    if (NULL == pFrame)
    {
        if (view.GetDPBList(0)->countAllFrames() >= view.maxDecFrameBuffering + m_DPBSizeEx)
        {
            DEBUG_PRINT((VM_STRING("Fail to find disposable frame\n")));
            return 0;
        }

        // Didn't find one. Let's try to insert a new one
        pFrame = new H264DecoderFrame(m_pMemoryAllocator, &m_ObjHeap);
        if (NULL == pFrame)
            return 0;

        view.GetDPBList(0)->append(pFrame);
    }

    DEBUG_PRINT((VM_STRING("Cleanup frame POC - %d, %d, field - %d, uid - %d, frame_num - %d, viewId - %d\n"),
        pFrame->m_PicOrderCnt[0], pFrame->m_PicOrderCnt[1],
        pFrame->GetNumberByParity(pSlice->GetSliceHeader()->bottom_field_flag),
        pFrame->m_UID, pFrame->m_FrameNum, pFrame->m_viewId)
    );

    DecReferencePictureMarking::Remove(pFrame);
    pFrame->Reset();
    InitFreeFrame(pFrame, pSlice);
    return pFrame;
}

Status VATaskSupplier::CompleteFrame(H264DecoderFrame * pFrame, int32_t field)
{
    if (!pFrame)
        return UMC_OK;

    H264DecoderFrameInfo * slicesInfo = pFrame->GetAU(field);

    if (slicesInfo->GetStatus() > H264DecoderFrameInfo::STATUS_NOT_FILLED)
        return UMC_OK;

    DEBUG_PRINT((VM_STRING("Complete frame POC - (%d,%d) type - %d, picStruct - %d, field - %d, count - %d, m_uid - %d, IDR - %d, viewId - %d\n"),
        pFrame->m_PicOrderCnt[0], pFrame->m_PicOrderCnt[1],
        pFrame->m_FrameType,
        pFrame->m_displayPictureStruct,
        field,
        pFrame->GetAU(field)->GetSliceCount(),
        pFrame->m_UID, pFrame->m_bIDRFlag, pFrame->m_viewId
    ));

    // skipping algorithm
    {
        if (((slicesInfo->IsField() && field) || !slicesInfo->IsField()) &&
            IsShouldSkipFrame(pFrame, field))
        {
            if (slicesInfo->IsField())
            {
                pFrame->GetAU(0)->SetStatus(H264DecoderFrameInfo::STATUS_COMPLETED);
                pFrame->GetAU(1)->SetStatus(H264DecoderFrameInfo::STATUS_COMPLETED);
            }
            else
            {
                pFrame->GetAU(0)->SetStatus(H264DecoderFrameInfo::STATUS_COMPLETED);
            }


            pFrame->SetisShortTermRef(false, 0);
            pFrame->SetisShortTermRef(false, 1);
            pFrame->SetisLongTermRef(false, 0);
            pFrame->SetisLongTermRef(false, 1);
            pFrame->SetSkipped(true);
            pFrame->OnDecodingCompleted();
            return UMC_OK;
        }
        else
        {
            if (IsShouldSkipDeblocking(pFrame, field))
            {
                pFrame->GetAU(field)->SkipDeblocking();
            }
        }
    }

    if (slicesInfo->GetAnySlice()->IsSliceGroups())
    {
        throw h264_exception(UMC_ERR_UNSUPPORTED);
    }

    DecodePicture(pFrame, field);
    DBPUpdate(pFrame, field);

    m_lazyCopier.Remove(slicesInfo);
    slicesInfo->SetStatus(H264DecoderFrameInfo::STATUS_FILLED);

    return UMC_OK;
}

void VATaskSupplier::InitFrameCounter(H264DecoderFrame * pFrame, const H264Slice *pSlice)
{
    TaskSupplier::InitFrameCounter(pFrame, pSlice);
}

Status VATaskSupplier::AddSource(MediaData * pSource)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VATaskSupplier::AddSource");

    if (!pSource)
        return MFXTaskSupplier::AddSource(pSource);

    notifier0<LazyCopier> copy_slice_data(&m_lazyCopier, &LazyCopier::CopyAll);
    return MFXTaskSupplier::AddSource(pSource);
}

Status VATaskSupplier::AllocateFrameData(H264DecoderFrame * pFrame)
{
    mfxSize dimensions = pFrame->lumaSize();
    VideoDataInfo info;
    info.Init(dimensions.width, dimensions.height, pFrame->GetColorFormat(), pFrame->m_bpp);

    FrameMemID frmMID;
    Status sts = m_pFrameAllocator->Alloc(&frmMID, &info, 0);

    if (sts == UMC_ERR_ALLOC)
        return UMC_ERR_ALLOC;

    if (sts != UMC_OK)
        throw h264_exception(UMC_ERR_ALLOC);

    FrameData frmData;
    frmData.Init(&info, frmMID, m_pFrameAllocator);

    mfx_UMC_FrameAllocator* mfx_alloc = 
        DynamicCast<mfx_UMC_FrameAllocator>(m_pFrameAllocator);
    if (mfx_alloc)
    {
        mfxFrameSurface1* surface = 
            mfx_alloc->GetSurfaceByIndex(frmMID);
        if (!surface)
            throw h264_exception(UMC_ERR_ALLOC);

        mfxExtBuffer* extbuf = 
            GetExtendedBuffer(surface->Data.ExtParam, surface->Data.NumExtParam, MFX_EXTBUFF_FEI_DEC_STREAM_OUT);
        if (extbuf)
            frmData.SetAuxInfo(extbuf, extbuf->BufferSz, extbuf->BufferId);

    }

    pFrame->allocate(&frmData, &info);

    pFrame->IncrementReference();
    m_UIDFrameCounter++;
    pFrame->m_UID = m_UIDFrameCounter;
    pFrame->m_index = frmMID;

    return UMC_OK;
}

H264Slice * VATaskSupplier::DecodeSliceHeader(NalUnit *nalUnit)
{
    size_t dataSize = nalUnit->GetDataSize();
    nalUnit->SetDataSize(std::min<size_t>(1024, dataSize));

    H264Slice * slice = TaskSupplier::DecodeSliceHeader(nalUnit);

    nalUnit->SetDataSize(dataSize);

    if (!slice)
        return 0;

    if (nalUnit->IsUsedExternalMem())
    {
        slice->m_pSource.SetData(nalUnit);
        m_lazyCopier.Add(slice);
    }
    else
    {
        slice->m_pSource.Allocate(nalUnit->GetDataSize() + DEFAULT_NU_TAIL_SIZE);
        MFX_INTERNAL_CPY(slice->m_pSource.GetPointer(), nalUnit->GetDataPointer(), (uint32_t)nalUnit->GetDataSize());
        memset(slice->m_pSource.GetPointer() + nalUnit->GetDataSize(), DEFAULT_NU_TAIL_VALUE, DEFAULT_NU_TAIL_SIZE);
        slice->m_pSource.SetDataSize(nalUnit->GetDataSize());
        slice->m_pSource.SetTime(nalUnit->GetTime());
    }

    uint32_t* pbs;
    uint32_t bitOffset;

    slice->GetBitStream()->GetState(&pbs, &bitOffset);

    size_t bytes = slice->GetBitStream()->BytesDecodedRoundOff();

    slice->GetBitStream()->Reset(slice->m_pSource.GetPointer(), bitOffset,
        (uint32_t)slice->m_pSource.GetDataSize());
    slice->GetBitStream()->SetState((uint32_t*)(slice->m_pSource.GetPointer() + bytes), bitOffset);

    return slice;
}

// walk over all view's  DPB and find an index free index.
// i.e. index not used by any frame in any view
// returns free index or -1 if no free index found
int32_t VATaskSupplier::GetFreeFrameIndex()
{
    for (int32_t i = 0; i < 127; i++)
    {
        ViewList::iterator iter = m_views.begin();
        ViewList::iterator iter_end = m_views.end();
        H264DecoderFrame *pFrm = NULL;

        // run over the list and try to find the corresponding view
        for (; iter != iter_end; ++iter)
        {
            ViewItem & item = *iter;
            H264DBPList *pDPBList = item.GetDPBList();
            pFrm = pDPBList->head();

            // walk over the list
            while(pFrm != NULL && pFrm->m_index != i)
            {
                pFrm = pFrm->future();
            }

            if (pFrm != NULL)
            {
                // go next index
                // no need to check next View
                break;
            }
        }

        if (pFrm == NULL)
        {
            // we wall over each frame in all Views
            // this index is free
            return i;
        }
    }

    VM_ASSERT(false);
    return -1;
}

} // namespace UMC

#endif // MFX_ENABLE_H264_VIDEO_DECODE
