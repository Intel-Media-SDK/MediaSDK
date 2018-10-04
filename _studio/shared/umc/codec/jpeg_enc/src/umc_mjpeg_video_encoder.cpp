// Copyright (c) 2018 Intel Corporation
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
#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)

#include <string.h>
#include "umc_video_data.h"
#include "umc_mjpeg_video_encoder.h"
#include "membuffout.h"
#include "jpegenc.h"

// VD: max num of padding bytes at the end of AVI frame
//     should be 1 by definition (padding up to int16_t boundary),
//     but in reality can be bigger
#define AVI_MAX_PADDING_BYTES 4


namespace UMC {

Status MJPEGEncoderScan::Init(uint32_t numPieces)
{
    if(m_numPieces != numPieces)
    {
        m_numPieces = numPieces;
        m_pieceLocation.resize(m_numPieces);
        m_pieceOffset.resize(m_numPieces);
        m_pieceSize.resize(m_numPieces);
    }
    return UMC_OK;
}

Status MJPEGEncoderScan::Reset(uint32_t numPieces)
{
    return Init(numPieces);
}

void MJPEGEncoderScan::Close()
{
    m_numPieces = 0;
    m_pieceLocation.clear();
    m_pieceOffset.clear();
    m_pieceSize.clear();
}

uint32_t MJPEGEncoderScan::GetNumPieces()
{
    return m_numPieces;
}


MJPEGEncoderPicture::MJPEGEncoderPicture()
{
    m_sourceData.reset(new VideoData());
    m_release_source_data = false;
}

MJPEGEncoderPicture::~MJPEGEncoderPicture()
{
    if(m_release_source_data)
    {
        m_sourceData->ReleaseImage();
    }

    for(uint32_t i=0; i<m_scans.size(); i++)
    {
        delete m_scans[i];
    }

    m_scans.clear();
}

uint32_t MJPEGEncoderPicture::GetNumPieces()
{
    uint32_t numPieces = 0;

    for(uint32_t i=0; i<m_scans.size(); i++)
        numPieces += m_scans[i]->GetNumPieces();

    return numPieces;
}


void MJPEGEncoderFrame::Reset()
{
    for(uint32_t i=0; i<m_pics.size(); i++)
    {
        delete m_pics[i];
    }

    m_pics.clear();
}

uint32_t MJPEGEncoderFrame::GetNumPics()
{
    return (uint32_t)m_pics.size();
}

uint32_t MJPEGEncoderFrame::GetNumPieces()
{
    uint32_t numPieces = 0;

    for(uint32_t i=0; i<GetNumPics(); i++)
        numPieces += m_pics[i]->GetNumPieces();

    return numPieces;
}

Status MJPEGEncoderFrame::GetPiecePosition(uint32_t pieceNum, uint32_t* fieldNum, uint32_t* scanNum, uint32_t* piecePosInField, uint32_t* piecePosInScan)
{
    uint32_t prevNumPieces = 0;
    for(uint32_t i=0; i<GetNumPics(); i++)
    {
        if(prevNumPieces <= pieceNum && pieceNum < prevNumPieces + m_pics[i]->GetNumPieces())
        {
            *fieldNum = i;
            *piecePosInField = pieceNum - prevNumPieces;
            break;
        }
        else
        {
            prevNumPieces += m_pics[i]->GetNumPieces();
        }
    }

    prevNumPieces = 0;

    for(uint32_t i=0; i<m_pics[*fieldNum]->m_scans.size(); i++)
    {
        if(prevNumPieces <= *piecePosInField && *piecePosInField < prevNumPieces + m_pics[*fieldNum]->m_scans[i]->GetNumPieces())
        {
            *scanNum = i;
            *piecePosInScan = *piecePosInField - prevNumPieces;
            break;
        }
        else
        {
            prevNumPieces += m_pics[*fieldNum]->m_scans[i]->GetNumPieces();
        }
    }

    return UMC_OK;
}


VideoEncoder *CreateMJPEGEncoder() { return new MJPEGVideoEncoder(); }


MJPEGVideoEncoder::MJPEGVideoEncoder(void)
{
    m_IsInit      = false;
}

MJPEGVideoEncoder::~MJPEGVideoEncoder(void)
{
    Close();
}

Status MJPEGVideoEncoder::Init(BaseCodecParams* lpInit)
{
    MJPEGEncoderParams* pEncoderParams = (MJPEGEncoderParams*) lpInit;
    Status status = UMC_OK;
    JERRCODE jerr = JPEG_OK;
    uint32_t i, numThreads;

    if(!pEncoderParams)
        return UMC_ERR_NULL_PTR;

    for (auto& buffer: m_pBitstreamBuffer)
    {
        buffer->Reset();
    }

    status = VideoEncoder::Close();
    if(UMC_OK != status)
        return UMC_ERR_INIT;

    status = VideoEncoder::Init(lpInit);
    if(UMC_OK != status)
        return UMC_ERR_INIT;

    m_EncoderParams  = *pEncoderParams;
    
    // allocate the JPEG encoders
    numThreads = JPEG_ENC_MAX_THREADS;
    if ((m_EncoderParams.numThreads) &&
        (numThreads > (uint32_t) m_EncoderParams.numThreads))
    {
        numThreads = m_EncoderParams.numThreads;
    }

    m_enc.resize(numThreads);
    m_pBitstreamBuffer.resize(numThreads);
    for (i = 0; i < numThreads; i += 1)
    {
        m_enc[i].reset(new CJPEGEncoder());

        if(m_EncoderParams.quality)
        {
            jerr = m_enc[i]->SetDefaultQuantTable((uint16_t)m_EncoderParams.quality);
            if(JPEG_OK != jerr)
                return UMC_ERR_FAILED;
        }

        jerr = m_enc[i]->SetDefaultACTable();
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;

        jerr = m_enc[i]->SetDefaultDCTable();
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;

        if(!m_pBitstreamBuffer[i])
            m_pBitstreamBuffer[i].reset(new MediaData(m_EncoderParams.buf_size));
    }

    if(m_frame)
        m_frame->Reset();

    m_frame.reset(new MJPEGEncoderFrame);

    m_IsInit      = true;

    return UMC_OK;
}

Status MJPEGVideoEncoder::Reset(void)
{
    return Init(&m_EncoderParams);
}

Status MJPEGVideoEncoder::Close(void)
{
    for (auto& enc: m_enc)
    {
        enc.reset();
    }
    for (auto& buffer: m_pBitstreamBuffer)
    {
        buffer.reset();
    }

    if(m_frame)
        m_frame->Reset();

    m_IsInit       = false;

    return VideoEncoder::Close();
}

Status MJPEGVideoEncoder::FillQuantTableExtBuf(mfxExtJPEGQuantTables* quantTables)
{
    JERRCODE jerr = JPEG_OK;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    quantTables->NumTable = m_enc[0]->GetNumQuantTables();
    for(int i=0; i<quantTables->NumTable; i++)
    {
        jerr = m_enc[0]->FillQuantTable(i, quantTables->Qm[i]);
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;
    }

    return UMC_OK;
}

Status MJPEGVideoEncoder::FillHuffmanTableExtBuf(mfxExtJPEGHuffmanTables* huffmanTables)
{
    JERRCODE jerr = JPEG_OK;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    huffmanTables->NumACTable = m_enc[0]->GetNumACTables();
    for(int i=0; i<huffmanTables->NumACTable; i++)
    {
        jerr = m_enc[0]->FillACTable(i, huffmanTables->ACTables[i].Bits, huffmanTables->ACTables[i].Values);
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;
    }

    huffmanTables->NumDCTable = m_enc[0]->GetNumDCTables();
    for(int i=0; i<huffmanTables->NumDCTable; i++)
    {
        jerr = m_enc[0]->FillDCTable(i, huffmanTables->DCTables[i].Bits, huffmanTables->DCTables[i].Values);
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;
    }

    return UMC_OK;
}

Status MJPEGVideoEncoder::SetDefaultQuantTable(const mfxU16 quality)
{
    JERRCODE jerr = JPEG_OK;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    for(auto& enc: m_enc)
    {
        jerr = enc->SetDefaultQuantTable(quality);
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;
    }

    return UMC_OK;
}

Status MJPEGVideoEncoder::SetDefaultHuffmanTable()
{
    JERRCODE jerr = JPEG_OK;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    for(auto& enc: m_enc)
    {
        jerr = enc->SetDefaultACTable();
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;

        jerr = enc->SetDefaultDCTable();
        if(JPEG_OK != jerr)
            return UMC_ERR_FAILED;
    }

    return UMC_OK;
}

Status MJPEGVideoEncoder::SetQuantTableExtBuf(mfxExtJPEGQuantTables* quantTables)
{
    JERRCODE jerr = JPEG_OK;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    for(auto& enc: m_enc)
    {
        for(uint16_t j=0; j<quantTables->NumTable; j++)
        {
            jerr = enc->SetQuantTable(j, quantTables->Qm[j]);
            if(JPEG_OK != jerr)
                return UMC_ERR_FAILED;
        }
    }

    return UMC_OK;
}

Status MJPEGVideoEncoder::SetHuffmanTableExtBuf(mfxExtJPEGHuffmanTables* huffmanTables)
{
    JERRCODE jerr = JPEG_OK;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    for(auto& enc: m_enc)
    {
        for(int j=0; j<huffmanTables->NumACTable; j++)
        {
            jerr = enc->SetACTable(j, huffmanTables->ACTables[j].Bits, huffmanTables->ACTables[j].Values);
            if(JPEG_OK != jerr)
                return UMC_ERR_FAILED;
        }

        for(int j=0; j<huffmanTables->NumDCTable; j++)
        {
            jerr = enc->SetDCTable(j, huffmanTables->DCTables[j].Bits, huffmanTables->DCTables[j].Values);
            if(JPEG_OK != jerr)
                return UMC_ERR_FAILED;
        }
    }

    return UMC_OK;
}

bool MJPEGVideoEncoder::IsQuantTableInited()
{
    return m_enc[0]->IsQuantTableInited();
}

bool MJPEGVideoEncoder::IsHuffmanTableInited()
{
    return m_enc[0]->IsACTableInited() && m_enc[0]->IsDCTableInited();
}

Status MJPEGVideoEncoder::GetInfo(BaseCodecParams *info)
{
    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    *(MJPEGEncoderParams*)info = m_EncoderParams;

    return UMC_OK;
}

uint32_t MJPEGVideoEncoder::NumEncodersAllocated(void)
{
    return static_cast<Ipp32u>(m_enc.size());
}

uint32_t MJPEGVideoEncoder::NumPicsCollected(void)
{
    std::lock_guard<std::mutex> guard(m_guard);

    return m_frame->GetNumPics();
}

uint32_t MJPEGVideoEncoder::NumPiecesCollected(void)
{
    return m_frame->GetNumPieces();
}

Status MJPEGVideoEncoder::AddPicture(MJPEGEncoderPicture* pic)
{
    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    std::lock_guard<std::mutex> guard(m_guard);

    m_frame->m_pics.push_back(pic);

    return UMC_OK;
}

Status MJPEGVideoEncoder::GetFrame(MediaData* /*in*/, MediaData* /*out*/)
{
    JERRCODE   status   = JPEG_OK;
    return status;
}

Status MJPEGVideoEncoder::EncodePiece(const mfxU32 threadNumber, const uint32_t numPiece)
{
    mfxSize       srcSize;
    JCOLOR         jsrcColor = JC_NV12;
    JCOLOR         jdstColor = JC_NV12;
    JTMODE         jtmode   = JT_OLD;
    JMODE          jmode    = JPEG_BASELINE;
    JSS            jss      = JS_444;
    int32_t         srcChannels = 3;
    int32_t         depth    = 8;
    bool           planar   = false;
    JERRCODE       status   = JPEG_OK;
    CMemBuffOutput streamOut;
    uint32_t         numField, numScan, piecePosInField, piecePosInScan;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    m_frame->GetPiecePosition(numPiece, &numField, &numScan, &piecePosInField, &piecePosInScan);

    VideoData *in = m_frame->m_pics[numField]->m_sourceData.get();
    VideoData *pDataIn = DynamicCast<VideoData, MediaData>(in);

    if(!in)
        return UMC_OK;

    if(!pDataIn)
        return UMC_OK;

    srcSize.width  = pDataIn->GetWidth();
    srcSize.height = pDataIn->GetHeight();

    jmode = (JMODE)m_EncoderParams.profile;
    depth = pDataIn->GetPlaneBitDepth(0);

    switch(m_EncoderParams.chroma_format)
    {
    case MFX_CHROMAFORMAT_YUV444:
        // now RGB32 only is supported
        srcChannels = 4;
        jsrcColor   = JC_BGRA;        
        jdstColor   = JC_RGB;
        jss         = JS_444;  
        break;
    case MFX_CHROMAFORMAT_YUV422H:
        srcChannels = 3;
        jsrcColor   = JC_YCBCR;
        jdstColor   = JC_YCBCR;
        jss         = JS_422H;
        planar      = true;
        break;
    case MFX_CHROMAFORMAT_YUV422V:
        srcChannels = 3;
        jsrcColor   = JC_YCBCR;
        jdstColor   = JC_YCBCR;
        jss         = JS_422V;
        planar      = true;
        break;
    case MFX_CHROMAFORMAT_YUV420:
        srcChannels = 3;
        jsrcColor   = JC_YCBCR;
        jdstColor   = JC_YCBCR;
        jss         = JS_420;
        planar      = true;
        break;
    case MFX_CHROMAFORMAT_YUV400:
        srcChannels = 1;
        jsrcColor   = JC_GRAY;
        jdstColor   = JC_GRAY;
        break;
    default:
        return UMC_ERR_UNSUPPORTED;
    }

    status = streamOut.Open((uint8_t*)m_pBitstreamBuffer[threadNumber]->GetBufferPointer() + m_pBitstreamBuffer[threadNumber]->GetDataSize(),
                            (int)m_pBitstreamBuffer[threadNumber]->GetBufferSize() - (int)m_pBitstreamBuffer[threadNumber]->GetDataSize());
    if(JPEG_OK != status)
        return UMC_ERR_FAILED;

    status = m_enc[threadNumber]->SetDestination(&streamOut);
    if(JPEG_OK != status)
        return UMC_ERR_FAILED;

    if(depth == 8)
    {
        if(planar)
        {
            uint8_t  *pSrc[]  = {(uint8_t*)pDataIn->GetPlanePointer(0), (uint8_t*)pDataIn->GetPlanePointer(1), (uint8_t*)pDataIn->GetPlanePointer(2), (uint8_t*)pDataIn->GetPlanePointer(3)};
            int32_t  iStep[] = {(int32_t)pDataIn->GetPlanePitch(0), (int32_t)pDataIn->GetPlanePitch(1), (int32_t)pDataIn->GetPlanePitch(2), (int32_t)pDataIn->GetPlanePitch(3)};

            status = m_enc[threadNumber]->SetSource(pSrc, iStep, srcSize, srcChannels, jsrcColor, jss, 8);
            if(JPEG_OK != status)
                return UMC_ERR_FAILED;
        }
        else
        {
            uint8_t  *pSrc  = (uint8_t*)pDataIn->GetPlanePointer(0);
            int32_t  iStep = (int32_t)pDataIn->GetPlanePitch(0);

            status = m_enc[threadNumber]->SetSource(pSrc, iStep, srcSize, srcChannels, jsrcColor, jss, 8);
            if(JPEG_OK != status)
                return UMC_ERR_FAILED;
        }
    }
    else if(depth == 16)
    {
        if(planar)
        {
            int16_t *pSrc[]  = {(int16_t*)pDataIn->GetPlanePointer(0), (int16_t*)pDataIn->GetPlanePointer(1), (int16_t*)pDataIn->GetPlanePointer(2), (int16_t*)pDataIn->GetPlanePointer(3)};
            int32_t  iStep[] = {(int32_t)pDataIn->GetPlanePitch(0), (int32_t)pDataIn->GetPlanePitch(1), (int32_t)pDataIn->GetPlanePitch(2), (int32_t)pDataIn->GetPlanePitch(3)};

            status = m_enc[threadNumber]->SetSource(pSrc, iStep, srcSize, srcChannels, jsrcColor, jss, 16);
            if(JPEG_OK != status)
                return UMC_ERR_FAILED;
        }
        else
        {
            int16_t *pSrc  = (int16_t*)pDataIn->GetPlanePointer(0);
            int32_t  iStep = (int32_t)pDataIn->GetPlanePitch(0);

            status = m_enc[threadNumber]->SetSource(pSrc, iStep, srcSize, srcChannels, jsrcColor, jss, 16);
            if(JPEG_OK != status)
                return UMC_ERR_FAILED;
        }
    }
    else
        return UMC_ERR_UNSUPPORTED;

    if(jmode == JPEG_LOSSLESS)
    {
        status = m_enc[threadNumber]->SetParams(JPEG_LOSSLESS,
                                                jdstColor,
                                                jss,
                                                m_EncoderParams.restart_interval,
                                                m_EncoderParams.huffman_opt,
                                                m_EncoderParams.point_transform,
                                                m_EncoderParams.predictor);
        if(JPEG_OK != status)
            return UMC_ERR_FAILED;
    }
    else
    {
        status = m_enc[threadNumber]->SetParams(jmode,
                                                jdstColor,
                                                jss,
                                                m_EncoderParams.restart_interval,
                                                m_EncoderParams.interleaved,
                                                m_frame->m_pics[numField]->GetNumPieces(),
                                                piecePosInField,
                                                numScan,
                                                piecePosInScan,
                                                m_EncoderParams.huffman_opt,
                                                m_EncoderParams.quality,
                                                jtmode);
        if(JPEG_OK != status)
            return UMC_ERR_FAILED;
    }

    status = m_enc[threadNumber]->SetJFIFApp0Resolution(JRU_NONE, 1, 1);
    if(JPEG_OK != status)
        return UMC_ERR_FAILED;

    status = m_enc[threadNumber]->WriteHeader();
    if(JPEG_OK != status)
        return UMC_ERR_FAILED;

    status = m_enc[threadNumber]->WriteData();
    if(JPEG_ERR_DHT_DATA == status)
        return UMC_ERR_INVALID_PARAMS;
    else if(JPEG_OK != status)
        return UMC_ERR_FAILED;

    m_frame->m_pics[numField]->m_scans[numScan]->m_pieceLocation[piecePosInScan] = threadNumber; //m_pOutputBuffer[numPic].get()->m_pieceLocation[numPiece] = threadNumber;
    m_frame->m_pics[numField]->m_scans[numScan]->m_pieceOffset[piecePosInScan] = m_pBitstreamBuffer[threadNumber]->GetDataSize(); //m_pOutputBuffer[numPic].get()->m_pieceOffset[numPiece] = m_pBitstreamBuffer[threadNumber].get()->GetDataSize();
    m_frame->m_pics[numField]->m_scans[numScan]->m_pieceSize[piecePosInScan] = streamOut.GetPosition(); //m_pOutputBuffer[numPic].get()->m_pieceSize[numPiece] = streamOut.GetPosition();

    //out->SetTime(pDataIn->GetTime());

    m_pBitstreamBuffer[threadNumber]->SetDataSize(m_pBitstreamBuffer[threadNumber]->GetDataSize() + streamOut.GetPosition());

    return status;
}

Status MJPEGVideoEncoder::PostProcessing(MediaData* out)
{
    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    Status status = UMC_OK;
    uint32_t i, j, k;
    size_t requiredDataSize = 0;

    // check buffer size
    for(i=0; i<m_frame->GetNumPics(); i++)
        for(j=0; j<m_frame->m_pics[i]->m_scans.size(); j++)
            for(k=0; k<m_frame->m_pics[i]->m_scans[j]->GetNumPieces(); k++)
                requiredDataSize += m_frame->m_pics[i]->m_scans[j]->m_pieceSize[k];

    if(out->GetBufferSize() - out->GetDataSize() < requiredDataSize)
    {
        return UMC_ERR_NOT_ENOUGH_BUFFER;
    }

    for(i=0; i<m_frame->GetNumPics(); i++)
    {
        for(j=0; j<m_frame->m_pics[i]->m_scans.size(); j++)
        {
            for(k=0; k<m_frame->m_pics[i]->m_scans[j]->GetNumPieces(); k++)
            {
                size_t location = m_frame->m_pics[i]->m_scans[j]->m_pieceLocation[k];
                size_t offset = m_frame->m_pics[i]->m_scans[j]->m_pieceOffset[k];
                size_t size = m_frame->m_pics[i]->m_scans[j]->m_pieceSize[k];

                MFX_INTERNAL_CPY((uint8_t *)out->GetBufferPointer() + out->GetDataSize(),
                            (uint8_t *)m_pBitstreamBuffer[location]->GetBufferPointer() + offset,
                            (uint32_t)size);

                if(k != m_frame->m_pics[i]->m_scans[j]->GetNumPieces() - 1)
                {
                    uint8_t *link = (uint8_t *)out->GetBufferPointer() + out->GetDataSize() + size;
                    link[0] = 0xFF;
                    link[1] = (uint8_t)(0xD0 + (k % 8));

                    out->SetDataSize(out->GetDataSize() + size + 2);
                }
                else
                {
                    out->SetDataSize(out->GetDataSize() + size);
                }
            }
        }
    }

    return status;
}

} // end namespace UMC

#endif // MFX_ENABLE_MJPEG_VIDEO_ENCODE
