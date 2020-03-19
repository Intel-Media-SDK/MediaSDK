// Copyright (c) 2017-2019 Intel Corporation
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

#include <limits>
#include "mfx_vp9_dec_decode.h"

#include "mfx_common.h"
#include "mfx_common_decode_int.h"
#include "mfx_vpx_dec_common.h"
#include "mfx_enc_common.h"

#if defined(MFX_ENABLE_VP9_VIDEO_DECODE_HW)

#include "umc_vp9_utils.h"
#include "umc_vp9_bitstream.h"
#include "umc_vp9_frame.h"

using namespace UMC_VP9_DECODER;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MFX_VP9_Utility implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace MFX_VP9_Utility {

mfxStatus DecodeHeader(VideoCORE* core, mfxBitstream* bs, mfxVideoParam* params)
{
    mfxStatus sts = MFX_ERR_NONE;

    MFX_CHECK_NULL_PTR3(core, bs, params);

    sts = CheckBitstream(bs);
    MFX_CHECK_STS(sts);

    if (bs->DataLength < 3)
    {
        MoveBitstreamData(*bs, bs->DataLength);
        return MFX_ERR_MORE_DATA;
    }

    bool bHeaderRead = false;

    VP9DecoderFrame frame{};
    frame.bit_depth = 8;

    for (;;)
    {
        VP9Bitstream bsReader(bs->Data + bs->DataOffset,  bs->DataLength - bs->DataOffset);

        if (VP9_FRAME_MARKER != bsReader.GetBits(2))
            break; // invalid

        frame.profile = bsReader.GetBit();
        frame.profile |= bsReader.GetBit() << 1;
        if (frame.profile > 2)
            frame.profile += bsReader.GetBit();

        if (frame.profile >= 4)
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        if (bsReader.GetBit()) // show_existing_frame
            break;

        VP9_FRAME_TYPE frameType = (VP9_FRAME_TYPE) bsReader.GetBit();
        mfxU32 showFrame = bsReader.GetBit();
        mfxU32 errorResilientMode = bsReader.GetBit();

        if (KEY_FRAME == frameType)
        {
            if (!CheckSyncCode(&bsReader))
                return MFX_ERR_UNDEFINED_BEHAVIOR;

            if (frame.profile >= 2)
            {
                frame.bit_depth = bsReader.GetBit() ? 12 : 10;
            }

            if (SRGB != (COLOR_SPACE)bsReader.GetBits(3)) // color_space
            {
                bsReader.GetBit(); // color_range
                if (1 == frame.profile || 3 == frame.profile)
                {
                    frame.subsamplingX = bsReader.GetBit();
                    frame.subsamplingY = bsReader.GetBit();
                    bsReader.GetBit(); // reserved_zero
                }
                else
                {
                    frame.subsamplingX = 1;
                    frame.subsamplingY = 1;
                }
            }
            else
            {
                if (1 == frame.profile || 3 == frame.profile)
                    bsReader.GetBit();
                else
                    break; // invalid
            }

            frame.width = (mfxU16)bsReader.GetBits(16) + 1;
            frame.height = (mfxU16)bsReader.GetBits(16) + 1;

            bHeaderRead = true;
        }
        else
        {
            mfxU32 intraOnly = showFrame ? 0 : bsReader.GetBit();
            if (!intraOnly)
                break;

            if (!errorResilientMode)
                bsReader.GetBits(2);

            if (!CheckSyncCode(&bsReader))
                return MFX_ERR_UNDEFINED_BEHAVIOR;

            if (frame.profile >= 2)
                frame.bit_depth = bsReader.GetBit() ? 12 : 10;

            if (frame.profile == 0)
            {
                // There is no color format info in intra-only frame for frame.profile 0
                frame.subsamplingX = 1;
                frame.subsamplingY = 1;
            }
            else // frame.profile > 0
            {
                if (SRGB != (COLOR_SPACE)bsReader.GetBits(3)) // color_space
                {
                    bsReader.GetBit(); // color_range
                    if (1 == frame.profile || 3 == frame.profile)
                    {
                        frame.subsamplingX = bsReader.GetBit();
                        frame.subsamplingY = bsReader.GetBit();
                        bsReader.GetBit(); // reserved_zero
                    }
                    else
                    {
                        frame.subsamplingX = 1;
                        frame.subsamplingY = 1;
                    }
                }
                else
                {
                    if (1 == frame.profile || 3 == frame.profile)
                        bsReader.GetBit();
                    else
                        break; // invalid
                }
            }

            bsReader.GetBits(NUM_REF_FRAMES);

            frame.width = (mfxU16)bsReader.GetBits(16) + 1;
            frame.height = (mfxU16)bsReader.GetBits(16) + 1;

            bHeaderRead = true;
        }

        break;
    }

    if (!bHeaderRead)
    {
        MoveBitstreamData(*bs, bs->DataLength);
        return MFX_ERR_MORE_DATA;
    }

    FillVideoParam(core->GetPlatformType(), frame, *params);

    return MFX_ERR_NONE;
}

void FillVideoParam(eMFXPlatform platform, UMC_VP9_DECODER::VP9DecoderFrame const& frame, mfxVideoParam &params)
{
    params.mfx.CodecProfile = mfxU16(frame.profile + 1);

    params.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    params.mfx.FrameInfo.AspectRatioW = 1;
    params.mfx.FrameInfo.AspectRatioH = 1;

    params.mfx.FrameInfo.CropX = 0;
    params.mfx.FrameInfo.CropY = 0;
    params.mfx.FrameInfo.CropW = static_cast<mfxU16>(frame.width);
    params.mfx.FrameInfo.CropH = static_cast<mfxU16>(frame.height);

    params.mfx.FrameInfo.Width  = mfx::align2_value(params.mfx.FrameInfo.CropW, 16);
    params.mfx.FrameInfo.Height = mfx::align2_value(params.mfx.FrameInfo.CropH, 16);

    if (!frame.subsamplingX && !frame.subsamplingY)
        params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
    //else if (!subsampling_x && subsampling_y)
    //    params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV440;
    else if (frame.subsamplingX && !frame.subsamplingY)
        params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV422;
    else if (frame.subsamplingX && frame.subsamplingY)
        params.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    switch (frame.bit_depth)
    {
        case  8:
            params.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
            if (MFX_CHROMAFORMAT_YUV444 == params.mfx.FrameInfo.ChromaFormat)
                params.mfx.FrameInfo.FourCC = MFX_FOURCC_AYUV;
            else if (MFX_CHROMAFORMAT_YUV422 == params.mfx.FrameInfo.ChromaFormat)
                params.mfx.FrameInfo.FourCC = MFX_FOURCC_YUY2;
            params.mfx.FrameInfo.BitDepthLuma   = 8;
            params.mfx.FrameInfo.BitDepthChroma = 8;
            params.mfx.FrameInfo.Shift = 0;
            break;

        case 10:
            params.mfx.FrameInfo.FourCC = MFX_FOURCC_P010;
#if (MFX_VERSION >= 1027)
            if (MFX_CHROMAFORMAT_YUV444 == params.mfx.FrameInfo.ChromaFormat)
                params.mfx.FrameInfo.FourCC = MFX_FOURCC_Y410;
            else if (MFX_CHROMAFORMAT_YUV422 == params.mfx.FrameInfo.ChromaFormat)
                params.mfx.FrameInfo.FourCC = MFX_FOURCC_Y210;
#endif
            params.mfx.FrameInfo.BitDepthLuma   = 10;
            params.mfx.FrameInfo.BitDepthChroma = 10;
            break;

        case 12:
            params.mfx.FrameInfo.FourCC = 0;
#if (MFX_VERSION >= 1031)
            if (MFX_CHROMAFORMAT_YUV420 == params.mfx.FrameInfo.ChromaFormat)
                params.mfx.FrameInfo.FourCC = MFX_FOURCC_P016;
            else if(MFX_CHROMAFORMAT_YUV444 == params.mfx.FrameInfo.ChromaFormat)
                params.mfx.FrameInfo.FourCC = MFX_FOURCC_Y416;
#endif
            params.mfx.FrameInfo.BitDepthLuma   = 12;
            params.mfx.FrameInfo.BitDepthChroma = 12;
            break;
    }

    if (platform == MFX_PLATFORM_HARDWARE)
    {
        params.mfx.FrameInfo.Shift = 0;

        if (params.mfx.FrameInfo.FourCC == MFX_FOURCC_P010
#if (MFX_VERSION >= 1031)
            || params.mfx.FrameInfo.FourCC == MFX_FOURCC_P016
            || params.mfx.FrameInfo.FourCC == MFX_FOURCC_Y416
#endif
        )
        {
            params.mfx.FrameInfo.Shift = 1;
        }
    }
}

} //MFX_VP9_Utility

#endif // #if defined(MFX_ENABLE_VP9_VIDEO_DECODE) || defined(MFX_ENABLE_VP9_VIDEO_DECODE_HW)
