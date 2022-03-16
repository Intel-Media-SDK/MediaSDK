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

#pragma once

#include "umc_defs.h"

#if defined (MFX_ENABLE_MPEG2_VIDEO_DECODE)

#include <memory>

#include "umc_frame_data.h"
#include "mfx_common_decode_int.h"

namespace UMC_MPEG2_DECODER
{
    class MPEG2Slice;

    class MPEG2DecoderFrameInfo
    {
    public:
        MPEG2DecoderFrameInfo(MPEG2DecoderFrame&);
        ~MPEG2DecoderFrameInfo();

        // Reinitialize frame structure before reusing frame
        void Reset();

        // Include a new slice into a set of frame slices
        void AddSlice(MPEG2Slice* slice);

        size_t GetSliceCount() const
        { return slices.size(); }

        MPEG2Slice* GetSlice(uint32_t num) const
        {
            return num < slices.size() ? slices[num] : nullptr;
        }

        bool IsField() const
        { return isField; }

        FrameType GetType() const
        { return frameType; }

        // Set forward and backward reference frames
        void UpdateReferences(const DPBType&);
        // Clear all references to other frames
        void FreeReferenceFrames();

        bool CheckReferenceFrameError();

        MPEG2DecoderFrame * GetForwardRefPic() const
        { return references.forward; }

        MPEG2DecoderFrame * GetBackwardRefPic() const
        { return references.backward; }

        void SetFilled()
        { isFilled = true; }

        bool IsFilled () const
        { return isFilled; }

    private:
        MPEG2DecoderFrame&                    frame;   // "Parent" frame
        bool                                  isField; // Field or frame
        bool                                  isBottomField;
        FrameType                             frameType;
        bool                                  isFilled;
        std::vector<MPEG2Slice*>              slices;

        struct RefFramePair
        {
            MPEG2DecoderFrame*  forward  = nullptr;
            MPEG2DecoderFrame*  backward = nullptr;
        };

        RefFramePair references;
    };

    class MPEG2DecoderFrame : public RefCounter
    {

    public:
        MPEG2DecoderFrame();
        ~MPEG2DecoderFrame();

        void Reset();
        void Allocate(UMC::FrameData const*);

        bool Empty() const
        { return !(data->m_locked); }
        bool IsDecoded() const
        { return decoded; }

        UMC::FrameData* GetFrameData()
        { return data.get(); }
        UMC::FrameData const* GetFrameData() const
        { return data.get(); }

        // Wrapper for getting 'surface Index' FrameMID
        UMC::FrameMemID GetMemID() const
        {
            return data->GetFrameMID();
        }

        int32_t GetError() const
        { return error; }

        void SetError(int32_t e)
        { error = e; }

        void AddError(int32_t e)
        { error |= e; }

        // Returns access unit: frame or field
        MPEG2DecoderFrameInfo * GetAU(uint8_t field = 0)
        { return (field) ? &slicesInfoBottom : &slicesInfo; }

        // Returns access unit: frame or field
        const MPEG2DecoderFrameInfo * GetAU(uint8_t field = 0) const
        { return (field) ? &slicesInfoBottom : &slicesInfo; }

        bool IsDisplayed() const
        { return displayed; }
        void SetDisplayed()
        { displayed = true; }

        bool IsOutputted() const
        { return outputted; }
        void SetOutputted()
        { outputted = true; }

        bool DecodingStarted() const
        { return decodingStarted; }
        void StartDecoding()
        { decodingStarted = true; }

        bool DecodingCompleted() const
        { return decodingCompleted; }
        // Flag frame as completely decoded
        void CompleteDecoding();

        // Check reference frames for error status and flag current frame if error is found
        void UpdateErrorWithRefFrameStatus();
        bool CheckReferenceFrameError();

        bool IsReadyToBeOutputted() const
        { return reordered; }
        void SetReadyToBeOutputted()
        { reordered = true; }

        bool IsFullFrame() const
        { return isFull;}
        void SetFullFrame(bool isFull)
        { this->isFull = isFull; }

        // Clear all references to other frames
        void FreeReferenceFrames();

        // Clean up data after decoding is done
        void FreeResources();

        // Sets reference frames
        void UpdateReferenceList(const DPBType & dpb);

        // Delete unneeded references and set flags after decoding is done
        void OnDecodingCompleted();

        bool IsRef() const
        { return isRef; }
        // Mark frame as a reference frame
        void SetIsRef(bool ref)
        {
            if (ref)
            {
                if (!IsRef())
                    IncrementReference();

                isRef = true;
            }
            else
            {
                bool wasRef = IsRef();
                isRef = false;

                if (wasRef)
                {
                    DecrementReference();
                }
            }
        }

        uint8_t GetNumberByParity(uint8_t parity) const
        {
            return bottom_field_flag[1] == parity ? 1 : 0;
        }

        // Skip frame API
        void SetSkipped(bool skipped)
        {
            isSkipped = skipped;
        }

        bool IsSkipped() const
        {
            return isSkipped;
        }

    public:

        uint32_t                          decOrder;
        uint32_t                          displayOrder;
        FrameType                         frameType;

        int8_t                            currFieldIndex; // 0 means first field (in decoded order) or frame, 1 means 2nd field (in decoded order)
        uint32_t                          fieldIndex; //Each slice of the current frame, 0 means first field (in decoded order) or frame, 1 means 2nd field (in decoded order)
        virtual void Free()
        {
            auto fd = GetFrameData();
            fd->m_locked = false;

            Reset();
        }

        int8_t                            bottom_field_flag[2];
        int32_t                           pictureStructure;
        DisplayPictureStruct              displayPictureStruct;

        bool                              isProgressiveSequence;
        bool                              isProgressiveFrame;

        double                            dFrameTime;
        bool                              isOriginalPTS;
        bool                              isPostProccesComplete;

        uint32_t                          horizontalSize;
        uint32_t                          verticalSize;

        uint32_t                          aspectWidth;
        uint32_t                          aspectHeight;

        std::shared_ptr<MPEG2GroupOfPictures> group;

    private:

        std::unique_ptr<UMC::FrameData>   data;
        uint16_t                          locked;

        bool                              decodingStarted;
        bool                              decodingCompleted;
        bool                              reordered; // Can frame already be returned to application or not
        bool                              outputted; // set in [application thread] when frame is mapped to respective output mfxFrameSurface
        bool                              displayed; // set in [scheduler thread] when frame decoding is finished and
                                                     // respective mfxFrameSurface prepared for output to application
        bool                              decoded;   // set in [application thread] to signal that frame is completed and respective reference counter decremented
                                                     // after it frame still may remain in [MPEG2Decoder::dpb], but only as reference

        bool                              isFull;

        bool                              isRef;     // Is reference frame

        bool                              isSkipped; // Is frame skipped

        int32_t                           error;     // error flags

        MPEG2DecoderFrameInfo             slicesInfo;       // Top field or frame
        MPEG2DecoderFrameInfo             slicesInfoBottom; // Bottom field
    };
}

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
