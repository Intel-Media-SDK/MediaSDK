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

#include "umc_defs.h"
#if defined (UMC_ENABLE_MJPEG_VIDEO_DECODER)

#include "umc_mjpeg_mfx_decode_hw.h"

#if defined(UMC_VA)

#include <string.h>
#include "vm_debug.h"
#include "membuffin.h"
#include <algorithm>
#include <limits.h>

namespace UMC
{

MJPEGVideoDecoderMFX_HW::MJPEGVideoDecoderMFX_HW(void) : m_convertInfo()
{
    m_fourCC = 0;
    m_statusReportFeedbackCounter = 0;
    m_va = 0;
} // ctor

MJPEGVideoDecoderMFX_HW::~MJPEGVideoDecoderMFX_HW(void)
{
    Close();
} // dtor

Status MJPEGVideoDecoderMFX_HW::Init(BaseCodecParams* lpInit)
{
    VideoDecoderParams* pDecoderParams = DynamicCast<VideoDecoderParams>(lpInit);

    if(0 == pDecoderParams)
        return UMC_ERR_NULL_PTR;

    Status status = Close();
    if(UMC_OK != status)
        return UMC_ERR_INIT;

    m_DecoderParams  = *pDecoderParams;

    m_IsInit       = true;
    m_interleaved  = false;
    m_interleavedScan = false;
    m_frameSampling = 0;
    m_statusReportFeedbackCounter = 1;
    m_fourCC       = 0;

    m_va = pDecoderParams->pVideoAccelerator;

    m_decoder.reset(new CJPEGDecoderBase());

    m_decBase = m_decoder.get();

    return UMC_OK;
} // MJPEGVideoDecoderMFX_HW::Init()

Status MJPEGVideoDecoderMFX_HW::Reset(void)
{
    m_statusReportFeedbackCounter = 1;
    {
        AutomaticUMCMutex guard(m_guard);

        m_cachedReadyTaskIndex.clear();
        m_cachedCorruptedTaskIndex.clear();
        m_submittedTaskIndex.clear();
    }

    return MJPEGVideoDecoderBaseMFX::Reset();
} // MJPEGVideoDecoderMFX_HW::Reset()

Status MJPEGVideoDecoderMFX_HW::Close(void)
{
    {
        AutomaticUMCMutex guard(m_guard);

        m_cachedReadyTaskIndex.clear();
        m_cachedCorruptedTaskIndex.clear();
        m_submittedTaskIndex.clear();
    }

    return MJPEGVideoDecoderBaseMFX::Close();
} // MJPEGVideoDecoderMFX_HW::Close()

Status MJPEGVideoDecoderMFX_HW::AllocateFrame()
{
    mfxSize size = m_frameDims;
    size.height = m_DecoderParams.info.disp_clip_info.height;
    size.width = m_DecoderParams.info.disp_clip_info.width;

    VideoDataInfo info;
    info.Init(size.width, size.height, m_DecoderParams.info.color_format, 8);
    info.SetPictureStructure(m_interleaved ? VideoDataInfo::PS_FRAME : VideoDataInfo::PS_TOP_FIELD_FIRST);

    FrameMemID frmMID;
    Status sts = m_frameAllocator->Alloc(&frmMID, &info, 0);

    if (sts != UMC_OK)
    {
        return UMC_ERR_ALLOC;
    }

    m_frameData.Init(&info, frmMID, m_frameAllocator);

    return UMC_OK;
}

Status MJPEGVideoDecoderMFX_HW::CloseFrame(UMC::FrameData** in, const mfxU32 fieldPos)
{
    if((*in) != NULL)
    {
        (*in)[fieldPos].Close();
    }

    return UMC_OK;

} // Status MJPEGVideoDecoderMFX::CloseFrame(void)

mfxStatus MJPEGVideoDecoderMFX_HW::CheckStatusReportNumber(uint32_t statusReportFeedbackNumber, mfxU16* corrupted)
{
    UMC::Status sts = UMC_OK;

    mfxU32 numStructures = 32;
    mfxU32 numZeroFeedback = 0;
    JPEG_DECODE_QUERY_STATUS queryStatus[32];

    std::set<mfxU32>::iterator iteratorReady;
    std::set<mfxU32>::iterator iteratorSubmitted;
    std::set<mfxU32>::iterator iteratorCorrupted;

    AutomaticUMCMutex guard(m_guard);
    iteratorReady = find(m_cachedReadyTaskIndex.begin(), m_cachedReadyTaskIndex.end(), statusReportFeedbackNumber);
    iteratorSubmitted = find(m_submittedTaskIndex.begin(), m_submittedTaskIndex.end(), statusReportFeedbackNumber);
    if (m_cachedReadyTaskIndex.end() == iteratorReady){
        for (mfxU32 i = 0; i < numStructures; i += 1){
            queryStatus[i].bStatus = 3;
        }
        // on Linux ExecuteStatusReportBuffer returns UMC_ERR_UNSUPPORTED so the task pretends as already done
        // mark tasks as not completed to skip them in the next loop
        for (mfxU32 i = 0; i < numStructures; i += 1) {
            queryStatus[i].bStatus = 5;
            queryStatus[i].StatusReportFeedbackNumber = 0;
        }
        // mark the first task as ready if m_submittedTaskIndex is not empty
        if (m_submittedTaskIndex.end() != iteratorSubmitted) {
            queryStatus[0].bStatus = 0;
            queryStatus[0].StatusReportFeedbackNumber = statusReportFeedbackNumber;
        }

        if(sts != UMC_OK)
        {
            return MFX_ERR_DEVICE_FAILED;
        }

        for (mfxU32 i = 0; i < numStructures; i += 1)
        {
            if(queryStatus[i].StatusReportFeedbackNumber==0)numZeroFeedback++;
            if (0 == queryStatus[i].bStatus || 1 == queryStatus[i].bStatus)
            {
                m_cachedReadyTaskIndex.insert(queryStatus[i].StatusReportFeedbackNumber);
            }
            else if (2 == queryStatus[i].bStatus)
            {
                m_cachedCorruptedTaskIndex.insert(queryStatus[i].StatusReportFeedbackNumber);
            }
            else if (3 == queryStatus[i].bStatus)
            {
                return MFX_ERR_DEVICE_FAILED;
            }
            else if (5 == queryStatus[i].bStatus)
            {
                numZeroFeedback--;
                // Operation is not completed; do nothing
                continue;
            }
        }
        iteratorReady = find(m_cachedReadyTaskIndex.begin(), m_cachedReadyTaskIndex.end(), statusReportFeedbackNumber);
        {
            if (m_cachedReadyTaskIndex.end() == iteratorReady)
            {
                iteratorCorrupted = find(m_cachedCorruptedTaskIndex.begin(), m_cachedCorruptedTaskIndex.end(), statusReportFeedbackNumber);

                if(m_cachedCorruptedTaskIndex.end() == iteratorCorrupted)
                {
                    if(m_submittedTaskIndex.end() != iteratorSubmitted && numZeroFeedback == numStructures )
                        return MFX_ERR_DEVICE_FAILED;
                    else
                        return MFX_TASK_BUSY;
                }

                m_cachedCorruptedTaskIndex.erase(iteratorCorrupted);

                *corrupted = 1;
                m_submittedTaskIndex.erase(iteratorSubmitted);
                return MFX_TASK_DONE;
            }

            m_submittedTaskIndex.erase(iteratorSubmitted);
            m_cachedReadyTaskIndex.erase(iteratorReady);
        }
    }else{
        m_submittedTaskIndex.erase(iteratorSubmitted);
        m_cachedReadyTaskIndex.erase(iteratorReady);
    }

    return MFX_TASK_DONE;
}

Status MJPEGVideoDecoderMFX_HW::_DecodeHeader(CBaseStreamInput* in, int32_t* cnt)
{
    JSS      sampling;
    JERRCODE jerr;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    jerr = m_decBase->SetSource(in);
    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    mfxSize size = {};

    int32_t frameChannels, framePrecision;
    jerr = m_decBase->ReadHeader(
        &size.width, &size.height, &frameChannels, &m_color, &sampling, &framePrecision);

    if(JPEG_ERR_BUFF == jerr)
        return UMC_ERR_NOT_ENOUGH_DATA;

    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    bool sizeHaveChanged = (m_frameDims.width != size.width || (m_frameDims.height != size.height && m_frameDims.height != (size.height << 1)));

    if ((m_frameSampling != (int)sampling) || (m_frameDims.width && sizeHaveChanged))
    {
        in->Seek(-m_decBase->GetNumDecodedBytes(),UIC_SEEK_CUR);
        *cnt = 0;
        return UMC_ERR_NOT_ENOUGH_DATA;
    }

    *cnt = m_decBase->GetNumDecodedBytes();
    return UMC_OK;
}

Status MJPEGVideoDecoderMFX_HW::_DecodeField(MediaDataEx* in)
{
    int32_t     nUsedBytes = 0;

    CMemBuffInput stream;
    stream.Open((uint8_t*)in->GetDataPointer(), (int32_t)in->GetDataSize());

    Status status = _DecodeHeader(&stream, &nUsedBytes);
    if (status > 0)
    {
        in->MoveDataPointer(nUsedBytes);
        return UMC_OK;
    }

    if(UMC_OK != status)
    {
        in->MoveDataPointer(nUsedBytes);
        return status;
    }

    status = GetFrameHW(in);
    if (status != UMC_OK)
    {
        return status;
    }

    in->MoveDataPointer(m_decBase->GetNumDecodedBytes());

    return status;
} // MJPEGVideoDecoderMFX_HW::_DecodeField(MediaDataEx* in)

Status MJPEGVideoDecoderMFX_HW::DefaultInitializationHuffmantables()
{
    JERRCODE jerr;
    if(m_decBase->m_dctbl[0].IsEmpty())
    {
        jerr = m_decBase->m_dctbl[0].Create();
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;

        jerr = m_decBase->m_dctbl[0].Init(0,0,(uint8_t*)&DefaultLuminanceDCBits[0],(uint8_t*)&DefaultLuminanceDCValues[0]);
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;
    }

    if(m_decBase->m_dctbl[1].IsEmpty())
    {
        jerr = m_decBase->m_dctbl[1].Create();
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;

        jerr = m_decBase->m_dctbl[1].Init(1,0,(uint8_t*)&DefaultChrominanceDCBits[0],(uint8_t*)&DefaultChrominanceDCValues[0]);
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;
    }

    if(m_decBase->m_actbl[0].IsEmpty())
    {
        jerr = m_decBase->m_actbl[0].Create();
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;

        jerr = m_decBase->m_actbl[0].Init(0,1,(uint8_t*)&DefaultLuminanceACBits[0],(uint8_t*)&DefaultLuminanceACValues[0]);
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;
    }

    if(m_decBase->m_actbl[1].IsEmpty())
    {
        jerr = m_decBase->m_actbl[1].Create();
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;

        jerr = m_decBase->m_actbl[1].Init(1,1,(uint8_t*)&DefaultChrominanceACBits[0],(uint8_t*)&DefaultChrominanceACValues[0]);
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;
    }

    return UMC_OK;
}

Status MJPEGVideoDecoderMFX_HW::GetFrameHW(MediaDataEx* in)
{
#ifdef UMC_VA
    JERRCODE jerr;
    uint8_t    buffersForUpdate = 0; // Bits: 0 - picParams, 1 - quantTable, 2 - huffmanTables, 3 - bistreamData, 4 - scanParams
    bool     foundSOS = false;
    uint32_t   marker;
    JPEG_DECODE_SCAN_PARAMETER scanParams;
    Status sts;
    const size_t  srcSize = in->GetDataSize();
    const MediaDataEx::_MediaDataEx *pAuxData = in->GetExData();

    // we strongly need auxilary data
    if (NULL == pAuxData)
        return UMC_ERR_NULL_PTR;

    if (!m_va)
        return UMC_ERR_NOT_INITIALIZED;

    m_statusReportFeedbackCounter++;
    if (m_statusReportFeedbackCounter >= UINT_MAX)
        m_statusReportFeedbackCounter = 1;

    sts = m_va->BeginFrame(m_frameData.GetFrameMID());
    if (sts != UMC_OK)
        return sts;
    {
        AutomaticUMCMutex guard(m_guard);
        m_submittedTaskIndex.insert(m_statusReportFeedbackCounter);
    }
    /////////////////////////////////////////////////////////////////////////////////////////

    buffersForUpdate = (1 << 5) - 1;
    m_decBase->m_num_scans = GetNumScans(in);
    m_decBase->m_curr_scan = &m_decBase->m_scans[0];

    for (uint32_t i = 0; i < pAuxData->count; i += 1)
    {
        // get chunk size
        size_t chunkSize = (i + 1 < pAuxData->count) ?
                    (pAuxData->offsets[i + 1] - pAuxData->offsets[i]) :
                    (srcSize - pAuxData->offsets[i]);

        marker = pAuxData->values[i] & 0xFF;

        if(!foundSOS && (JM_SOS != marker))
        {
            continue;
        }
        else
        {
            foundSOS = true;
        }

        // some data
        if (JM_SOS == marker)
        {
            uint32_t nextNotRSTMarkerPos = i+1;

            while((pAuxData->values[nextNotRSTMarkerPos] & 0xFF) >= JM_RST0 &&
                  (pAuxData->values[nextNotRSTMarkerPos] & 0xFF) <= JM_RST7)
            {
                  nextNotRSTMarkerPos++;
            }

            // update chunk size
            chunkSize = (nextNotRSTMarkerPos < pAuxData->count) ?
                        (pAuxData->offsets[nextNotRSTMarkerPos] - pAuxData->offsets[i]) :
                        (srcSize - pAuxData->offsets[i]);

            CMemBuffInput stream;
            stream.Open((uint8_t*)in->GetDataPointer() + pAuxData->offsets[i] + 2, (int32_t)in->GetDataSize() - pAuxData->offsets[i] - 2);

            jerr = m_decBase->SetSource(&stream);
            if(JPEG_OK != jerr)
                return UMC_ERR_FAILED;

            jerr = m_decBase->ParseSOS(JO_READ_HEADER);
            if(JPEG_OK != jerr)
            {
                if (JPEG_ERR_BUFF == jerr)
                    return UMC_ERR_NOT_ENOUGH_DATA;
                else
                    return UMC_ERR_FAILED;
            }

            buffersForUpdate |= 1 << 4;

            scanParams.NumComponents        = (uint16_t)m_decBase->m_curr_scan->ncomps;
            scanParams.RestartInterval      = (uint16_t)m_decBase->m_curr_scan->jpeg_restart_interval;
            scanParams.MCUCount             = (uint32_t)(m_decBase->m_curr_scan->numxMCU * m_decBase->m_curr_scan->numyMCU);
            scanParams.ScanHoriPosition     = 0;
            scanParams.ScanVertPosition     = 0;
            scanParams.DataOffset           = (uint32_t)(pAuxData->offsets[i] + m_decBase->GetSOSLen() + 2); // 2 is marker length
            scanParams.DataLength           = (uint32_t)(chunkSize - m_decBase->GetSOSLen() - 2);

            for(int j = 0; j < m_decBase->m_curr_scan->ncomps; j += 1)
            {
                scanParams.ComponentSelector[j] = (uint8_t)m_decBase->m_ccomp[m_decBase->m_curr_comp_no + 1 - m_decBase->m_curr_scan->ncomps + j].m_id;
                scanParams.DcHuffTblSelector[j] = (uint8_t)m_decBase->m_ccomp[m_decBase->m_curr_comp_no + 1 - m_decBase->m_curr_scan->ncomps + j].m_dc_selector;
                scanParams.AcHuffTblSelector[j] = (uint8_t)m_decBase->m_ccomp[m_decBase->m_curr_comp_no + 1 - m_decBase->m_curr_scan->ncomps + j].m_ac_selector;
            }

            sts = PackHeaders(in, &scanParams, &buffersForUpdate);
            if (sts != UMC_OK)
                return sts;

            if (m_decBase->m_curr_scan->scan_no >= 2) // m_scans has 3 elements only
                m_decBase->m_curr_scan->scan_no = 0;

            m_decBase->m_curr_scan = &m_decBase->m_scans[m_decBase->m_curr_scan->scan_no + 1];
        }
        else if (JM_DRI == marker)
        {
            CMemBuffInput stream;
            stream.Open((uint8_t*)in->GetDataPointer() + pAuxData->offsets[i] + 2, (int32_t)in->GetDataSize() - pAuxData->offsets[i] - 2);

            jerr = m_decBase->SetSource(&stream);
            if(JPEG_OK != jerr)
                return UMC_ERR_FAILED;

            jerr = m_decBase->ParseDRI();
            if(JPEG_OK != jerr)
            {
                if (JPEG_ERR_BUFF == jerr)
                    return UMC_ERR_NOT_ENOUGH_DATA;
                else
                    return UMC_ERR_FAILED;
            }
        }
        else if (JM_DQT == marker)
        {
            CMemBuffInput stream;
            stream.Open((uint8_t*)in->GetDataPointer() + pAuxData->offsets[i] + 2, (int32_t)in->GetDataSize() - pAuxData->offsets[i] - 2);

            jerr = m_decBase->SetSource(&stream);
            if(JPEG_OK != jerr)
                return UMC_ERR_FAILED;

            jerr = m_decBase->ParseDQT();
            if(JPEG_OK != jerr)
            {
                if (JPEG_ERR_BUFF == jerr)
                    return UMC_ERR_NOT_ENOUGH_DATA;
                else
                    return UMC_ERR_FAILED;
            }

            buffersForUpdate |= 1 << 1;
        }
        else if (JM_DHT == marker)
        {
            CMemBuffInput stream;
            stream.Open((uint8_t*)in->GetDataPointer() + pAuxData->offsets[i] + 2, (int32_t)in->GetDataSize() - pAuxData->offsets[i] - 2);

            jerr = m_decBase->SetSource(&stream);
            if(JPEG_OK != jerr)
                return UMC_ERR_FAILED;

            jerr = m_decBase->ParseDHT();
            if(JPEG_OK != jerr)
            {
                if (JPEG_ERR_BUFF == jerr)
                    return UMC_ERR_NOT_ENOUGH_DATA;
                else
                    return UMC_ERR_FAILED;
            }

            buffersForUpdate |= 1 << 2;
        }
        else if ((JM_RST0 <= marker) && (JM_RST7 >= marker))
        {
            continue;
        }
        else if (JM_EOI == marker)
        {
            continue;
        }
        else
        {
            return UMC_ERR_NOT_IMPLEMENTED;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////
    sts = m_va->Execute();
    if (sts != UMC_OK)
        return sts;
    sts = m_va->EndFrame(NULL);
    if (sts != UMC_OK)
        return sts;
#endif
    return UMC_OK;
}

#ifdef UMC_VA
// Linux/Android version
Status MJPEGVideoDecoderMFX_HW::PackHeaders(MediaData* src, JPEG_DECODE_SCAN_PARAMETER* obtainedScanParams, uint8_t* buffersForUpdate)
{
    UMCVACompBuffer* compBuf = 0;
    uint32_t bitstreamTile = 0;
    bool shiftDataOffset = false;
    Status sts = UMC_OK;

    /////////////////////////////////////////////////////////////////////////////////////////
    if((*buffersForUpdate & 1) != 0)
    {
        *buffersForUpdate -= 1;

        UMCVACompBuffer* compBufPic;
        VAPictureParameterBufferJPEGBaseline *picParams;
        picParams = (VAPictureParameterBufferJPEGBaseline*)m_va->GetCompBuffer(VAPictureParameterBufferType,
                                                                            &compBufPic,
                                                                            sizeof(VAPictureParameterBufferJPEGBaseline));
        if(!picParams)
            return UMC_ERR_DEVICE_FAILED;
        picParams->picture_width  = (uint16_t)m_decBase->m_jpeg_width;
        picParams->picture_height = (uint16_t)m_decBase->m_jpeg_height;
        picParams->num_components = (uint8_t) m_decBase->m_jpeg_ncomp;

        if(m_decBase->m_jpeg_color == JC_RGB || m_decBase->m_jpeg_color == JC_RGBA) {
            picParams->color_space    = 1;
        } else if(m_decBase->m_jpeg_color == JC_BGR || m_decBase->m_jpeg_color == JC_BGRA) {
            picParams->color_space    = 2;
        } else {
            picParams->color_space    = 0;
        }
        switch(m_rotation)
        {
        case 0:
            picParams->rotation = VA_ROTATION_NONE;
            break;
        case 90:
            picParams->rotation = VA_ROTATION_90;
            break;
        case 180:
            picParams->rotation = VA_ROTATION_180;
            break;
        case 270:
            picParams->rotation = VA_ROTATION_270;
            break;
        }

        for (int32_t i = 0; i < m_decBase->m_jpeg_ncomp; i++)
        {
            picParams->components[i].component_id             = (uint8_t)m_decBase->m_ccomp[i].m_id;
            picParams->components[i].quantiser_table_selector = (uint8_t)m_decBase->m_ccomp[i].m_q_selector;
            picParams->components[i].h_sampling_factor        = m_decBase->m_ccomp[i].m_hsampling;
            picParams->components[i].v_sampling_factor        = m_decBase->m_ccomp[i].m_vsampling;
        }
        compBufPic->SetDataSize(sizeof(VAPictureParameterBufferJPEGBaseline));
    }

    /////////////////////////////////////////////////////////////////////////////////////////
    if((*buffersForUpdate & (1 << 1)) != 0)
    {
        *buffersForUpdate -= 1 << 1;

        UMCVACompBuffer* compBufQM;
        VAIQMatrixBufferJPEGBaseline *qmatrixParams;
        qmatrixParams = (VAIQMatrixBufferJPEGBaseline*)m_va->GetCompBuffer(VAIQMatrixBufferType,
                                                                       &compBufQM,
                                                                       sizeof(VAIQMatrixBufferJPEGBaseline));
        for (int32_t i = 0; i < MAX_QUANT_TABLES; i++)
        {
            if (!m_decBase->m_qntbl[i].m_initialized)
                continue;
            qmatrixParams->load_quantiser_table[i] = 1;
            for (int32_t j = 0; j < 64; j++)
            {
                qmatrixParams->quantiser_table[i][j] = m_decBase->m_qntbl[i].m_raw8u[j];
            }
        }
        compBufQM->SetDataSize(sizeof(VAIQMatrixBufferJPEGBaseline));
    }

    /////////////////////////////////////////////////////////////////////////////////////////
    if((*buffersForUpdate & (1 << 2)) != 0)
    {
        *buffersForUpdate -= 1 << 2;

        if (DefaultInitializationHuffmantables() != UMC_OK)
            return UMC_ERR_FAILED;

        UMCVACompBuffer* compBufHm;
        VAHuffmanTableBufferJPEGBaseline *huffmanParams;
        huffmanParams = (VAHuffmanTableBufferJPEGBaseline*)m_va->GetCompBuffer(VAHuffmanTableBufferType,
                                                                       &compBufHm,
                                                                       sizeof(VAHuffmanTableBufferJPEGBaseline));
        if(!huffmanParams)
            return UMC_ERR_DEVICE_FAILED;
        for (int32_t i = 0; i < 2; i++)
        {
            if(m_decBase->m_dctbl[i].IsValid())
            {
                huffmanParams->load_huffman_table[i] = 1;
                const uint8_t *bits = m_decBase->m_dctbl[i].GetBits();
                memcpy_s(huffmanParams->huffman_table[i].num_dc_codes, sizeof(huffmanParams->huffman_table[i].num_dc_codes), bits, 16);
                bits = m_decBase->m_dctbl[i].GetValues();
                memcpy_s(huffmanParams->huffman_table[i].dc_values, sizeof(huffmanParams->huffman_table[i].dc_values), bits, 12);
            }
            if(m_decBase->m_actbl[i].IsValid())
            {
                huffmanParams->load_huffman_table[i] = 1;
                const uint8_t *bits = m_decBase->m_actbl[i].GetBits();
                memcpy_s(huffmanParams->huffman_table[i].num_ac_codes, sizeof(huffmanParams->huffman_table[i].num_ac_codes), bits, 16);
                bits = m_decBase->m_actbl[i].GetValues();
                memcpy_s(huffmanParams->huffman_table[i].ac_values, sizeof(huffmanParams->huffman_table[i].ac_values), bits, 162);
            }
            huffmanParams->huffman_table[i].pad[0] = 0;
            huffmanParams->huffman_table[i].pad[1] = 0;
        }
        compBufHm->SetDataSize(sizeof(VAHuffmanTableBufferJPEGBaseline));
    }

    /////////////////////////////////////////////////////////////////////////////////////////
    if((*buffersForUpdate & (1 << 3)) != 0)
    {
        *buffersForUpdate -= 1 << 3;
        UMCVACompBuffer* compBufSlice;
        VASliceParameterBufferJPEGBaseline *sliceParams;
        sliceParams = (VASliceParameterBufferJPEGBaseline*)m_va->GetCompBuffer(VASliceParameterBufferType,
                                                                                 &compBufSlice,
                                                                                 sizeof(VASliceParameterBufferJPEGBaseline));
        if ( !sliceParams )
            return MFX_ERR_DEVICE_FAILED;
        sliceParams->slice_data_size           = obtainedScanParams->DataLength;
        sliceParams->slice_data_offset         = 0;
        sliceParams->slice_data_flag           = VA_SLICE_DATA_FLAG_ALL;
        sliceParams->num_components            = obtainedScanParams->NumComponents;
        sliceParams->restart_interval          = obtainedScanParams->RestartInterval;
        sliceParams->num_mcus                  = obtainedScanParams->MCUCount;
        sliceParams->slice_horizontal_position = obtainedScanParams->ScanHoriPosition;
        sliceParams->slice_vertical_position   = obtainedScanParams->ScanVertPosition;
        for (int32_t i = 0; i < m_decBase->m_jpeg_ncomp; i++)
        {
            sliceParams->components[i].component_selector = obtainedScanParams->ComponentSelector[i];
            sliceParams->components[i].dc_table_selector  = obtainedScanParams->DcHuffTblSelector[i];
            sliceParams->components[i].ac_table_selector  = obtainedScanParams->AcHuffTblSelector[i];
        }
        compBufSlice->SetDataSize(sizeof(VASliceParameterBufferJPEGBaseline));
    }

    if((*buffersForUpdate & (1 << 4)) != 0)
    {
        *buffersForUpdate -= 1 << 4;
        UMCVACompBuffer* compBufBs;
        uint8_t *ptr   = (uint8_t *)src->GetDataPointer();
        uint8_t *bistreamData = (uint8_t *)m_va->GetCompBuffer(VASliceDataBufferType, &compBufBs, obtainedScanParams->DataLength);

        if(!bistreamData)
            return UMC_ERR_DEVICE_FAILED;

        if(m_decBase->m_num_scans == 1)
        {
            memcpy_s(bistreamData, obtainedScanParams->DataLength, ptr + obtainedScanParams->DataOffset, obtainedScanParams->DataLength);
            shiftDataOffset = true;
        }
        else
        {
            memcpy_s(bistreamData, obtainedScanParams->DataLength, ptr + obtainedScanParams->DataOffset, obtainedScanParams->DataLength);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////
    while(bitstreamTile != 0)
    {
        UMCVACompBuffer* compBufBs;
        uint8_t *bistreamData = (uint8_t *)m_va->GetCompBuffer(VASliceDataBufferType, &compBufBs, bitstreamTile);
        MFX_INTERNAL_CPY(bistreamData, (uint8_t*)src->GetDataPointer() + obtainedScanParams->DataOffset + obtainedScanParams->DataLength - bitstreamTile, bitstreamTile);
        bitstreamTile = 0;
    }

    return UMC_OK;
}
#endif // if UMC_VA

uint16_t MJPEGVideoDecoderMFX_HW::GetNumScans(MediaDataEx* in)
{
    MediaDataEx::_MediaDataEx *pAuxData = in->GetExData();
    uint16_t numScans = 0;
    for(uint32_t i=0; i<pAuxData->count; i++)
    {
        if((pAuxData->values[i] & 0xFF) == JM_SOS)
        {
            numScans++;
        }
    }

    return numScans;
}

Status MJPEGVideoDecoderMFX_HW::GetFrame(UMC::MediaDataEx *pSrcData,
                                         UMC::FrameData** out,
                                         const mfxU32  fieldPos)
{
    Status status = UMC_OK;

    if(0 == out)
    {
        return UMC_ERR_NULL_PTR;
    }

    // allocate frame
    Status sts = AllocateFrame();
    if (sts != UMC_OK)
    {
        return sts;
    }

    if(m_interleaved)
    {
        // interleaved frame
        status = _DecodeField(pSrcData);
        if (status > 0)
        {
            return UMC_OK;
        }

        if (UMC_OK != status)
        {
            return status;
        }

        (*out)[fieldPos].Init(m_frameData.GetInfo(), m_frameData.GetFrameMID(), m_frameAllocator);
        (*out)[fieldPos].SetTime(pSrcData->GetTime());
        m_frameData.Close();
    }
    else
    {
        // progressive frame
        status = _DecodeField(pSrcData);
        if (status > 0)
        {
            return UMC_OK;
        }

        if (UMC_OK != status)
        {
            return status;
        }

        (*out)->Init(m_frameData.GetInfo(), m_frameData.GetFrameMID(), m_frameAllocator);
        (*out)->SetTime(pSrcData->GetTime());
        m_frameData.Close();
    }

    return status;

} // MJPEGVideoDecoderMFX_HW::GetFrame()

ConvertInfo * MJPEGVideoDecoderMFX_HW::GetConvertInfo()
{
    return &m_convertInfo;
}

} // end namespace UMC


#endif // #if defined(UMC_VA)
#endif // UMC_ENABLE_MJPEG_VIDEO_DECODER
