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

#include <fstream>
#include <algorithm>

#include "frame_marker.h"
#include "test_processor.h"

ASGLog TestProcessor::asgLog;

//Base
// Entry point for all test which uses TestProcessor
void TestProcessor::RunTest(const InputParams & params)
{
    try {
        Init(params);

        switch(params.m_TestType)
        {
        case GENERATE_PICSTRUCT:
            RunPSTest(params);
            return;
        case GENERATE_REPACK_CTRL:
            if (params.m_ProcMode == GENERATE)
                RunRepackGenerate();
            else //VERIFY
                RunRepackVerify();
            return;
        default:
            break;
        }

        //Check that processing parameters have correct number of elements
        if (m_vProcessingParams.size() != m_InputParams.m_numFrames)
            throw std::string("ERROR: Incorrect elements number in the m_vProcessingParams");

        // Sort processing params into the encoded order
        std::sort(m_vProcessingParams.begin(), m_vProcessingParams.end(),
            [](const FrameProcessingParam& left, const FrameProcessingParam& right) { return left.EncodedOrder < right.EncodedOrder; });

        auto itProcessingParam = m_vProcessingParams.begin();

        // Iterate through frames
        for (mfxU32 i = 0; i < m_InputParams.m_numFrames; ++i)
        {
            // Load picture and obtain a surface
            ExtendedSurface* frame = PrepareSurface();

            // Set display order for the surface
            frame->Data.FrameOrder = i;

            // Find surface with the next encoded order in the surface pool
            auto itSurf = std::find_if(m_Surfaces.begin(), m_Surfaces.end(),
                [&itProcessingParam](const ExtendedSurface& ext_surf) { return ext_surf.Data.FrameOrder == itProcessingParam->DisplayOrder; });

            // Do next iteration if surface pool hasn't got the required surface
            if (itSurf == m_Surfaces.end())
                continue;

            // Set encoded order for the surface
            itSurf->encodedOrder = itProcessingParam->EncodedOrder;

            // Generate all data
            ChangePicture(*itSurf);

            // Write next available frames in the display order
            DropFrames();

            if (m_InputParams.m_bIsForceExtMVPBlockSize)
            {
                itSurf->ForceMVPBlockSizeInOutputBuffer(m_InputParams.m_ForcedExtMVPBlockSize);
            }
            // Drop buffers in the encoded order
            DropBuffers(*itSurf);

            // Go to the next frame in the encoded order
            ++itProcessingParam;
        }

        // Drain buffered frames
        while (itProcessingParam != m_vProcessingParams.end())
        {
            // Find surface with the next encoded order in the surface pool
            auto itSurf = std::find_if(m_Surfaces.begin(), m_Surfaces.end(),
                [&itProcessingParam](const ExtendedSurface& ext_surf) { return ext_surf.Data.FrameOrder == itProcessingParam->DisplayOrder; });

            // All surfaces must be into the surface pool
            if(itSurf == m_Surfaces.end())
                throw std::string("ERROR: Surface pool doesn't have required surface");

            // Set encoded order for the surface
            itSurf->encodedOrder = itProcessingParam->EncodedOrder;

            // Generate all data
            ChangePicture(*itSurf);

            // Write next available frames in the display order
            DropFrames();

            if (m_InputParams.m_bIsForceExtMVPBlockSize)
            {
                itSurf->ForceMVPBlockSizeInOutputBuffer(m_InputParams.m_ForcedExtMVPBlockSize);
            }
            // Drop buffers in the encoded order
            DropBuffers(*itSurf);

            // Go to the next frame in the encoded order
            ++itProcessingParam;
        }
    }
    catch (std::string & e) {
        std::cout << e << std::endl;
        throw std::string("ERROR: TestProcessor::RunTest");
    }
    return;
}

struct sMultiPak
{
    mfxU32 NumBytesInNalUnit;
    mfxU8 SliceQP;
};

void TestProcessor::RunRepackGenerate()
{
    mfxU32 maxNumBytesInNalUnit = 67104768; //14 bits value with the max unit 4K: 0x3FFF*4*1024
    mfxU32 MaxFrameSize;

    sMultiPak multiPakInBin;

    mfxU32 partFrame = 0;
    ASGRandomGenerator& randomGen = GetRandomGen();

    randomGen.SeedGenerator((mfxU32)time(nullptr));

    for (mfxU32 countFrame = 0; countFrame < m_InputParams.m_numFrames; countFrame++)
    {
        partFrame = randomGen.GetRandomNumber(0, 9);

        fpRepackStr.read((mfxI8 *)&multiPakInBin, sizeof(multiPakInBin));
        if (!fpRepackStr.good())
            throw std::string("ERROR: multiPakStr file read failed");

        if (multiPakInBin.NumBytesInNalUnit > maxNumBytesInNalUnit)
            throw std::string("ERROR: NumBytesInNalUnit more than HW limitation");

        MaxFrameSize = multiPakInBin.NumBytesInNalUnit
                       - (multiPakInBin.NumBytesInNalUnit*partFrame)/10;
        MaxFrameSize += 512; //Add a 512B window for headers.

        //If max frame size exceeds 14 bits, the unit is 4KB, otherwise 32B.
        if (MaxFrameSize >= (0x1 << 14) * 32)
            MaxFrameSize = (MaxFrameSize + 0xFFF) & ~0xFFF; //Rounding to 4KB.
        else
            MaxFrameSize = (MaxFrameSize + 0x1F) & ~0x1F;//Rounding to 32B.

        fpRepackCtrl.write((mfxI8 *)&MaxFrameSize, sizeof(MaxFrameSize));
        if (!fpRepackCtrl.good())
            throw std::string("ERROR: Repack ctrl file write failed");

        fpRepackCtrl.write((mfxI8 *)&m_InputParams.m_NumAddPasses, sizeof(m_InputParams.m_NumAddPasses));
        if (!fpRepackCtrl.good())
            throw std::string("ERROR: Repack ctrl file write failed");

        fpRepackCtrl.write((mfxI8 *)m_InputParams.m_DeltaQP, sizeof(m_InputParams.m_DeltaQP));
        if (!fpRepackCtrl.good())
            throw std::string("ERROR: Repack ctrl file write failed");
    }
}

void TestProcessor::RunRepackVerify()
{
    sMultiPak multiPakInBin;
    mfxExtFeiHevcRepackCtrl repackCtrl;
    mfxI8 repackStat[64];
    mfxU32 activeNumPasses;
    mfxU32 activeNumPassesMinus1;

    for (mfxU32 countFrame = 0; countFrame < m_InputParams.m_numFrames; countFrame++)
    {
        // Read repack data from files
        activeNumPasses = -1;

        fpRepackCtrl.read((mfxI8 *)&repackCtrl.MaxFrameSize, sizeof(repackCtrl.MaxFrameSize));
        if (!fpRepackCtrl.good())
            throw std::string("ERROR: Repack ctrl file read failed");

        fpRepackCtrl.read((mfxI8 *)&repackCtrl.NumPasses, sizeof(repackCtrl.NumPasses));
        if (!fpRepackCtrl.good())
            throw std::string("ERROR: Repack ctrl file read failed");

        fpRepackCtrl.read((mfxI8 *)&repackCtrl.DeltaQP, sizeof(repackCtrl.DeltaQP));
        if (!fpRepackCtrl.good())
            throw std::string("ERROR: Repack ctrl file read failed");

        fpRepackStr.read((mfxI8 *)&multiPakInBin, sizeof(multiPakInBin));
        if (!fpRepackStr.good())
            throw std::string("ERROR: Repack str file read failed");

        fpRepackStat.getline(repackStat, sizeof(repackStat));
        if (!fpRepackStat.good())
            throw std::string("ERROR: Repack stat file read failed");
        std::istringstream repackStatStream(std::string(repackStat, strlen(repackStat)));
        repackStatStream >> activeNumPasses;

        // Check read repack data

        if (0xFF == multiPakInBin.SliceQP)
        {
            // 0xFF indicates skipping
            std::cout << "INFO: Frame " << countFrame << " in coded order skipped" << std::endl;
            continue;
        }

        if (repackCtrl.MaxFrameSize == 0)
            throw std::string("ERROR: Incorrect MaxFrameSize value in repack ctrl");

        if ((repackCtrl.NumPasses < 1) || (repackCtrl.NumPasses > HEVC_MAX_NUMPASSES))
            throw std::string("ERROR: Incorrect NumPasses value in repack ctrl");

        if ((activeNumPasses > 0) && (activeNumPasses <= (repackCtrl.NumPasses + 1)))
            activeNumPassesMinus1 = activeNumPasses - 1;
        else
            throw std::string("ERROR: Incorrect output NumPasses value in repack stat");

        if ((multiPakInBin.SliceQP < m_InputParams.m_InitialQP)
         || (multiPakInBin.SliceQP > HEVC_MAX_QP))
            throw std::string("ERROR: Incorrect parsed QP value in repack str");

        // Verify repack control result
        std::vector<mfxU8> validQP(repackCtrl.NumPasses + 1);

        validQP[0] = m_InputParams.m_InitialQP;
        for (mfxU32 idxPass = 0; idxPass < repackCtrl.NumPasses; ++idxPass)
            validQP[idxPass+1] = std::min((validQP[idxPass] + repackCtrl.DeltaQP[idxPass]), HEVC_MAX_QP);

        mfxU8 sliceQPParser = multiPakInBin.SliceQP;

        auto it = std::find_if(validQP.begin(), validQP.end(),
            [sliceQPParser](const mfxU8 curQP) { return curQP == sliceQPParser; });

        if (it == validQP.end() || (distance(validQP.begin(), it) != activeNumPassesMinus1))
        {
            std::cout << "ERROR: parsed " << (mfxU32)multiPakInBin.SliceQP
                      << ", expected " << (mfxU32)*it << " for Frame "
                      << countFrame << " in coded order" << std::endl;
            throw std::string("ERROR: QP mismatched");
        }

        if ((multiPakInBin.NumBytesInNalUnit > repackCtrl.MaxFrameSize)
            && (multiPakInBin.SliceQP < validQP[repackCtrl.NumPasses]/*The max */))
        {
            std::cout << "ERROR: NumBytesInNalUnit " << multiPakInBin.NumBytesInNalUnit
                      << " MaxFrameSize " << repackCtrl.MaxFrameSize << " for Frame "
                      << countFrame << " in coded order" << std::endl;
            throw std::string("ERROR: Max exceeded");
        }
    }
}

void TestProcessor::RunPSTest(const InputParams & params)
{
    m_RefControl.SetParams(params);

    // Iterate through frames, store with random PS
    for (mfxU32 i = 0; i < m_InputParams.m_numFrames; ++i)
    {
        m_RefControl.Add(i);
    }

    // Iterate through frames, encode in random order
    for (mfxU32 i = 0; i < m_InputParams.m_numFrames; ++i)
    {
        m_RefControl.Encode(i);
    }

    // Dump or Load + Compare
    SavePSData();
}

void TestProcessor::Init(const InputParams &params)
{
    try
    {
        // TODO: try to use move constructor for such initialization (InputParams &&)
        m_InputParams = params;

        asgLog.Init(params);

        if (params.m_TestType == GENERATE_PICSTRUCT)
        {
            fpPicStruct.open(params.m_PicStructFileName.c_str(), (params.m_ProcMode == VERIFY) ? std::fstream::in : std::fstream::out);
            if (!fpPicStruct.is_open())
                throw std::string("ERROR: PicStruct buffer open failed");
        }
        else if (params.m_TestType == GENERATE_REPACK_CTRL)
        {
            if (params.m_ProcMode == GENERATE)
            {
                fpRepackStr.open(params.m_RepackStrFileName.c_str(), std::fstream::in | std::fstream::binary);
                if(!fpRepackStr.is_open())
                    throw std::string("ERROR: multiPakStr file open failed");

                fpRepackCtrl.open(params.m_RepackCtrlFileName.c_str(), std::fstream::out | std::fstream::binary);
                if (!fpRepackCtrl.is_open())
                    throw std::string("ERROR: Repack ctrl file open failed");
            }
            else //VERIFY
            {
                fpRepackStr.open(params.m_RepackStrFileName.c_str(), std::fstream::in | std::fstream::binary);
                if(!fpRepackStr.is_open())
                    throw std::string("ERROR: multiPakStr file open failed");

                fpRepackCtrl.open(params.m_RepackCtrlFileName.c_str(), std::fstream::in | std::fstream::binary);
                if (!fpRepackCtrl.is_open())
                    throw std::string("ERROR: Repack ctrl file open failed");

                fpRepackStat.open(params.m_RepackStatFileName.c_str(), std::fstream::in);
                if (!fpRepackStat.is_open())
                    throw std::string("ERROR: Repack stat file open failed");
            }
        }
        else
        {
            // Processing parameters creation
            // TODO work directly to m_vProcessingParams
            FrameMarker frameMarker;
            frameMarker.PreProcessStreamConfig(m_InputParams);
            m_vProcessingParams = m_InputParams.m_vProcessingParams;

            m_FrameProcessor.Init(params);

            Init(); // calls derived classes initialization
        }
    }
    catch (std::string& e) {
        std::cout << e << std::endl;
        throw std::string("ERROR: Couldn't initialize TestProcessor");
    }
}

void TestProcessor::ChangePicture(ExtendedSurface & frame)
{
    try {
        FrameChangeDescriptor descr;
        PrepareDescriptor(descr, frame.encodedOrder);
        descr.m_frame = &frame;

        m_FrameProcessor.ProcessFrame(descr);

        // Verification mode
        VerifyDesc(descr);

        switch (descr.m_changeType)
        {
        case GEN:
            // Current GEN frame will be used as reference for MOD frame
            m_ProcessedFramesDescr.push_back(std::move(descr));
            break;

        case SKIP:
            break;

        case MOD:
            // After processing of MOD frame all previously saved GEN frames could be released
            m_ProcessedFramesDescr.clear();
            break;
        default:
            break;
        }

    }
    catch (std::string & e) {
        std::cout << e << std::endl;
        throw std::string("ERROR: TestProcessor::ChangePicture");
    }
    return;
}

ExtendedSurface* TestProcessor::GetFreeSurf()
{
    // Find unlocked frame in pool
    auto it = std::find_if(m_Surfaces.begin(), m_Surfaces.end(),
        [](const ExtendedSurface& ext_surf) { return (ext_surf.Data.Locked == 0 && ext_surf.isWritten); });

    if (it == m_Surfaces.end())
    {
        // Create new surface if not found

        m_Surfaces.emplace_back(ExtendedSurface());

        // Allocate data for pixels
        m_Surfaces.back().AllocData(m_InputParams.m_width, m_InputParams.m_height);

        if (m_InputParams.m_TestType & GENERATE_PREDICTION)
        {
            /* Data granularity is always in 16x16 blocks. However, width/height alignment
               should be 32 because of HW requirements */
            mfxU32 alignedWidth = 0;
            mfxU32 alignedHeight = 0;

            if (m_InputParams.m_CTUStr.CTUSize == 16 || m_InputParams.m_CTUStr.CTUSize == 32)
            {
                alignedWidth = MSDK_ALIGN32(m_InputParams.m_width);
                alignedHeight = MSDK_ALIGN32(m_InputParams.m_height);
            }
            else if (m_InputParams.m_CTUStr.CTUSize == 64)
            {
                //For future 64x64 CTU applications
                alignedWidth = MSDK_ALIGN(m_InputParams.m_width, 64);
                alignedHeight = MSDK_ALIGN(m_InputParams.m_height, 64);
            }

            ExtendedBuffer buff(MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED, alignedWidth / MVP_BLOCK_SIZE, alignedHeight / MVP_BLOCK_SIZE);
            // Attach buffer to new frame if required
            m_Surfaces.back().AttachBuffer(buff);
        }

        return &m_Surfaces.back();
    }
    else
    {
        //Reset the isWritten flag upon surface reuse
        it->encodedOrder = 0xffffffff;
        it->isWritten = false;
        return &(*it);
    }
}

void TestProcessor::PrepareDescriptor(FrameChangeDescriptor & descr, const mfxU32 frameNum)
{
    descr.m_testType = m_InputParams.m_TestType;
    descr.m_procMode = m_InputParams.m_ProcMode;
    descr.m_changeType = m_vProcessingParams[frameNum].Mark;
    descr.m_frameNumber = m_vProcessingParams[frameNum].DisplayOrder;
    descr.m_frameType = m_vProcessingParams[frameNum].Type;
    descr.m_refDescrList0 = GetReferences(m_ProcessedFramesDescr, m_vProcessingParams[frameNum].ReferencesL0);
    descr.m_refDescrList1 = GetReferences(m_ProcessedFramesDescr, m_vProcessingParams[frameNum].ReferencesL1);

    // TODO: add support for full reference lists
    // m_refDescrList0 contains active reference frames with lower display order in descending order
    descr.m_refDescrList0.sort([](FrameChangeDescriptor const & lhs, FrameChangeDescriptor const & rhs) { return lhs.m_frameNumber > rhs.m_frameNumber; });

    // m_refDescrList1 contains active reference frames with higher display order in ascending order
    descr.m_refDescrList1.sort([](FrameChangeDescriptor const & lhs, FrameChangeDescriptor const & rhs) { return lhs.m_frameNumber < rhs.m_frameNumber; });

    //TODO: disable for I frames?

    descr.m_bUseBiDirSW = (m_vProcessingParams[frameNum].Type & MFX_FRAMETYPE_B) ||
        ((m_vProcessingParams[frameNum].Type & MFX_FRAMETYPE_P) && m_InputParams.m_UseGPB);

    return;
}

// Select references from list
// Most recently processed frames are at the end of the return list
std::list<FrameChangeDescriptor> TestProcessor::GetReferences(const std::list<FrameChangeDescriptor> & RecentProcessed, const std::vector<mfxU32>& ref_idx)
{
    std::list<FrameChangeDescriptor> refs_for_frame;

    for (auto & frm_descr : RecentProcessed)
    {
        if (std::find(ref_idx.begin(), ref_idx.end(), frm_descr.m_frameNumber) != ref_idx.end())
            refs_for_frame.emplace_back(frm_descr);
    }

    return refs_for_frame;
}

std::list<FrameChangeDescriptor> TestProcessor::GetSkips(const std::list<FrameChangeDescriptor> & RecentProcessed)
{
    std::list<FrameChangeDescriptor> skips;

    for (auto & frm_descr : RecentProcessed)
    {
        if (frm_descr.m_changeType == SKIP)
            skips.emplace_back(frm_descr);
    }

    return skips;
}

#endif // MFX_VERSION
