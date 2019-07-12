// Copyright (c) 2018-2019 Intel Corporation
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

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include "generator.h"

// Processor for generation mode
void Generator::Init()
{
    try
    {
        mfxStatus sts = m_FileReader.Init(std::list<msdk_string>{m_InputParams.m_InputFileName}, MFX_FOURCC_I420);
        CHECK_THROW_ERR(sts, "Generator::Init::m_FileReader.Init");

        sts = m_FileWriter.Init(m_InputParams.m_OutputFileName.c_str(), 1);
        CHECK_THROW_ERR(sts, "Generator::Init::m_FileWriter.Init");

        // Add a buffer to bufferWriter
        if (m_InputParams.m_TestType & GENERATE_PREDICTION)
            m_BufferWriter.AddBuffer(MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED, m_InputParams.m_PredBufferFileName);
    }
    catch (std::string& e) {
        std::cout << e << std::endl;
        throw std::string("ERROR: Couldn't initialize Generator");
    }
    return;
}

// Get surface and load new YUV frame from file to it
ExtendedSurface* Generator::PrepareSurface()
{
    ExtendedSurface* surf = GetFreeSurf();
    if (!surf)
    {
        throw std::string("ERROR: Generator::PrepareSurface: Undefined reference to surface");
    }
    ++surf->Data.Locked;

    mfxStatus sts = m_FileReader.LoadNextFrame(surf);
    CHECK_THROW_ERR(sts, "Generator::PrepareSurface::FileReader.LoadNextFrame");

    return surf;
}

void Generator::DropBuffers(ExtendedSurface& frame)
{
    for (mfxU32 i = 0; i < frame.Data.NumExtParam; ++i)
    {
        CHECK_THROW_ERR_BOOL(frame.Data.ExtParam[i], "Generator::DropFrames: surf.Data.ExtParam[i] == NULL");

        // Drop data on disk
        m_BufferWriter.WriteBuffer(frame.Data.ExtParam[i]);
        // Zero data of written buffer
        m_BufferWriter.ResetBuffer(frame.Data.ExtParam[i]);
    }

    return;
}

// Save all data
//
// Iterate through all unlocked frames and dump frame + buffers. When Locked frame met, iteration stops.
void Generator::DropFrames()
{
    // Sort data by frame order and drop in DisplayOrder

    // TODO / FIXME: Add EncodedOrder processing, it might be required for HEVC FEI PAK
    m_Surfaces.sort([](ExtendedSurface& left, ExtendedSurface& right) { return left.Data.FrameOrder < right.Data.FrameOrder; });

    for (ExtendedSurface& surf : m_Surfaces)
    {
        // If Locked surface met - stop dumping
        if (surf.Data.Locked) break;

        // If the surface has already been output, move on to the next one in frame order
        if (surf.isWritten) continue;

        // Write surface on disk
        mfxStatus sts = m_FileWriter.WriteNextFrameI420(&surf);
        surf.isWritten = true;
        CHECK_THROW_ERR(sts, "Generator::DropFrames::m_FileWriter.WriteNextFrameI420");
    }
}

void Generator::SavePSData()
{
    if (fpPicStruct.is_open())
    {
        // sort by coding order
        std::sort(m_RefControl.RefLogInfo.begin(), m_RefControl.RefLogInfo.end(), IsInCodingOrder);

        for (auto stat : m_RefControl.RefLogInfo)
            stat.Dump(fpPicStruct);
    }
}

#endif // MFX_VERSION