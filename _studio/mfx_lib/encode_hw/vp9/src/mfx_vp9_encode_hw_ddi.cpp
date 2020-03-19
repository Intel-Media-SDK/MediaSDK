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

#include <climits>
#include "mfx_vp9_encode_hw_utils.h"
#include "mfx_vp9_encode_hw_par.h"
#include "mfx_vp9_encode_hw_ddi.h"

namespace MfxHwVP9Encode
{

    GUID GetGuid(VP9MfxVideoParam const & par)
    {
        // Currently we don't support LP=OFF
        // so it is mapped to GUID_NULL
        // it will cause Query/Init fails with Unsupported
        // ever when driver support LP=OFF
        switch (par.mfx.CodecProfile)
        {
        case MFX_PROFILE_VP9_0:
            return (par.mfx.LowPower != MFX_CODINGOPTION_OFF) ?
                DXVA2_Intel_LowpowerEncode_VP9_Profile0 : GUID_NULL; //DXVA2_Intel_Encode_VP9_Profile0;
            break;
        case MFX_PROFILE_VP9_1:
            return (par.mfx.LowPower != MFX_CODINGOPTION_OFF) ?
                DXVA2_Intel_LowpowerEncode_VP9_Profile1 : GUID_NULL; //DXVA2_Intel_Encode_VP9_Profile1;
            break;
        case MFX_PROFILE_VP9_2:
            return (par.mfx.LowPower != MFX_CODINGOPTION_OFF) ?
                DXVA2_Intel_LowpowerEncode_VP9_10bit_Profile2 : GUID_NULL; // DXVA2_Intel_Encode_VP9_10bit_Profile2;
            break;
        case MFX_PROFILE_VP9_3:
            return (par.mfx.LowPower != MFX_CODINGOPTION_OFF) ?
                DXVA2_Intel_LowpowerEncode_VP9_10bit_Profile3 : GUID_NULL; // DXVA2_Intel_Encode_VP9_10bit_Profile3;
            break;
        default:
            // profile cannot be identified. Use Profile0 so far
            return (par.mfx.LowPower != MFX_CODINGOPTION_OFF) ?
                DXVA2_Intel_LowpowerEncode_VP9_Profile0 : GUID_NULL; // DXVA2_Intel_Encode_VP9_Profile0;
        }
    }

    mfxStatus QueryCaps(VideoCORE* pCore, ENCODE_CAPS_VP9 & caps, GUID guid, VP9MfxVideoParam const & par)
    {
        std::unique_ptr<DriverEncoder> ddi;

        ddi.reset(CreatePlatformVp9Encoder(pCore));
        MFX_CHECK(ddi.get() != NULL, MFX_WRN_PARTIAL_ACCELERATION);

        mfxStatus sts = ddi.get()->CreateAuxilliaryDevice(pCore, guid, par);
        MFX_CHECK_STS(sts);

        sts = ddi.get()->QueryEncodeCaps(caps);
        MFX_CHECK_STS(sts);

        return MFX_ERR_NONE;
    }

// uncompressed headencompressedr packing

#define VP9_FRAME_MARKER 0x2

#define VP9_SYNC_CODE_0 0x49
#define VP9_SYNC_CODE_1 0x83
#define VP9_SYNC_CODE_2 0x42

#define FRAME_CONTEXTS_LOG2 2
#define FRAME_CONTEXTS (1 << FRAME_CONTEXTS_LOG2)

#define QINDEX_BITS 8

#define MAX_TILE_WIDTH_B64 64
#define MIN_TILE_WIDTH_B64 4

#define MI_SIZE_LOG2 3
#define MI_BLOCK_SIZE_LOG2 (6 - MI_SIZE_LOG2)  // 64 = 2^6
#define MI_BLOCK_SIZE (1 << MI_BLOCK_SIZE_LOG2)  // mi-units per max block

    void WriteBit(BitBuffer &buf, mfxU8 bit)
    {
        const mfxU16 byteOffset = buf.bitOffset / CHAR_BIT;
        const mfxU8 bitsLeftInByte = CHAR_BIT - 1 - buf.bitOffset % CHAR_BIT;
        if (bitsLeftInByte == CHAR_BIT - 1)
        {
            buf.pBuffer[byteOffset] = mfxU8(bit << bitsLeftInByte);
        }
        else
        {
            buf.pBuffer[byteOffset] &= ~(1 << bitsLeftInByte);
            buf.pBuffer[byteOffset] |= bit << bitsLeftInByte;
        }
        buf.bitOffset = buf.bitOffset + 1;
    };

    void WriteLiteral(BitBuffer &buf, mfxU64 data, mfxU64 bits)
    {
        for (mfxI64 bit = bits - 1; bit >= 0; bit--)
        {
            WriteBit(buf, (data >> bit) & 1);
        }
    }

    void WriteColorConfig(BitBuffer &buf, VP9SeqLevelParam const &seqPar)
    {
        if (seqPar.profile >= PROFILE_2)
        {
            assert(seqPar.bitDepth > BITDEPTH_8);
            WriteBit(buf, seqPar.bitDepth == BITDEPTH_10 ? 0 : 1);
        }
        WriteLiteral(buf, seqPar.colorSpace, 3);
        if (seqPar.colorSpace != SRGB)
        {
            WriteBit(buf, seqPar.colorRange);
            if (seqPar.profile == PROFILE_1 || seqPar.profile == PROFILE_3)
            {
                assert(seqPar.subsamplingX != 1 || seqPar.subsamplingY != 1);
                WriteBit(buf, seqPar.subsamplingX);
                WriteBit(buf, seqPar.subsamplingY);
                WriteBit(buf, 0);  // unused
            }
            else
            {
                assert(seqPar.subsamplingX == 1 && seqPar.subsamplingY == 1);
            }
        }
        else
        {
            assert(seqPar.profile == PROFILE_1 || seqPar.profile == PROFILE_3);
            WriteBit(buf, 0);  // unused
        }
    }

    void WriteFrameSize(BitBuffer &buf, VP9FrameLevelParam const &framePar)
    {
        WriteLiteral(buf, framePar.width - 1, 16);
        WriteLiteral(buf, framePar.height - 1, 16);

        const mfxU8 renderFrameSizeDifferent = 0; // TODO: add support
        WriteBit(buf, renderFrameSizeDifferent);
        /*if (renderFrameSizeDifferent)
        {
            WriteLiteral(buf, framePar.renderWidth - 1, 16);
            WriteLiteral(buf, framePar.renderHeight - 1, 16);
        }*/
    }

    void WriteQIndexDelta(BitBuffer &buf, mfxI16 qDelta)
    {
        if (qDelta != 0)
        {
            WriteBit(buf, 1);
            WriteLiteral(buf, abs(qDelta), 4);
            WriteBit(buf, qDelta < 0);
        }
        else
        {
            WriteBit(buf, 0);
        }
    }

    mfxU16 WriteUncompressedHeader(BitBuffer &localBuf,
                                   Task const &task,
                                   VP9SeqLevelParam const &seqPar,
                                   BitOffsets &offsets)
    {
        VP9FrameLevelParam const &framePar = task.m_frameParam;

        Zero(offsets);

        offsets.BitOffsetUncompressedHeader = (mfxU16)localBuf.bitOffset;

        WriteLiteral(localBuf, VP9_FRAME_MARKER, 2);

        // profile
        switch (seqPar.profile)
        {
            case PROFILE_0: WriteLiteral(localBuf, 0, 2); break;
            case PROFILE_1: WriteLiteral(localBuf, 2, 2); break;
            case PROFILE_2: WriteLiteral(localBuf, 1, 2); break;
            case PROFILE_3: WriteLiteral(localBuf, 6, 3); break;
            default: assert(0);
        }

        WriteBit(localBuf, 0);  // show_existing_frame
        WriteBit(localBuf, framePar.frameType);
        WriteBit(localBuf, framePar.showFrame);
        WriteBit(localBuf, framePar.errorResilentMode);

        if (framePar.frameType == KEY_FRAME) // Key frame
        {
            // sync code
            WriteLiteral(localBuf, VP9_SYNC_CODE_0, 8);
            WriteLiteral(localBuf, VP9_SYNC_CODE_1, 8);
            WriteLiteral(localBuf, VP9_SYNC_CODE_2, 8);

            // color config
            WriteColorConfig(localBuf, seqPar);

            // frame, render size
            WriteFrameSize(localBuf, framePar);
        }
        else // Inter frame
        {
            if (!framePar.showFrame)
            {
                WriteBit(localBuf, framePar.intraOnly);
            }

            if (!framePar.errorResilentMode)
            {
                WriteLiteral(localBuf, framePar.resetFrameContext, 2);
            }

            // prepare refresh frame mask
            mfxU8 refreshFamesMask = 0;
            for (mfxU8 i = 0; i < DPB_SIZE; i++)
            {
                refreshFamesMask |= (framePar.refreshRefFrames[i] << i);
            }

            if (framePar.intraOnly)
            {
                // sync code
                WriteLiteral(localBuf, VP9_SYNC_CODE_0, 8);
                WriteLiteral(localBuf, VP9_SYNC_CODE_1, 8);
                WriteLiteral(localBuf, VP9_SYNC_CODE_2, 8);

                // Note for profile 0, 420 8bpp is assumed.
                if (seqPar.profile > PROFILE_0)
                {
                    WriteColorConfig(localBuf, seqPar);
                }

                // refresh frame info
                WriteLiteral(localBuf, refreshFamesMask, REF_FRAMES);

                // frame, render size
                WriteFrameSize(localBuf, framePar);
            }
            else
            {
                WriteLiteral(localBuf, refreshFamesMask, REF_FRAMES);
                for (mfxI8 refFrame = LAST_FRAME; refFrame <= ALTREF_FRAME; refFrame ++)
                {
                    WriteLiteral(localBuf, framePar.refList[int(refFrame)], REF_FRAMES_LOG2);
                    WriteBit(localBuf, framePar.refBiases[int(refFrame)]);
                }

                // frame size with refs
                mfxU8 found = 1;
                if (task.m_frameOrderInRefStructure == 0)
                {
                    // reference structure is reset which means resolution change
                    // don't inherit resolution of reference frames
                    found = 0;
                }

                for (mfxI8 refFrame = LAST_FRAME; refFrame <= ALTREF_FRAME; refFrame ++)
                {
                    // TODO: implement correct logic for [found] flag
                    WriteBit(localBuf, found);
                    if (found) break;
                }

                if (!found)
                {
                    WriteLiteral(localBuf, framePar.width - 1, 16);
                    WriteLiteral(localBuf, framePar.height - 1, 16);
                }

                const mfxU8 renderFrameSizeDifferent = 0; // TODO: add support
                WriteBit(localBuf, renderFrameSizeDifferent);
                /*if (renderFrameSizeDifferent)
                {
                    WriteLiteral(localBuf, framePar.renderWidth - 1, 16);
                    WriteLiteral(localBuf, framePar.renderHeight - 1, 16);
                }*/

                WriteBit(localBuf, framePar.allowHighPrecisionMV);

                // interpolation filter syntax
                const mfxU8 filterToLiteralMap[] = { 1, 0, 2, 3 };

                assert(framePar.interpFilter <= SWITCHABLE);
                WriteBit(localBuf, framePar.interpFilter == SWITCHABLE);
                if (framePar.interpFilter < SWITCHABLE)
                {
                    WriteLiteral(localBuf, filterToLiteralMap[framePar.interpFilter], 2);
                }
            }
        }

        if (!framePar.errorResilentMode)
        {
            WriteBit(localBuf, framePar.refreshFrameContext);
            WriteBit(localBuf, seqPar.frameParallelDecoding);
        }

        WriteLiteral(localBuf, framePar.frameContextIdx, FRAME_CONTEXTS_LOG2);

        offsets.BitOffsetForLFLevel = (mfxU16)localBuf.bitOffset;
        // loop filter syntax
        WriteLiteral(localBuf, framePar.lfLevel, 6);
        WriteLiteral(localBuf, framePar.sharpness, 3);

        WriteBit(localBuf, framePar.modeRefDeltaEnabled);

        if (framePar.modeRefDeltaEnabled)
        {
            WriteBit(localBuf, framePar.modeRefDeltaUpdate);
            if (framePar.modeRefDeltaUpdate)
            {
                offsets.BitOffsetForLFRefDelta = (mfxU16)localBuf.bitOffset;
                for (mfxI8 i = 0; i < MAX_REF_LF_DELTAS; i++)
                {
                    // always write deltas explicitly to allow BRC modify them
                    const mfxI8 delta = framePar.lfRefDelta[int(i)];
                    WriteBit(localBuf, 1);
                    WriteLiteral(localBuf, abs(delta) & 0x3F, 6);
                    WriteBit(localBuf, delta < 0);
                }

                offsets.BitOffsetForLFModeDelta = (mfxU16)localBuf.bitOffset;
                for (mfxI8 i = 0; i < MAX_MODE_LF_DELTAS; i++)
                {
                    // always write deltas explicitly to allow BRC modify them
                    const mfxI8 delta = framePar.lfModeDelta[int(i)];
                    WriteBit(localBuf, 1);
                    WriteLiteral(localBuf, abs(delta) & 0x3F, 6);
                    WriteBit(localBuf, delta < 0);
                }
            }
        }

        offsets.BitOffsetForQIndex = (mfxU16)localBuf.bitOffset;

        // quantization params
        WriteLiteral(localBuf, framePar.baseQIndex, QINDEX_BITS);
        WriteQIndexDelta(localBuf, framePar.qIndexDeltaLumaDC);
        WriteQIndexDelta(localBuf, framePar.qIndexDeltaChromaDC);
        WriteQIndexDelta(localBuf, framePar.qIndexDeltaChromaAC);

        offsets.BitOffsetForSegmentation = (mfxU16)localBuf.bitOffset;

        //segmentation
        bool segmentation = framePar.segmentation != NO_SEGMENTATION;
        WriteBit(localBuf, segmentation);
        if (segmentation)
        {
            // for both cases (APP_SEGMENTATION and BRC_SEGMENTATION) segmentation_params() will be completely re-written by HW accelerator
            // so just writing dummy parameters here
            WriteBit(localBuf, 0);
            WriteBit(localBuf, 0);
        }

        offsets.BitSizeForSegmentation = (mfxU16)localBuf.bitOffset - offsets.BitOffsetForSegmentation;

        // tile info
        mfxU8 minLog2TileCols = 0;
        mfxU8 maxLog2TileCols = 1;
        mfxU8 ones;

        const mfxU16 sb64Cols = (mfx::align2_value(framePar.modeInfoCols, 1 << MI_BLOCK_SIZE_LOG2)) >> MI_BLOCK_SIZE_LOG2;
        while ((MAX_TILE_WIDTH_B64 << minLog2TileCols) < sb64Cols)
        {
            minLog2TileCols ++;
        }
        while ((sb64Cols >> maxLog2TileCols) >= MIN_TILE_WIDTH_B64)
        {
            maxLog2TileCols ++;
        }
        maxLog2TileCols--;

        ones = framePar.log2TileCols - minLog2TileCols;
        while (ones--)
        {
            WriteBit(localBuf, 1);
        }
        if (framePar.log2TileCols < maxLog2TileCols)
        {
            WriteBit(localBuf, 0);
        }

        WriteBit(localBuf, framePar.log2TileRows != 0);
        if (framePar.log2TileRows != 0)
        {
            WriteBit(localBuf, framePar.log2TileRows != 1);
        }

        offsets.BitOffsetForFirstPartitionSize = (mfxU16)localBuf.bitOffset;;

        // size of compressed header (unknown so far, will be written by driver/HuC)
        WriteLiteral(localBuf, 0, 16);

        return localBuf.bitOffset;
    };

    mfxU16 PrepareFrameHeader(VP9MfxVideoParam const &par,
        mfxU8 *pBuf,
        mfxU32 bufferSizeBytes,
        Task const& task,
        VP9SeqLevelParam const &seqPar,
        BitOffsets &offsets)
    {
        if (bufferSizeBytes < VP9_MAX_UNCOMPRESSED_HEADER_SIZE + MAX_IVF_HEADER_SIZE)
        {
            return 0; // zero size of header - indication that something went wrong
        }

        BitBuffer localBuf;
        localBuf.pBuffer = pBuf;
        localBuf.bitOffset = 0;

        mfxExtVP9Param& opt = GetExtBufferRef(par);
        mfxU16 ivfHeaderSize = 0;

        if (opt.WriteIVFHeaders != MFX_CODINGOPTION_OFF)
        {
            if (task.m_insertIVFSeqHeader)
            {
                mfxStatus sts = AddSeqHeader(task.m_frameParam.width,
                                             task.m_frameParam.height,
                                             par.mfx.FrameInfo.FrameRateExtN,
                                             par.mfx.FrameInfo.FrameRateExtD,
                                             0,
                                             localBuf.pBuffer,
                                             bufferSizeBytes);
                MFX_CHECK_STS(sts);

                ivfHeaderSize += IVF_SEQ_HEADER_SIZE_BYTES;
            }

            mfxStatus sts = AddPictureHeader(localBuf.pBuffer + ivfHeaderSize, bufferSizeBytes - ivfHeaderSize);
            MFX_CHECK_STS(sts);

            ivfHeaderSize += IVF_PIC_HEADER_SIZE_BYTES;
        }

        localBuf.bitOffset += ivfHeaderSize * 8;

        mfxU16 totalBitsWritten = WriteUncompressedHeader(localBuf,
            task,
            seqPar,
            offsets);

        return (totalBitsWritten + 7) / 8;
    }
} // MfxHwVP9Encode
