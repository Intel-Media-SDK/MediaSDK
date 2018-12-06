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
#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include "umc_mpeg2_defs.h"
#include "umc_mpeg2_slice.h"
#include "umc_mpeg2_frame.h"

namespace UMC_MPEG2_DECODER
{
    MPEG2DecoderFrameInfo::MPEG2DecoderFrameInfo(MPEG2DecoderFrame& Frame)
        : frame(Frame)
        , isField(false)
        , isBottomField(false)
        , frameType((FrameType)0)
        , isFilled(false)
    {
    }

    MPEG2DecoderFrameInfo::~MPEG2DecoderFrameInfo()
    {
        std::for_each(slices.begin(), slices.end(),
            std::default_delete<MPEG2Slice>()
        );
    }

    void MPEG2DecoderFrameInfo::Reset()
    {
        isField       = false;
        isBottomField = false;
        frameType     = (FrameType)0;
        isFilled      = false;

        std::for_each(slices.begin(), slices.end(),
            std::default_delete<MPEG2Slice>()
        );

        slices.resize(0);
        FreeReferenceFrames();

    }

    void MPEG2DecoderFrameInfo::AddSlice(MPEG2Slice* slice)
    {
        if (0 == slices.size()) // on the first slice
        {
            const auto pic    = slice->GetPicHeader();
            const auto picExt = slice->GetPicExtHeader();
            isField       =  picExt.picture_structure != FRM_PICTURE;
            isBottomField = (picExt.picture_structure == BOTTOM_FLD_PICTURE);
            frameType     = (FrameType)pic.picture_coding_type;
        }

        slices.push_back(slice);
    }

    // Set forward and backward reference frames
    void MPEG2DecoderFrameInfo::UpdateReferences(const DPBType & dpb)
    {
        const auto pic    = GetSlice(0)->GetPicHeader();
        const auto picExt = GetSlice(0)->GetPicExtHeader();

        if (MPEG2_I_PICTURE == pic.picture_coding_type)
            return;

        // 7.6.3.5 Prediction in P-pictures for IP pair
        if (FRM_PICTURE != frame.pictureStructure)
        {
            uint8_t currFieldIndex = frame.GetNumberByParity(picExt.picture_structure == BOTTOM_FLD_PICTURE);
            if ((1 == currFieldIndex) && (MPEG2_P_PICTURE == pic.picture_coding_type) &&  // current picture is P and the 2nd field
                (MPEG2_I_PICTURE == frame.GetAU(0)->GetSlice(0)->GetPicHeader().picture_coding_type) ) // the first field is I
            {
                references.forward = &frame;
                references.forward->IncrementReference();
                return;
            }
        }

        // 7.6.1 Frame/Field prediction
        DPBType refFrames;
        std::copy_if(dpb.begin(), dpb.end(), back_inserter(refFrames),
             [this](MPEG2DecoderFrame const * f)
             {
                return (f != &frame) && f->IsRef();
             }
        );

        // Find latest ref frame
        auto iter = std::max_element(std::begin(refFrames), std::end(refFrames),
            [](MPEG2DecoderFrame const* f1, MPEG2DecoderFrame const* f2)
            {
                return f1->decOrder < f2->decOrder;
            }
        );

        // If no reference frame is found, use 'self' reference.
        // This is to handle cases without leading I picture
        auto ref = (iter == std::end(refFrames)) ? &frame : *iter;

        if (MPEG2_P_PICTURE == pic.picture_coding_type)
        {
            references.forward = ref;
            references.forward->IncrementReference();
        }
        else // MPEG2_B_PICTURE
        {
            references.backward = ref;
            references.backward->IncrementReference();

            refFrames.remove(ref);

            // Find another latest ref frame
            auto iter = std::max_element(std::begin(refFrames), std::end(refFrames),
                [](MPEG2DecoderFrame const* f1, MPEG2DecoderFrame const* f2)
                {
                    return f1->decOrder < f2->decOrder;
                }
            );

            // if there is no 'forward' reference then use 'backward' again
            ref = (iter == std::end(refFrames)) ? references.backward : *iter;

            references.forward = ref;
            references.forward->IncrementReference();
        }
    }

    // Clear all references to other frames
    void MPEG2DecoderFrameInfo::FreeReferenceFrames()
    {
        if (references.forward)
            references.forward->DecrementReference();

        if (references.backward)
            references.backward->DecrementReference();

        references.forward  = nullptr;
        references.backward = nullptr;
    }

    bool MPEG2DecoderFrameInfo::CheckReferenceFrameError()
    {
        uint32_t checkedErrorMask = UMC::ERROR_FRAME_MINOR | UMC::ERROR_FRAME_MAJOR | UMC::ERROR_FRAME_REFERENCE_FRAME;

        if (references.forward && references.forward->GetError() & checkedErrorMask)
            return true;

        if (references.backward && references.backward->GetError() & checkedErrorMask)
            return true;

        return false;
    }

    MPEG2DecoderFrame::MPEG2DecoderFrame()
        : decOrder(-1)
        , displayOrder(-1)
        , frameType((FrameType)0)
        , currFieldIndex(-1)
        , pictureStructure(FRM_PICTURE)
        , displayPictureStruct(DPS_UNKNOWN)
        , isProgressiveSequence(false)
        , isProgressiveFrame(false)
        , dFrameTime(-1.0)
        , isOriginalPTS(false)
        , isPostProccesComplete(false)
        , horizontalSize(0)
        , verticalSize(0)
        , aspectWidth(0)
        , aspectHeight(0)
        , data(new UMC::FrameData{})
        , locked(0)
        , decodingStarted(false)
        , decodingCompleted(false)
        , reordered(false)
        , outputted(false)
        , displayed(false)
        , decoded(false)
        , isFull(false)
        , isRef(false)
        , isSkipped(false)
        , error(0)
        , slicesInfo(*this)
        , slicesInfoBottom(*this)
    {
        Reset();
    }

    MPEG2DecoderFrame::~MPEG2DecoderFrame()
    {
    }

    void MPEG2DecoderFrame::Reset()
    {
        slicesInfo.Reset();
        slicesInfoBottom.Reset();

        error     = 0;
        displayed = false;
        outputted = false;
        decoded   = false;

        decodingStarted   = false;
        decodingCompleted = false;

        reordered         = false;
        data->Close();

        ResetRefCounter();
        FreeReferenceFrames();

        decOrder     = -1;
        displayOrder = -1;
        frameType    = (FrameType)0;
        isFull       = false;
        isRef        = false;
        isSkipped    = false;

        bottom_field_flag[0] = bottom_field_flag[1] = -1;
        pictureStructure     = FRM_PICTURE;
        displayPictureStruct = DPS_UNKNOWN;
        currFieldIndex = -1;
        isProgressiveSequence = false;
        isProgressiveFrame = false;

        dFrameTime = -1.0;
        isOriginalPTS = false;
        isPostProccesComplete = false;

        horizontalSize = 0;
        verticalSize   = 0;

        aspectWidth  = 0;
        aspectHeight = 0;

        group.reset();
    }

    void MPEG2DecoderFrame::Allocate(UMC::FrameData const* fd)
    {
        if (!fd)
            throw mpeg2_exception(UMC::UMC_ERR_NULL_PTR);

        *data = *fd;
    }

    // Clean up data after decoding is done
    void MPEG2DecoderFrame::FreeResources()
    {
        FreeReferenceFrames();

        if (IsDecoded())
        {
            slicesInfo.Reset();
            slicesInfoBottom.Reset();
        }
    }

    // Clear all references to other frames
    void MPEG2DecoderFrame::FreeReferenceFrames()
    {
        slicesInfo.FreeReferenceFrames();
        slicesInfoBottom.FreeReferenceFrames();
    }

    // Flag frame as decoded and set flags after decoding is done
    void MPEG2DecoderFrame::CompleteDecoding()
    {
        UpdateErrorWithRefFrameStatus();
        decodingCompleted = true;
    }

    // Delete unneeded references and set flags after decoding is done
    void MPEG2DecoderFrame::OnDecodingCompleted()
    {
        UpdateErrorWithRefFrameStatus();
        decoded = true;

        FreeResources();
        DecrementReference();
    }

    // Check reference frames for error status and flag current frame if error is found
    void MPEG2DecoderFrame::UpdateErrorWithRefFrameStatus()
    {
        if (slicesInfo.CheckReferenceFrameError() || slicesInfoBottom.CheckReferenceFrameError())
        {
            AddError(UMC::ERROR_FRAME_REFERENCE_FRAME);
        }
    }
}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
