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

#include <mfx_mjpeg_task.h>
#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE) && defined(MFX_ENABLE_SW_FALLBACK)

#include <umc_mjpeg_mfx_decode.h>
#include <jpegbase.h>

#include <mfx_common_decode_int.h>

CJpegTaskBuffer::CJpegTaskBuffer(void)
{
    bufSize = 0;
    dataSize = 0;
    numPieces = 0;
    pBuf = NULL;
    imageHeaderSize = 0;
    fieldPos = 0;
    numScans = 0;
    timeStamp = 0;
} // CJpegTaskBuffer::CJpegTaskBuffer(void)

CJpegTaskBuffer::~CJpegTaskBuffer(void)
{
    Close();

} // CJpegTaskBuffer::~CJpegTaskBuffer(void)

void CJpegTaskBuffer::Close(void)
{
    if(pBuf)
    {
        delete[] pBuf;
        pBuf = NULL;
    }
    bufSize = 0;
    dataSize = 0;
    numPieces = 0;

} // void CJpegTaskBuffer::Close(void)

mfxStatus CJpegTaskBuffer::Allocate(const size_t size)
{
    // check if the existing buffer is good enough
    if (pBuf)
    {
        if (bufSize >= size)
        {
            return MFX_ERR_NONE;
        }

        Close();
    }

    // allocate the new buffer
    pBuf = new mfxU8[size];
    bufSize = size;

    return MFX_ERR_NONE;

} // mfxStatus CJpegTaskBuffer::Allocate(const size_t size)



CJpegTask::CJpegTask(void)
{
    m_numPic = 0;
    m_numPieces = 0;
    surface_work = NULL;
    surface_out = NULL;
    dst = NULL;
} // CJpegTask::CJpegTask(void)

CJpegTask::~CJpegTask(void)
{
    Close();

} // CJpegTask::~CJpegTask(void)

void CJpegTask::Close(void)
{
    // delete all buffers allocated
    for (auto& pic: m_pics)
    {
        pic.reset(nullptr);
    }

    m_numPic = 0;
    m_numPieces = 0;

} // void CJpegTask::Close(void)

mfxStatus CJpegTask::Initialize(UMC::VideoDecoderParams &params,
                                UMC::FrameAllocator *pFrameAllocator,
                                mfxU16 rotation,
                                mfxU16 chromaFormat,
                                mfxU16 colorFormat)
{
    // close the object before initialization
    Close();

    {
        UMC::Status umcRes;

        m_pMJPEGVideoDecoder.reset(new UMC::MJPEGVideoDecoderMFX);
        m_pMJPEGVideoDecoder->SetFrameAllocator(pFrameAllocator);
        umcRes = m_pMJPEGVideoDecoder->Init(&params);
        if (umcRes != UMC::UMC_OK)
        {
            return ConvertUMCStatusToMfx(umcRes);
        }
        m_pMJPEGVideoDecoder->Reset();

        switch(rotation)
        {
        case MFX_ROTATION_0:
            umcRes = m_pMJPEGVideoDecoder->SetRotation(0);
            break;
        case MFX_ROTATION_90:
            umcRes = m_pMJPEGVideoDecoder->SetRotation(90);
            break;
        case MFX_ROTATION_180:
            umcRes = m_pMJPEGVideoDecoder->SetRotation(180);
            break;
        case MFX_ROTATION_270:
            umcRes = m_pMJPEGVideoDecoder->SetRotation(270);
            break;
        }

        if (umcRes != UMC::UMC_OK)
        {
            return ConvertUMCStatusToMfx(umcRes);
        }

        umcRes = m_pMJPEGVideoDecoder->SetColorSpace(chromaFormat, colorFormat);

        if (umcRes != UMC::UMC_OK)
        {
            return ConvertUMCStatusToMfx(umcRes);
        }
    }

    return MFX_ERR_NONE;

} // mfxStatus CJpegTask::Initialize(const VideoDecoderParams params,

void CJpegTask::Reset(void)
{
    for(mfxU32 i=0; i<m_numPic; i++)
    {
        m_pics[i]->pieceOffset.clear();
        m_pics[i]->pieceSize.clear();
        m_pics[i]->pieceRSTOffset.clear();

        m_pics[i]->scanOffset.clear();
        m_pics[i]->scanSize.clear();
        m_pics[i]->scanTablesOffset.clear();
        m_pics[i]->scanTablesSize.clear();
    }

    m_numPic = 0;
    m_numPieces = 0;

} // void CJpegTask::Reset(void)

mfxStatus CJpegTask::AddPicture(UMC::MediaDataEx *pSrcData,
                                const mfxU32  fieldPos)
{
    const mfxU8*  pSrc = static_cast<const mfxU8*>(pSrcData->GetDataPointer());
    const size_t  srcSize = pSrcData->GetDataSize();
    const Ipp64f  timeStamp = pSrcData->GetTime();
    const UMC::MediaDataEx::_MediaDataEx *pAuxData = pSrcData->GetExData();
    Ipp32u        i, numPieces, maxNumPieces, numScans, maxNumScans;
    mfxStatus     mfxRes;
    size_t        imageHeaderSize;
    Ipp32u        marker;

    // we strongly need auxilary data
    if (NULL == pAuxData)
    {
        return MFX_ERR_NULL_PTR;
    }

    // allocate the buffer
    mfxRes = CheckBufferSize(srcSize);
    if (MFX_ERR_NONE != mfxRes)
    {
        return mfxRes;
    }

    // allocates vectors for data offsets and sizes
    maxNumPieces = pAuxData->count;
    m_pics[m_numPic]->pieceOffset.resize(maxNumPieces);
    m_pics[m_numPic]->pieceSize.resize(maxNumPieces);
    m_pics[m_numPic]->pieceRSTOffset.resize(maxNumPieces);

    // allocates vectors for scans parameters
    maxNumScans = MAX_SCANS_PER_FRAME;
    m_pics[m_numPic]->scanOffset.resize(maxNumScans);
    m_pics[m_numPic]->scanSize.resize(maxNumScans);
    m_pics[m_numPic]->scanTablesOffset.resize(maxNumScans);
    m_pics[m_numPic]->scanTablesSize.resize(maxNumScans);

    // get the number of pieces collected. SOS piece is supposed to be
    imageHeaderSize = 0;
    numPieces = 0;
    numScans = 0;
    for (i = 0; i < pAuxData->count; i += 1)
    {
        size_t chunkSize;

        // get chunk size
        chunkSize = (i + 1 < pAuxData->count) ?
                    (pAuxData->offsets[i + 1] - pAuxData->offsets[i]) :
                    (srcSize - pAuxData->offsets[i]);

        marker = pAuxData->values[i] & 0xFF;

        m_pics[m_numPic]->pieceRSTOffset[numPieces] = pAuxData->values[i] >> 8;

        // some data
        if (JM_SOS == marker)
        {
            // fill the chunks with the current chunk data
            m_pics[m_numPic]->pieceOffset[numPieces] = pAuxData->offsets[i];
            m_pics[m_numPic]->pieceSize[numPieces] = chunkSize;
            numPieces += 1;

            // fill scan parameters
            m_pics[m_numPic]->scanOffset[numScans] = pAuxData->offsets[i];
            m_pics[m_numPic]->scanSize[numScans] = chunkSize;
            numScans += 1;
            if (numScans >= maxNumScans)
            {
                throw UMC::UMC_ERR_INVALID_STREAM;
            }
        }
        else if ((JM_DRI == marker || JM_DQT == marker || JM_DHT == marker) && 0 != numScans)
        {
            if(0 == m_pics[m_numPic]->scanTablesOffset[numScans])
            {
                m_pics[m_numPic]->scanTablesOffset[numScans] = pAuxData->offsets[i];
                m_pics[m_numPic]->scanTablesSize[numScans] += chunkSize;
            }
            else
            {
                m_pics[m_numPic]->scanTablesSize[numScans] += chunkSize;
            }
        }
        else if ((JM_RST0 <= marker) && (JM_RST7 >= marker))
        {
            // fill the chunks with the current chunk data
            m_pics[m_numPic]->pieceOffset[numPieces] = pAuxData->offsets[i];
            m_pics[m_numPic]->pieceSize[numPieces] = chunkSize;
            numPieces += 1;
        }
        // some header before the regular JPEG data
        else if (0 == numPieces)
        {
            imageHeaderSize += chunkSize;
        }
    }

    // copy the data
    if(m_pics[m_numPic]->bufSize < srcSize)
        return MFX_ERR_NOT_ENOUGH_BUFFER;
    std::copy(pSrc, pSrc + srcSize, m_pics[m_numPic]->pBuf);
    m_pics[m_numPic]->dataSize = srcSize;
    m_pics[m_numPic]->imageHeaderSize = imageHeaderSize;
    m_pics[m_numPic]->timeStamp = timeStamp;
    m_pics[m_numPic]->numScans = numScans;
    m_pics[m_numPic]->numPieces = numPieces;
    m_pics[m_numPic]->fieldPos = fieldPos;

    // increment the number of pictures collected
    m_numPic += 1;

    // increment the number of pieces collected
    m_numPieces += numPieces;

    return MFX_ERR_NONE;

} // mfxStatus CJpegTask::AddPicture(UMC::MediaDataEx *pSrcData,

mfxStatus CJpegTask::CheckBufferSize(const size_t srcSize)
{
    // add new entry in the array
    m_pics.reserve(m_numPic+1);
    while (m_pics.size() <= m_numPic)
    {
        m_pics.emplace_back(new CJpegTaskBuffer());
    }

    return m_pics[m_numPic]->Allocate(srcSize);

} // mfxStatus CJpegTask::CheckBufferSize(const size_t srcSize)

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE && MFX_ENABLE_SW_FALLBACK
