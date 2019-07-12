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

#ifndef __ASG_HEVC_FRAME_CHANGE_DESCRIPTOR_H__
#define __ASG_HEVC_FRAME_CHANGE_DESCRIPTOR_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include "block_structures.h"
#include "inputparameters.h"

// Descriptor containing all information about frame
struct FrameChangeDescriptor
{
    FRAME_MARKER                       m_changeType = SKIP;                 // type of frame
    mfxU16                             m_testType = UNDEFINED_TYPE;         // test type (i.e. use or not split or use)
    PROCESSING_MODE                    m_procMode = UNDEFINED_MODE;         // processing mode
    mfxU32                             m_frameNumber = 0;                   // frame number in display order
    ExtendedSurface*                   m_frame = nullptr;                   // ExtendedSurface for current frame
    mfxU16                             m_frameType = MFX_FRAMETYPE_UNKNOWN; // Frame type, used for bi-prediction decision
    bool                               m_bUseBiDirSW = true;                // Whether or not bi-directional search window should be used
                                                                            // instead of one-directional

    std::vector<CTUDescriptor>         m_vCTUdescr;           // vector of generated blocks inside current frame

    std::list<FrameChangeDescriptor>   m_refDescrList0;       // L0 list
    std::list<FrameChangeDescriptor>   m_refDescrList1;       // L1 list

    std::vector<BaseBlock>             m_OccupiedRefBlocks;   // vector of blocks on the current frame
                                                              // already taken by PUs from other frames

    mfxU32 GetFrameCropW() const
    {
        return m_frame ? m_frame->Info.CropW : 0;
    }
    mfxU32 GetFrameCropH() const
    {
        return m_frame ? m_frame->Info.CropH : 0;
    }

    FrameOccRefBlockRecord BackupOccupiedRefBlocks()
    {
        FrameOccRefBlockRecord ret;
        for (const FrameChangeDescriptor& descr : m_refDescrList0)
        {
            ret[descr.m_frameNumber] = descr.m_OccupiedRefBlocks;
        }

        for (const FrameChangeDescriptor& descr : m_refDescrList1)
        {
            ret[descr.m_frameNumber] = descr.m_OccupiedRefBlocks;
        }
        return ret;
    }

    void RestoreOccupiedRefBlocks(FrameOccRefBlockRecord& bak)
    {
        for (FrameChangeDescriptor& descr : m_refDescrList0)
        {
            if (bak.find(descr.m_frameNumber) != bak.end())
            {
                descr.m_OccupiedRefBlocks = std::move(bak[descr.m_frameNumber]);
            }
            else
            {
                throw std::string("ERROR: RestoreOccupiedRefBlocks: occupied block data not found in backup for L0");
            }
        }

        for (FrameChangeDescriptor& descr : m_refDescrList1)
        {
            if (bak.find(descr.m_frameNumber) != bak.end())
            {
                descr.m_OccupiedRefBlocks = std::move(bak[descr.m_frameNumber]);
            }
            else
            {
                throw std::string("ERROR: RestoreOccupiedRefBlocks: occupied block data not found in backup for L1");
            }
        }
        return;
    }

    bool IsNewPUValid(const PUBlock & newPU) const
    {
        return newPU.IsPUInRange(m_frame->Info) &&
            !CheckNewPUForIntersection(newPU);
    }

    bool CheckNewPUForIntersection(const PUBlock & newPU) const
    {
        if (newPU.predFlagL0)
        {
            const auto& frameDescrL0 = std::next(m_refDescrList0.begin(), newPU.m_MV.RefIdx.RefL0);
            BaseBlock shiftPU_L0 = newPU.GetShiftedBaseBlock(L0);
            for (const auto& occBlock : frameDescrL0->m_OccupiedRefBlocks)
            {
                if (occBlock.CheckForIntersect(shiftPU_L0)) return true;
            }
        }

        if (newPU.predFlagL1)
        {
            const auto& frameDescrL1 = std::next(m_refDescrList1.begin(), newPU.m_MV.RefIdx.RefL1);
            BaseBlock shiftPU_L1 = newPU.GetShiftedBaseBlock(L1);
            for (const auto& occBlock : frameDescrL1->m_OccupiedRefBlocks)
            {
                if (occBlock.CheckForIntersect(shiftPU_L1)) return true;
            }
        }

        return false;
    }
};


#endif

#endif // __ASG_HEVC_FRAME_CHANGE_DESCRIPTOR_H__
