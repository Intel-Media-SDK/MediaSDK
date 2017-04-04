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

#include "mfx_common.h"

#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && (defined(MFX_ENABLE_H264_VIDEO_FEI_ENC) || defined(MFX_ENABLE_H264_VIDEO_FEI_PAK))
#include "fei_common.h"

using namespace MfxHwH264Encode;


template void MfxH264FEIcommon::ConfigureTaskFEI<>(
    DdiTask             & task,
    DdiTask       const & prevTask,
    MfxVideoParam const & video,
    mfxENCInput * inParams,
    mfxENCInput * outParams,
    std::map<mfxU32, mfxU32> &      frameOrder_frameNum);

template void MfxH264FEIcommon::ConfigureTaskFEI<>(
    DdiTask             & task,
    DdiTask       const & prevTask,
    MfxVideoParam const & video,
    mfxPAKInput * inParams,
    mfxPAKOutput* outParams,
    std::map<mfxU32, mfxU32> &      frameOrder_frameNum);



template <typename T, typename U>
void MfxH264FEIcommon::ConfigureTaskFEI(
    DdiTask             & task,
    DdiTask       const & prevTask,
    MfxVideoParam const & video,
    T                   * inParams,
    U                   * outParams,
    std::map<mfxU32, mfxU32> &      frameOrder_frameNum)


{
    bool FrameRepacking = prevTask.m_encOrder != 0xffffffff && task.m_frameOrder == prevTask.m_frameOrder;

    mfxExtCodingOption  const & extOpt  = GetExtBufferRef(video);
    mfxExtCodingOption2 const & extOpt2 = GetExtBufferRef(video);
    mfxExtSpsHeader     const & extSps  = GetExtBufferRef(video);

    mfxU32 const FRAME_NUM_MAX = 1 << (extSps.log2MaxFrameNumMinus4 + 4);

    mfxU8 idrPicFlag   = !!(task.GetFrameType() & MFX_FRAMETYPE_IDR);
    mfxU8 intraPicFlag = !!(task.GetFrameType() & MFX_FRAMETYPE_I);
    mfxU8 refPicFlag   = !!(task.GetFrameType() & MFX_FRAMETYPE_REF);

    if (!FrameRepacking)
    {
        mfxU8 prevIdrFrameFlag = !!(prevTask.GetFrameType() & MFX_FRAMETYPE_IDR);
        mfxU8 prevIFrameFlag   = !!(prevTask.GetFrameType() & MFX_FRAMETYPE_I);
        mfxU8 prevRefPicFlag   = !!(prevTask.GetFrameType() & MFX_FRAMETYPE_REF);

        mfxU8 frameNumIncrement = (prevRefPicFlag || prevTask.m_nalRefIdc[0]) ? 1 : 0;

        task.m_frameOrderIdr = idrPicFlag   ? task.m_frameOrder : prevTask.m_frameOrderIdr;
        task.m_frameOrderI   = intraPicFlag ? task.m_frameOrder : prevTask.m_frameOrderI;
        task.m_encOrder      = prevTask.m_encOrder + 1;
        task.m_encOrderIdr   = prevIdrFrameFlag ? prevTask.m_encOrder : prevTask.m_encOrderIdr;
        task.m_encOrderI     = prevIFrameFlag   ? prevTask.m_encOrder : prevTask.m_encOrderI;


        task.m_frameNum = idrPicFlag ? 0 : mfxU16((prevTask.m_frameNum + frameNumIncrement) % FRAME_NUM_MAX);

        task.m_picNum[1] = task.m_picNum[0] = task.m_frameNum * (task.m_fieldPicFlag + 1) + task.m_fieldPicFlag;

        task.m_idrPicId = prevTask.m_idrPicId + idrPicFlag;
    }
    else
    {
        task.m_frameOrderIdr = prevTask.m_frameOrderIdr;
        task.m_frameOrderI   = prevTask.m_frameOrderI;
        task.m_encOrder      = prevTask.m_encOrder;
        task.m_encOrderIdr   = prevTask.m_encOrderIdr;
        task.m_encOrderI     = prevTask.m_encOrderI;

        task.m_picNum[1] = task.m_picNum[0] = task.m_frameNum = prevTask.m_frameNum;

        task.m_idrPicId = prevTask.m_idrPicId;
    }

    mfxU32 ffid = task.m_fid[0];
    mfxU32 sfid = !ffid;

    task.m_pid = 0;

    task.m_insertAud[ffid] = IsOn(extOpt.AUDelimiter);
    task.m_insertAud[sfid] = IsOn(extOpt.AUDelimiter);
    task.m_insertSps[ffid] = intraPicFlag;
    task.m_insertSps[sfid] = 0;
    task.m_insertPps[ffid] = task.m_insertSps[ffid] || IsOn(extOpt2.RepeatPPS);
    task.m_insertPps[sfid] = task.m_insertSps[sfid] || IsOn(extOpt2.RepeatPPS);

    task.m_reference[ffid] = task.m_nalRefIdc[ffid] = !!(task.m_type[ffid] & MFX_FRAMETYPE_REF);
    task.m_reference[sfid] = task.m_nalRefIdc[sfid] = !!(task.m_type[sfid] & MFX_FRAMETYPE_REF);

    if (video.calcParam.tempScalabilityMode)
    {
        task.m_insertPps[ffid] = task.m_insertSps[ffid];
        task.m_insertPps[sfid] = task.m_insertSps[sfid];
        task.m_nalRefIdc[ffid] = idrPicFlag ? 3 : (task.m_reference[ffid] ? 2 : 0);
        task.m_nalRefIdc[sfid] = task.m_reference[sfid] ? 2 : 0;
    }

    task.m_cqpValue[0] = GetQpValue(video, task.m_ctrl, task.m_type[0]);
    task.m_cqpValue[1] = GetQpValue(video, task.m_ctrl, task.m_type[1]);

    frameOrder_frameNum[task.m_frameOrder] = task.m_frameNum;

    // Fill Deblocking parameters
    mfxU32 fieldMaxCount = (video.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 2;
    for (mfxU32 field = 0; field < fieldMaxCount; ++field)
    {
        mfxU32 fieldParity = (video.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_FIELD_BFF)? (1 - field) : field;

        mfxExtFeiSliceHeader * extFeiSlice = GetExtBufferFEI(outParams, field);

        for (size_t i = 0; i < extFeiSlice->NumSlice; ++i)
        {
            mfxU8 disableDeblockingIdc   = (mfxU8) extFeiSlice->Slice[i].DisableDeblockingFilterIdc;
            mfxI8 sliceAlphaC0OffsetDiv2 = (mfxI8) extFeiSlice->Slice[i].SliceAlphaC0OffsetDiv2;
            mfxI8 sliceBetaOffsetDiv2    = (mfxI8) extFeiSlice->Slice[i].SliceBetaOffsetDiv2;

            // Store per-slice values in task
            task.m_disableDeblockingIdc[fieldParity].push_back(disableDeblockingIdc);
            task.m_sliceAlphaC0OffsetDiv2[fieldParity].push_back(sliceAlphaC0OffsetDiv2);
            task.m_sliceBetaOffsetDiv2[fieldParity].push_back(sliceBetaOffsetDiv2);
        } // for (size_t i = 0; i < GetMaxNumSlices(video); i++)

        // Fill DPB
        mfxExtFeiPPS * pDataPPS = GetExtBufferFEI(outParams, field);

        std::vector<mfxFrameSurface1*> dpb_frames;

        mfxU16 indexFromPPSHeader;
        mfxU8  list_item;
        mfxU8  refParity; // 0 - top field / progressive frame; 1 - bottom field
        mfxFrameSurface1* pMfxFrame;
        mfxI32 poc_base;

        task.m_dpb[fieldParity].Resize(0);
        for (mfxU32 dpb_idx = 0; dpb_idx < 16; ++dpb_idx)
        {
            indexFromPPSHeader = pDataPPS->ReferenceFrames[dpb_idx];

            if (indexFromPPSHeader == 0xffff) break;

            pMfxFrame = inParams->L0Surface[indexFromPPSHeader];
            dpb_frames.push_back(pMfxFrame);

            DpbFrame frame;
            poc_base = 2 * ((pMfxFrame->Data.FrameOrder - task.m_frameOrderIdr) & 0x7fffffff);
            frame.m_poc          = PairI32(poc_base + (TFIELD != task.m_fid[0]), poc_base + (BFIELD != task.m_fid[0]));
            frame.m_frameOrder   = pMfxFrame->Data.FrameOrder;
            frame.m_frameNum     = frameOrder_frameNum[frame.m_frameOrder];
            frame.m_frameNumWrap = (frame.m_frameNum > task.m_frameNum) ? frame.m_frameNum - FRAME_NUM_MAX : frame.m_frameNum;

            frame.m_picNum[0] = task.m_fieldPicFlag ? 2 * frame.m_frameNumWrap + ( !fieldParity) : frame.m_frameNumWrap; // in original here field = ffid / sfid
            frame.m_picNum[1] = task.m_fieldPicFlag ? 2 * frame.m_frameNumWrap + (!!fieldParity) : frame.m_frameNumWrap;

            frame.m_refPicFlag[task.m_fid[0]] = 1;
            frame.m_refPicFlag[task.m_fid[1]] = task.m_fieldPicFlag ? (frame.m_frameOrder != task.m_frameOrder) : 1;

            frame.m_longterm  = 0;

            task.m_dpb[fieldParity].PushBack(frame);
        }

        // Get default reflists
        MfxHwH264Encode::InitRefPicList(task, fieldParity);
        ArrayU8x33 initList0 = task.m_list0[fieldParity];
        ArrayU8x33 initList1 = task.m_list1[fieldParity];

        // Fill reflists
        mfxExtFeiSliceHeader * pDataSliceHeader = extFeiSlice;
        /* Number of reference handling */
        mfxU32 maxNumRefL0 = pDataSliceHeader->Slice[0].NumRefIdxL0Active;
        mfxU32 maxNumRefL1 = pDataSliceHeader->Slice[0].NumRefIdxL1Active;

        mfxU16 indexFromSliceHeader;

        task.m_list0[fieldParity].Resize(0);
        for (mfxU32 list0_idx = 0; list0_idx < maxNumRefL0; ++list0_idx)
        {
            indexFromSliceHeader = pDataSliceHeader->Slice[0].RefL0[list0_idx].Index;
            refParity            = pDataSliceHeader->Slice[0].RefL0[list0_idx].PictureType == MFX_PICTYPE_BOTTOMFIELD;

            pMfxFrame = inParams->L0Surface[indexFromSliceHeader];

            list_item = static_cast<mfxU8>(std::distance(dpb_frames.begin(), std::find(dpb_frames.begin(), dpb_frames.end(), pMfxFrame)));

            task.m_list0[fieldParity].PushBack((refParity << 7) | list_item);
        }

        task.m_list1[fieldParity].Resize(0);
        for (mfxU32 list1_idx = 0; list1_idx < maxNumRefL1; ++list1_idx)
        {
            indexFromSliceHeader = pDataSliceHeader->Slice[0].RefL1[list1_idx].Index;
            refParity            = pDataSliceHeader->Slice[0].RefL1[list1_idx].PictureType == MFX_PICTYPE_BOTTOMFIELD;

            pMfxFrame = inParams->L0Surface[indexFromSliceHeader];

            list_item = static_cast<mfxU8>(std::distance(dpb_frames.begin(), std::find(dpb_frames.begin(), dpb_frames.end(), pMfxFrame)));

            task.m_list1[fieldParity].PushBack((refParity << 7) | list_item);
        }

        initList0.Resize(task.m_list0[fieldParity].Size());
        initList1.Resize(task.m_list1[fieldParity].Size());

        // Fill reflists modificators
        task.m_refPicList0Mod[fieldParity] = MfxHwH264Encode::CreateRefListMod(task.m_dpb[fieldParity], initList0, task.m_list0[fieldParity], task.m_viewIdx, task.m_picNum[fieldParity], true);
        task.m_refPicList1Mod[fieldParity] = MfxHwH264Encode::CreateRefListMod(task.m_dpb[fieldParity], initList1, task.m_list1[fieldParity], task.m_viewIdx, task.m_picNum[fieldParity], true);

    } // for (mfxU32 field = 0; field < fieldMaxCount; ++field)
} // void MfxH264FEIcommon::ConfigureTaskFEI

mfxStatus MfxH264FEIcommon::CheckInitExtBuffers(const MfxVideoParam & owned_video, const mfxVideoParam & passed_video)
{
    // Slice number control through CO3 is not allowed
    mfxExtCodingOption3 const * extOpt3 = GetExtBuffer(owned_video);

    MFX_CHECK(extOpt3->NumSliceI == owned_video.mfx.NumSlice, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(extOpt3->NumSliceP == owned_video.mfx.NumSlice, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(extOpt3->NumSliceB == owned_video.mfx.NumSlice, MFX_ERR_INVALID_VIDEO_PARAM);


    // Internal headers should be updated before CreateAccelerationService call
    const mfxExtFeiSPS* pDataSPS = GetExtBuffer(passed_video);
    if (pDataSPS)
    {
        // Check correctness of provided PPS parameters
        MFX_CHECK(pDataSPS->Log2MaxPicOrderCntLsb >= 4 && pDataSPS->Log2MaxPicOrderCntLsb <= 16, MFX_ERR_INVALID_VIDEO_PARAM);

        // Update internal SPS if FEI SPS buffer attached
        mfxExtSpsHeader * extSps = GetExtBuffer(owned_video);

        extSps->seqParameterSetId           = pDataSPS->SPSId;
        extSps->picOrderCntType             = pDataSPS->PicOrderCntType;
        extSps->log2MaxPicOrderCntLsbMinus4 = pDataSPS->Log2MaxPicOrderCntLsb - 4;
    }

    const mfxExtFeiPPS* pDataPPS = GetExtBuffer(passed_video);
    if (pDataPPS)
    {
        // Update internal PPS if FEI PPS buffer attached
        mfxExtPpsHeader* extPps = GetExtBuffer(owned_video);
        MFX_CHECK(extPps, MFX_ERR_INVALID_VIDEO_PARAM);

        extPps->seqParameterSetId              = pDataPPS->SPSId;
        extPps->picParameterSetId              = pDataPPS->PPSId;

        extPps->picInitQpMinus26               = pDataPPS->PicInitQP - 26;
        extPps->numRefIdxL0DefaultActiveMinus1 = (std::max)(pDataPPS->NumRefIdxL0Active, mfxU16(1)) - 1;;
        extPps->numRefIdxL1DefaultActiveMinus1 = (std::max)(pDataPPS->NumRefIdxL1Active, mfxU16(1)) - 1;;

        extPps->chromaQpIndexOffset            = pDataPPS->ChromaQPIndexOffset;
        extPps->secondChromaQpIndexOffset      = pDataPPS->SecondChromaQPIndexOffset;
        extPps->transform8x8ModeFlag           = pDataPPS->Transform8x8ModeFlag;
    }

    // mfxExtFeiSliceHeader is useless at init stage for ENC and PAK (unlike the FEI ENCODE)
    const mfxExtFeiSliceHeader * pDataSliceHeader = GetExtBuffer(passed_video);
    MFX_CHECK(pDataSliceHeader == NULL, MFX_ERR_INVALID_VIDEO_PARAM);

    return MFX_ERR_NONE;
}

template mfxStatus MfxH264FEIcommon::CheckRuntimeExtBuffers<>(mfxENCInput* input, mfxENCOutput* output, const MfxVideoParam & owned_video);
template mfxStatus MfxH264FEIcommon::CheckRuntimeExtBuffers<>(mfxPAKOutput* input, mfxPAKOutput* output, const MfxVideoParam & owned_video);

template <typename T, typename U>
mfxStatus MfxH264FEIcommon::CheckRuntimeExtBuffers(T* input, U* output, const MfxVideoParam & owned_video)
{
    // SPS at runtime is not allowed
    mfxExtFeiSPS* extFeiSPSinRuntime = GetExtBufferFEI(input, 0);
    MFX_CHECK(!extFeiSPSinRuntime, MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxU32 fieldMaxCount = (owned_video.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 2;
    for (mfxU32 field = 0; field < fieldMaxCount; field++)
    {
        // SliceHeader is required to pass reference lists
        mfxExtFeiSliceHeader * extFeiSliceInRintime = GetExtBufferFEI(input, field);
        MFX_CHECK(extFeiSliceInRintime,                                           MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(extFeiSliceInRintime->Slice,                                    MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(extFeiSliceInRintime->NumSlice,                                 MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(extFeiSliceInRintime->NumSlice <= GetMaxNumSlices(owned_video), MFX_ERR_UNDEFINED_BEHAVIOR);
        /*
        if (task.m_numSlice[feiFieldId]){
            MFX_CHECK(extFeiSliceInRintime->NumSlice == task.m_numSlice[feiFieldId], MFX_ERR_UNDEFINED_BEHAVIOR);
        }
        else*/
        {
            MFX_CHECK(extFeiSliceInRintime->NumSlice == owned_video.mfx.NumSlice, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        for (mfxU32 i = 0; i < extFeiSliceInRintime->NumSlice; ++i)
        {
            MFX_CHECK(extFeiSliceInRintime->Slice[i].DisableDeblockingFilterIdc <=  2, MFX_ERR_INVALID_VIDEO_PARAM);
            MFX_CHECK(extFeiSliceInRintime->Slice[i].SliceAlphaC0OffsetDiv2     <=  6, MFX_ERR_INVALID_VIDEO_PARAM);
            MFX_CHECK(extFeiSliceInRintime->Slice[i].SliceAlphaC0OffsetDiv2     >= -6, MFX_ERR_INVALID_VIDEO_PARAM);
            MFX_CHECK(extFeiSliceInRintime->Slice[i].SliceBetaOffsetDiv2        <=  6, MFX_ERR_INVALID_VIDEO_PARAM);
            MFX_CHECK(extFeiSliceInRintime->Slice[i].SliceBetaOffsetDiv2        >= -6, MFX_ERR_INVALID_VIDEO_PARAM);
        }

        mfxExtFeiPPS* extFeiPPSinRuntime = GetExtBufferFEI(input, field);
        MFX_CHECK(extFeiPPSinRuntime, MFX_ERR_UNDEFINED_BEHAVIOR);

        // Check that parameters from previous init kept unchanged
        {
            mfxExtPpsHeader* extPps = GetExtBuffer(owned_video);

            MFX_CHECK(extPps->seqParameterSetId              == extFeiPPSinRuntime->SPSId,                                        MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(extPps->picParameterSetId              == extFeiPPSinRuntime->PPSId,                                        MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(extPps->picInitQpMinus26               == extFeiPPSinRuntime->PicInitQP - 26,                               MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(extPps->numRefIdxL0DefaultActiveMinus1 == (std::max)(extFeiPPSinRuntime->NumRefIdxL0Active, mfxU16(1)) - 1, MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(extPps->numRefIdxL1DefaultActiveMinus1 == (std::max)(extFeiPPSinRuntime->NumRefIdxL1Active, mfxU16(1)) - 1, MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(extPps->chromaQpIndexOffset            == extFeiPPSinRuntime->ChromaQPIndexOffset,                          MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(extPps->secondChromaQpIndexOffset      == extFeiPPSinRuntime->SecondChromaQPIndexOffset,                    MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(extPps->transform8x8ModeFlag           == extFeiPPSinRuntime->Transform8x8ModeFlag,                         MFX_ERR_UNDEFINED_BEHAVIOR);
        }

    }


    return MFX_ERR_NONE;
}


#endif // defined(MFX_VA) && defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_ENC)
