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

#include "frame_marker.h"
#include "frame_reorder.h"

void FrameMarker::PreProcessStreamConfig(InputParams & params)
{
    try
    {
        BuildRefLists(params);
        TagFrames(params);

        params.m_vProcessingParams = m_vProcessingParams;
    }
    catch (std::string & e) {
        std::cout << e << std::endl;
        throw std::string("ERROR: FrameMarker::PreProcessStreamConfig");
    }
    return;
}

void FrameMarker::BuildRefLists(const InputParams & params)
{
    // Parameters for reordering
    m_vProcessingParams.resize(params.m_numFrames);

    FrameReorder reorder(params);

    mfxVideoParam param = {};
    param.mfx.GopOptFlag = params.m_GopOptFlag;
    param.mfx.GopPicSize = params.m_GopSize;
    param.mfx.GopRefDist = params.m_RefDist;
    param.mfx.IdrInterval = params.m_nIdrInterval;
    param.mfx.FrameInfo.PicStruct = params.m_PicStruct;

    // Construction of the frame with correct lists
    // Reference list creating
    mfxU32 idxEncoded = 0;
    for (mfxU32 idxFrame = 0; idxFrame < params.m_numFrames; ++idxFrame)
    {
        ExternalFrame f = reorder.CreateExternalFrame(idxFrame, param);
        if (f.Poc != -1)
        {
            m_vProcessingParams.at(f.DisplayOrder) = f;
            m_vProcessingParams.at(f.DisplayOrder).EncodedOrder = idxEncoded;
            ++idxEncoded;
        }
    }
    // Drain buffered frames
    while (reorder.GetBufferedQueueSize() != 0)
    {
        ExternalFrame f = reorder.CreateExternalFrame(-1, param);
        m_vProcessingParams.at(f.DisplayOrder) = f;
        m_vProcessingParams.at(f.DisplayOrder).EncodedOrder = idxEncoded;
        ++idxEncoded;
    }

    return;
}

void FrameMarker::TagFrames(const InputParams & params)
{
    mfxVideoParam param = {};
    param.mfx.GopOptFlag = params.m_GopOptFlag;
    param.mfx.GopPicSize = params.m_GopSize;
    param.mfx.GopRefDist = params.m_RefDist;
    param.mfx.IdrInterval = params.m_nIdrInterval;
    param.mfx.FrameInfo.PicStruct = params.m_PicStruct;

    std::vector<mfxU32> refFramesL0;
    std::vector<mfxU32> refFramesL1;
    refFramesL0.reserve(std::max(params.m_NumRefActiveP, params.m_NumRefActiveBL0));
    refFramesL1.reserve(params.m_NumRefActiveBL1);

    mfxU32 num_gen = 0, num_mod = 0;

    if (GenerateInterMBs(params.m_TestType))
    {
        // Current test requires P/B frames

        // Creating of the processing parameters for each frame
        // It contains mark of the frame, reference frames for MOD frames (test frame)
        // For frames with mark GEN or SKIP reference frames are empty
        for (mfxU32 idxFrame = 0; idxFrame < params.m_numFrames; ++idxFrame)
        {
            // Clear references
            refFramesL0.clear();
            refFramesL1.clear();

            // Marker shouldn't set P-frames as MOD in presets with B-frames
            if ((m_vProcessingParams.at(idxFrame).Type & MFX_FRAMETYPE_P) && !HasBFramesInGOP(params.m_RefDist))
            {
                // We need to check number of available reference and their status
                // If they already are MOD or GEN we won't use them
                if (m_vProcessingParams.at(idxFrame).ListX[0].size() == params.m_NumRefActiveP && !HasAlreadyUsedRefs(idxFrame, 0))
                {
                    SetRefList(idxFrame, 0, refFramesL0, num_gen);
                    m_vProcessingParams.at(idxFrame).ReferencesL0 = refFramesL0;
                    if (params.m_UseGPB)
                    {
                        SetRefList(idxFrame, 1, refFramesL1, num_gen);
                        m_vProcessingParams.at(idxFrame).ReferencesL1 = refFramesL1;
                    }

                    m_vProcessingParams.at(idxFrame).Mark = MOD;
                    ++num_mod;
                }
            }
            else if (m_vProcessingParams.at(idxFrame).Type & MFX_FRAMETYPE_B)
            {
                // We need to check number of available reference and their status
                // If they already are MOD or GEN we won't use them
                if ((m_vProcessingParams.at(idxFrame).ListX[0].size() == params.m_NumRefActiveBL0 && !HasAlreadyUsedRefs(idxFrame, 0))
                    && (m_vProcessingParams.at(idxFrame).ListX[1].size() == params.m_NumRefActiveBL1 && !HasAlreadyUsedRefs(idxFrame, 1)))
                {
                    SetRefList(idxFrame, 0, refFramesL0, num_gen);
                    SetRefList(idxFrame, 1, refFramesL1, num_gen);
                    m_vProcessingParams.at(idxFrame).ReferencesL0 = refFramesL0;
                    m_vProcessingParams.at(idxFrame).ReferencesL1 = refFramesL1;
                    m_vProcessingParams.at(idxFrame).Mark = MOD;
                    ++num_mod;
                }
            }
        }
    }
    else
    {
        // Intra test only
        for (mfxU32 idxFrame = 0; idxFrame < params.m_numFrames; ++idxFrame)
        {
            m_vProcessingParams.at(idxFrame).Mark = MOD;
            ++num_mod;
        }
    }

    // Check that preset is valid, i.e. at least test_frames_percent_min frames are utilized for testing
    const mfxU32 test_frames_percent_min = 15;

    mfxU32 test_frames_percent = 100 * (num_gen + num_mod) / params.m_numFrames;

    if (test_frames_percent < test_frames_percent_min)
        throw std::string("ERROR: FrameMaker::CreateProcessingParams: Current preset involves too few frames");
}

bool FrameMarker::HasAlreadyUsedRefs(mfxU32 frame, mfxU8 list)
{
    for (mfxU32 idxRefLX = 0; idxRefLX < m_vProcessingParams.at(frame).ListX[list].size(); ++idxRefLX)
    {
        if (m_vProcessingParams.at(m_vProcessingParams.at(frame).ListX[list][idxRefLX].DisplayOrder).Mark != SKIP)
        {
            return true;
        }
    }
    return false;
}

void FrameMarker::SetRefList(mfxU32 frame, mfxU8 list, std::vector<mfxU32> & refFrames, mfxU32 & num_gen)
{
    for (mfxU32 idxRefLX = 0; idxRefLX < m_vProcessingParams.at(frame).ListX[list].size(); ++idxRefLX)
    {
        mfxU32 idxFrameLX = m_vProcessingParams.at(frame).ListX[list].at(idxRefLX).DisplayOrder;
        refFrames.push_back(idxFrameLX);
        m_vProcessingParams.at(idxFrameLX).Mark = GEN;
        ++num_gen;
    }
    return;
}

#endif // MFX_VERSION
