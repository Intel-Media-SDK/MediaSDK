// Copyright (c) 2019-2020 Intel Corporation
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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base_legacy.h"
#include "hevcehw_base_data.h"
#include "hevcehw_base_constraints.h"
#include "hevcehw_base_parser.h"
#include "fast_copy.h"
#include <algorithm>
#include <exception>
#include <numeric>
#include <set>
#include <cmath>

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void Legacy::SetSupported(ParamSupport& blocks)
{
    auto CopyRawBuffer = [](mfxU8* pSrcBuf, mfxU16 szSrcBuf, mfxU8* pDstBuf, mfxU16& szDstBuf)
    {
        bool bCopy = pSrcBuf && pDstBuf;
        ThrowIf(bCopy && szSrcBuf > szDstBuf, MFX_ERR_NOT_ENOUGH_BUFFER);

        if (bCopy)
        {
            std::copy_n(pSrcBuf, szSrcBuf, pDstBuf);
            szDstBuf = szSrcBuf;
        }
    };

    blocks.m_mvpCopySupported.emplace_back(
        [](const mfxVideoParam* pSrc, mfxVideoParam* pDst) -> void
    {
        const auto& buf_src = *(const mfxVideoParam*)pSrc;
        auto& buf_dst = *(mfxVideoParam*)pDst;
        MFX_COPY_FIELD(IOPattern);
        MFX_COPY_FIELD(Protected);
        MFX_COPY_FIELD(AsyncDepth);
        MFX_COPY_FIELD(mfx.CodecId);
        MFX_COPY_FIELD(mfx.LowPower);
        MFX_COPY_FIELD(mfx.CodecLevel);
        MFX_COPY_FIELD(mfx.CodecProfile);
        MFX_COPY_FIELD(mfx.TargetUsage);
        MFX_COPY_FIELD(mfx.GopPicSize);
        MFX_COPY_FIELD(mfx.GopRefDist);
        MFX_COPY_FIELD(mfx.GopOptFlag);
        MFX_COPY_FIELD(mfx.IdrInterval);
        MFX_COPY_FIELD(mfx.BRCParamMultiplier);
        MFX_COPY_FIELD(mfx.RateControlMethod);
        MFX_COPY_FIELD(mfx.InitialDelayInKB);
        MFX_COPY_FIELD(mfx.BufferSizeInKB);
        MFX_COPY_FIELD(mfx.TargetKbps);
        MFX_COPY_FIELD(mfx.MaxKbps);
        MFX_COPY_FIELD(mfx.NumSlice);
        MFX_COPY_FIELD(mfx.NumRefFrame);
        MFX_COPY_FIELD(mfx.EncodedOrder);
        MFX_COPY_FIELD(mfx.FrameInfo.Shift);
        MFX_COPY_FIELD(mfx.FrameInfo.BitDepthLuma);
        MFX_COPY_FIELD(mfx.FrameInfo.BitDepthChroma);
        MFX_COPY_FIELD(mfx.FrameInfo.FourCC);
        MFX_COPY_FIELD(mfx.FrameInfo.Width);
        MFX_COPY_FIELD(mfx.FrameInfo.Height);
        MFX_COPY_FIELD(mfx.FrameInfo.CropX);
        MFX_COPY_FIELD(mfx.FrameInfo.CropY);
        MFX_COPY_FIELD(mfx.FrameInfo.CropW);
        MFX_COPY_FIELD(mfx.FrameInfo.CropH);
        MFX_COPY_FIELD(mfx.FrameInfo.FrameRateExtN);
        MFX_COPY_FIELD(mfx.FrameInfo.FrameRateExtD);
        MFX_COPY_FIELD(mfx.FrameInfo.AspectRatioW);
        MFX_COPY_FIELD(mfx.FrameInfo.AspectRatioH);
        MFX_COPY_FIELD(mfx.FrameInfo.ChromaFormat);
        MFX_COPY_FIELD(mfx.FrameInfo.PicStruct);
    });

    blocks.m_ebCopySupported[MFX_EXTBUFF_HEVC_PARAM].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtHEVCParam*)pSrc;
        auto& buf_dst = *(mfxExtHEVCParam*)pDst;
        MFX_COPY_FIELD(PicWidthInLumaSamples);
        MFX_COPY_FIELD(PicHeightInLumaSamples);
        MFX_COPY_FIELD(GeneralConstraintFlags);
        MFX_COPY_FIELD(SampleAdaptiveOffset);
        MFX_COPY_FIELD(LCUSize);
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_HEVC_TILES].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtHEVCTiles*)pSrc;
        auto& buf_dst = *(mfxExtHEVCTiles*)pDst;
        MFX_COPY_FIELD(NumTileRows);
        MFX_COPY_FIELD(NumTileColumns);
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtOpaqueSurfaceAlloc*)pSrc;
        auto& buf_dst = *(mfxExtOpaqueSurfaceAlloc*)pDst;
        MFX_COPY_FIELD(In.Surfaces);
        MFX_COPY_FIELD(In.Type);
        MFX_COPY_FIELD(In.NumSurface);
        MFX_COPY_FIELD(Out.Surfaces);
        MFX_COPY_FIELD(Out.Type);
        MFX_COPY_FIELD(Out.NumSurface);
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_AVC_REFLISTS].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtAVCRefLists*)pSrc;
        auto& buf_dst = *(mfxExtAVCRefLists*)pDst;
        MFX_COPY_FIELD(NumRefIdxL0Active);
        MFX_COPY_FIELD(NumRefIdxL1Active);
        for (mfxU32 i = 0; i < 16; i++)
        {
            MFX_COPY_FIELD(RefPicList0[i].FrameOrder);
            MFX_COPY_FIELD(RefPicList1[i].FrameOrder);
        }
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_CODING_OPTION].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtCodingOption*)pSrc;
        auto& buf_dst = *(mfxExtCodingOption*)pDst;
        MFX_COPY_FIELD(PicTimingSEI);
        MFX_COPY_FIELD(VuiNalHrdParameters);
        MFX_COPY_FIELD(NalHrdConformance);
        MFX_COPY_FIELD(AUDelimiter);
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_CODING_OPTION2].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtCodingOption2*)pSrc;
        auto& buf_dst = *(mfxExtCodingOption2*)pDst;
        MFX_COPY_FIELD(IntRefType);
        MFX_COPY_FIELD(IntRefCycleSize);
        MFX_COPY_FIELD(IntRefQPDelta);
        MFX_COPY_FIELD(MBBRC);
        MFX_COPY_FIELD(BRefType);
        MFX_COPY_FIELD(NumMbPerSlice);
        MFX_COPY_FIELD(DisableDeblockingIdc);
        MFX_COPY_FIELD(RepeatPPS);
        MFX_COPY_FIELD(MaxSliceSize);
        MFX_COPY_FIELD(ExtBRC);
        MFX_COPY_FIELD(MinQPI);
        MFX_COPY_FIELD(MaxQPI);
        MFX_COPY_FIELD(MinQPP);
        MFX_COPY_FIELD(MaxQPP);
        MFX_COPY_FIELD(MinQPB);
        MFX_COPY_FIELD(MaxQPB);
        MFX_COPY_FIELD(SkipFrame);
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_CODING_OPTION3].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtCodingOption3*)pSrc;
        auto& buf_dst = *(mfxExtCodingOption3*)pDst;
        MFX_COPY_FIELD(PRefType);
        MFX_COPY_FIELD(IntRefCycleDist);
        MFX_COPY_FIELD(EnableQPOffset);
        MFX_COPY_FIELD(GPB);
        MFX_COPY_ARRAY_FIELD(QPOffset);
        MFX_COPY_ARRAY_FIELD(NumRefActiveP);
        MFX_COPY_ARRAY_FIELD(NumRefActiveBL0);
        MFX_COPY_ARRAY_FIELD(NumRefActiveBL1);
        MFX_COPY_FIELD(QVBRQuality);
        MFX_COPY_FIELD(EnableMBQP);
        MFX_COPY_FIELD(TransformSkip);
        MFX_COPY_FIELD(TargetChromaFormatPlus1);
        MFX_COPY_FIELD(TargetBitDepthLuma);
        MFX_COPY_FIELD(TargetBitDepthChroma);
        MFX_COPY_FIELD(WinBRCMaxAvgKbps);
        MFX_COPY_FIELD(WinBRCSize);
        MFX_COPY_FIELD(EnableNalUnitType);
        MFX_COPY_FIELD(LowDelayBRC);
#if MFX_VERSION >= MFX_VERSION_NEXT
        MFX_COPY_FIELD(DeblockingAlphaTcOffset);
        MFX_COPY_FIELD(DeblockingBetaOffset);
#endif
        MFX_COPY_FIELD(BRCPanicMode);
        MFX_COPY_FIELD(ScenarioInfo);
    });
    blocks.m_ebCopyPtrs[MFX_EXTBUFF_CODING_OPTION_SPSPPS].emplace_back(
        [&](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtCodingOptionSPSPPS*)pSrc;
        auto&       buf_dst = *(mfxExtCodingOptionSPSPPS*)pDst;

        MFX_COPY_FIELD(SPSBuffer);
        MFX_COPY_FIELD(SPSBufSize);
        MFX_COPY_FIELD(PPSBuffer);
        MFX_COPY_FIELD(PPSBufSize);
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_CODING_OPTION_SPSPPS].emplace_back(
        [&](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtCodingOptionSPSPPS*)pSrc;
        auto&       buf_dst = *(mfxExtCodingOptionSPSPPS*)pDst;

        CopyRawBuffer(buf_src.SPSBuffer, buf_src.SPSBufSize, buf_dst.SPSBuffer, buf_dst.SPSBufSize);
        CopyRawBuffer(buf_src.PPSBuffer, buf_src.PPSBufSize, buf_dst.PPSBuffer, buf_dst.PPSBufSize);
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_AVC_REFLIST_CTRL].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtAVCRefListCtrl*)pSrc;
        auto& buf_dst = *(mfxExtAVCRefListCtrl*)pDst;
        MFX_COPY_FIELD(NumRefIdxL0Active);
        MFX_COPY_FIELD(NumRefIdxL1Active);
        for (mfxU32 i = 0; i < 16; i++)
        {
            MFX_COPY_FIELD(PreferredRefList[i].FrameOrder);
            MFX_COPY_FIELD(RejectedRefList[i].FrameOrder);
            MFX_COPY_FIELD(LongTermRefList[i].FrameOrder);
        }
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_AVC_TEMPORAL_LAYERS].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtAvcTemporalLayers*)pSrc;
        auto& buf_dst = *(mfxExtAvcTemporalLayers*)pDst;
        for (mfxU32 i = 0; i < 7; i++)
        {
            MFX_COPY_FIELD(Layer[i].Scale);
        }
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_ENCODER_RESET_OPTION].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtEncoderResetOption*)pSrc;
        auto& buf_dst = *(mfxExtEncoderResetOption*)pDst;
        MFX_COPY_FIELD(StartNewSequence);
    });
    blocks.m_ebCopyPtrs[MFX_EXTBUFF_CODING_OPTION_VPS].emplace_back(
        [&](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtCodingOptionVPS*)pSrc;
        auto&       buf_dst = *(mfxExtCodingOptionVPS*)pDst;

        MFX_COPY_FIELD(VPSBuffer);
        MFX_COPY_FIELD(VPSBufSize);
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_CODING_OPTION_VPS].emplace_back(
        [&](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtCodingOptionVPS*)pSrc;
        auto& buf_dst = *(mfxExtCodingOptionVPS*)pDst;

        CopyRawBuffer(buf_src.VPSBuffer, buf_src.VPSBufSize, buf_dst.VPSBuffer, buf_dst.VPSBufSize);
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_VIDEO_SIGNAL_INFO].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtVideoSignalInfo*)pSrc;
        auto& buf_dst = *(mfxExtVideoSignalInfo*)pDst;
        buf_dst = buf_src;
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_LOOKAHEAD_STAT].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtLAFrameStatistics*)pSrc;
        auto& buf_dst = *(mfxExtLAFrameStatistics*)pDst;
        MFX_COPY_FIELD(NumAlloc);
        MFX_COPY_FIELD(NumStream);
        MFX_COPY_FIELD(NumFrame);
        MFX_COPY_FIELD(FrameStat);
        MFX_COPY_FIELD(OutSurface);
    });
    blocks.m_ebCopySupported[MFX_EXTBUFF_MBQP].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        const auto& buf_src = *(const mfxExtMBQP*)pSrc;
        auto& buf_dst = *(mfxExtMBQP*)pDst;
        MFX_COPY_FIELD(NumQPAlloc);
        MFX_COPY_FIELD(QP);
    });
}

bool Legacy::IsTCBRC(const mfxVideoParam& par, mfxU16 tcbrcSupport)
{
    const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par);
    const mfxExtCodingOption& CO = ExtBuffer::Get(par);
    return (IsOn(CO3.LowDelayBRC) && tcbrcSupport && IsOff(CO.NalHrdConformance) &&
           (par.mfx.RateControlMethod  ==  MFX_RATECONTROL_VBR  ||
            par.mfx.RateControlMethod  ==  MFX_RATECONTROL_QVBR ||
            par.mfx.RateControlMethod  ==  MFX_RATECONTROL_VCM ));
}

void Legacy::SetInherited(ParamInheritance& par)
{
    par.m_mvpInheritDefault.emplace_back(
        [](const mfxVideoParam* pSrc, mfxVideoParam* pDst)
    {
        auto& parInit = *pSrc;
        auto& parReset = *pDst;

#define INHERIT_OPT(OPT) InheritOption(parInit.OPT, parReset.OPT)
#define INHERIT_BRC(OPT) { OPT tmp(parReset.mfx); InheritOption(OPT(parInit.mfx), tmp); }

        INHERIT_OPT(AsyncDepth);
        //INHERIT_OPT(mfx.BRCParamMultiplier);
        INHERIT_OPT(mfx.CodecId);
        INHERIT_OPT(mfx.CodecProfile);
        INHERIT_OPT(mfx.CodecLevel);
        INHERIT_OPT(mfx.NumThread);
        INHERIT_OPT(mfx.TargetUsage);
        INHERIT_OPT(mfx.GopPicSize);
        INHERIT_OPT(mfx.GopRefDist);
        INHERIT_OPT(mfx.GopOptFlag);
        INHERIT_OPT(mfx.IdrInterval);
        INHERIT_OPT(mfx.RateControlMethod);
        INHERIT_OPT(mfx.BufferSizeInKB);
        INHERIT_OPT(mfx.NumSlice);
        INHERIT_OPT(mfx.NumRefFrame);

        mfxU16 RC = parInit.mfx.RateControlMethod
            * (parInit.mfx.RateControlMethod == parReset.mfx.RateControlMethod);
        static const std::map<
            mfxU16 , std::function<void(const mfxVideoParam&, mfxVideoParam&)>
        > InheritBrcOpt =
        {
            {
                mfxU16(MFX_RATECONTROL_CBR)
                , [](const mfxVideoParam& parInit, mfxVideoParam& parReset)
                {
                    INHERIT_BRC(InitialDelayInKB);
                    INHERIT_BRC(TargetKbps);
                }
            }
            , {
                mfxU16(MFX_RATECONTROL_VBR)
                , [](const mfxVideoParam& parInit, mfxVideoParam& parReset)
                {
                    INHERIT_BRC(InitialDelayInKB);
                    INHERIT_BRC(TargetKbps);
                    INHERIT_BRC(MaxKbps);
                }
            }
            , {
                mfxU16(MFX_RATECONTROL_CQP)
                , [](const mfxVideoParam& parInit, mfxVideoParam& parReset)
                {
                    INHERIT_OPT(mfx.QPI);
                    INHERIT_OPT(mfx.QPP);
                    INHERIT_OPT(mfx.QPB);
                }
            }
            , {
                mfxU16(MFX_RATECONTROL_ICQ)
                , [](const mfxVideoParam& parInit, mfxVideoParam& parReset)
                {
                    INHERIT_OPT(mfx.ICQQuality);
                }
            }
            , {
                mfxU16(MFX_RATECONTROL_LA_ICQ)
                , [](const mfxVideoParam& parInit, mfxVideoParam& parReset)
                {
                    INHERIT_OPT(mfx.ICQQuality);
                }
            }
            , {
                mfxU16(MFX_RATECONTROL_VCM)
                , [](const mfxVideoParam& parInit, mfxVideoParam& parReset)
                {
                    INHERIT_BRC(InitialDelayInKB);
                    INHERIT_BRC(TargetKbps);
                    INHERIT_BRC(MaxKbps);
                }
            }
            , {
                mfxU16(MFX_RATECONTROL_QVBR)
                , [](const mfxVideoParam& parInit, mfxVideoParam& parReset)
                {
                    INHERIT_BRC(InitialDelayInKB);
                    INHERIT_BRC(TargetKbps);
                    INHERIT_BRC(MaxKbps);
                }
            }
        };
        auto itInheritBrcOpt = InheritBrcOpt.find(RC);

        if (itInheritBrcOpt != InheritBrcOpt.end())
            itInheritBrcOpt->second(parInit, parReset);

        INHERIT_OPT(mfx.FrameInfo.FourCC);
        INHERIT_OPT(mfx.FrameInfo.Width);
        INHERIT_OPT(mfx.FrameInfo.Height);
        INHERIT_OPT(mfx.FrameInfo.CropX);
        INHERIT_OPT(mfx.FrameInfo.CropY);
        INHERIT_OPT(mfx.FrameInfo.CropW);
        INHERIT_OPT(mfx.FrameInfo.CropH);
        INHERIT_OPT(mfx.FrameInfo.FrameRateExtN);
        INHERIT_OPT(mfx.FrameInfo.FrameRateExtD);
        INHERIT_OPT(mfx.FrameInfo.AspectRatioW);
        INHERIT_OPT(mfx.FrameInfo.AspectRatioH);

#undef INHERIT_OPT
#undef INHERIT_BRC
    });

#define INIT_EB(TYPE)\
    if (!pSrc || !pDst) return;\
    auto& ebInit = *(TYPE*)pSrc;\
    auto& ebReset = *(TYPE*)pDst;
#define INHERIT_OPT(OPT) InheritOption(ebInit.OPT, ebReset.OPT);

    par.m_ebInheritDefault[MFX_EXTBUFF_HEVC_PARAM].emplace_back(
        [](const mfxVideoParam& /*parInit*/
            , const mfxExtBuffer* pSrc
            , const mfxVideoParam& /*parReset*/
            , mfxExtBuffer* pDst)
    {
        INIT_EB(mfxExtHEVCParam);
        INHERIT_OPT(GeneralConstraintFlags);
        INHERIT_OPT(SampleAdaptiveOffset);
        INHERIT_OPT(LCUSize);
    });

    par.m_ebInheritDefault[MFX_EXTBUFF_HEVC_TILES].emplace_back(
        [](const mfxVideoParam& /*parInit*/
            , const mfxExtBuffer* pSrc
            , const mfxVideoParam& /*parReset*/
            , mfxExtBuffer* pDst)
    {
        INIT_EB(mfxExtHEVCTiles);
        INHERIT_OPT(NumTileColumns);
        INHERIT_OPT(NumTileRows);
    });

    par.m_ebInheritDefault[MFX_EXTBUFF_CODING_OPTION].emplace_back(
        [](const mfxVideoParam& /*parInit*/
            , const mfxExtBuffer* pSrc
            , const mfxVideoParam& /*parReset*/
            , mfxExtBuffer* pDst)
    {
        INIT_EB(mfxExtCodingOption);
        INHERIT_OPT(VuiNalHrdParameters);
        INHERIT_OPT(NalHrdConformance);
        INHERIT_OPT(PicTimingSEI);
    });

    par.m_ebInheritDefault[MFX_EXTBUFF_CODING_OPTION2].emplace_back(
        [](const mfxVideoParam& /*parInit*/
            , const mfxExtBuffer* pSrc
            , const mfxVideoParam& /*parReset*/
            , mfxExtBuffer* pDst)
    {
        INIT_EB(mfxExtCodingOption2);
        INHERIT_OPT(IntRefType);
        INHERIT_OPT(IntRefCycleSize);
        INHERIT_OPT(IntRefQPDelta);
        INHERIT_OPT(MBBRC);
        INHERIT_OPT(BRefType);
        INHERIT_OPT(NumMbPerSlice);
        INHERIT_OPT(MinQPI);
        INHERIT_OPT(MinQPP);
        INHERIT_OPT(MinQPB);
        INHERIT_OPT(MaxQPI);
        INHERIT_OPT(MaxQPP);
        INHERIT_OPT(MaxQPB);
    });

    par.m_ebInheritDefault[MFX_EXTBUFF_CODING_OPTION3].emplace_back(
        [this](const mfxVideoParam& parInit
            , const mfxExtBuffer* pSrc
            , const mfxVideoParam& parReset
            , mfxExtBuffer* pDst)
    {
        INIT_EB(mfxExtCodingOption3);
        INHERIT_OPT(LowDelayBRC);
        INHERIT_OPT(IntRefCycleDist);
        INHERIT_OPT(PRefType);
        INHERIT_OPT(GPB);
        INHERIT_OPT(TransformSkip);
        INHERIT_OPT(TargetChromaFormatPlus1);
        INHERIT_OPT(TargetBitDepthLuma);
        INHERIT_OPT(TargetBitDepthChroma);
        INHERIT_OPT(WinBRCMaxAvgKbps);
        INHERIT_OPT(WinBRCSize);
        INHERIT_OPT(EnableMBQP);

        mfxU16 RC = parInit.mfx.RateControlMethod
            * (parInit.mfx.RateControlMethod == parReset.mfx.RateControlMethod);

        if (RC == MFX_RATECONTROL_QVBR)
            INHERIT_OPT(QVBRQuality);

        if (parInit.mfx.TargetUsage != parReset.mfx.TargetUsage)
        {
            auto maxRef = m_GetMaxRef(parReset);
            auto ClipRefP   = [maxRef](mfxU16 ref) { return std::min<mfxU16>(ref, std::get<P>(maxRef)); };
            auto ClipRefBL0 = [maxRef](mfxU16 ref) { return std::min<mfxU16>(ref, std::get<BL0>(maxRef)); };
            auto ClipRefBL1 = [maxRef](mfxU16 ref) { return std::min<mfxU16>(ref, std::get<BL1>(maxRef)); };

            std::transform(ebInit.NumRefActiveP, ebInit.NumRefActiveP + 8, ebReset.NumRefActiveP, ClipRefP);
            std::transform(ebInit.NumRefActiveBL0, ebInit.NumRefActiveBL0 + 8, ebReset.NumRefActiveBL0, ClipRefBL0);
            std::transform(ebInit.NumRefActiveBL1, ebInit.NumRefActiveBL1 + 8, ebReset.NumRefActiveBL1, ClipRefBL1);
        }
        else
        {
            InheritOptions(ebInit.NumRefActiveP, ebInit.NumRefActiveP + 8, ebReset.NumRefActiveP);
            InheritOptions(ebInit.NumRefActiveBL0, ebInit.NumRefActiveBL0 + 8, ebReset.NumRefActiveBL0);
            InheritOptions(ebInit.NumRefActiveBL1, ebInit.NumRefActiveBL1 + 8, ebReset.NumRefActiveBL1);
        }
    });

    par.m_ebInheritDefault[MFX_EXTBUFF_VIDEO_SIGNAL_INFO].emplace_back(
        [](const mfxVideoParam& /*parInit*/
            , const mfxExtBuffer* pSrc
            , const mfxVideoParam& /*parReset*/
            , mfxExtBuffer* pDst)
    {
        INIT_EB(mfxExtVideoSignalInfo);
        INHERIT_OPT(VideoFormat);
        INHERIT_OPT(ColourPrimaries);
        INHERIT_OPT(TransferCharacteristics);
        INHERIT_OPT(MatrixCoefficients);
        INHERIT_OPT(VideoFullRange);
        INHERIT_OPT(ColourDescriptionPresent);
    });
#undef INIT_EB
#undef INHERIT_OPT
}

void Legacy::Query0(const FeatureBlocks& blocks, TPushQ0 Push)
{
    using namespace std::placeholders;
    Push( BLK_Query0, std::bind(&Legacy::CheckQuery0, this, std::cref(blocks), _1));
}

void Legacy::Query1NoCaps(const FeatureBlocks& blocks, TPushQ1 Push)
{
    Push(BLK_SetDefaultsCallChain,
        [this](const mfxVideoParam&, mfxVideoParam&, StorageRW& strg) -> mfxStatus
    {
        auto& defaults = Glob::Defaults::GetOrConstruct(strg);
        auto& bSet = defaults.SetForFeature[GetID()];
        MFX_CHECK(!bSet, MFX_ERR_NONE);

        PushDefaults(defaults);

        VideoCORE * pCore = &Glob::VideoCore::Get(strg);
        defaults.RunFastCopyWrapper.Push([pCore](Defaults::TRunFastCopyWrapper::TExt
            , mfxFrameSurface1 &surfDst
            , mfxU16 dstMemType
            , mfxFrameSurface1 &surfSrc
            , mfxU16 srcMemType) -> mfxStatus
            {
                return pCore->DoFastCopyWrapper(&surfDst, dstMemType, &surfSrc, srcMemType);
            });

        bSet = true;

        m_pQNCDefaults = &defaults;
        m_hw = Glob::VideoCore::Get(strg).GetHWType();

        return MFX_ERR_NONE;
    });

    Push(BLK_PreCheckCodecId,
        [this](const mfxVideoParam& in, mfxVideoParam&, StorageRW& /*strg*/) -> mfxStatus
    {
        return m_pQNCDefaults->PreCheckCodecId(in);
    });

    Push(BLK_PreCheckChromaFormat,
        [this](const mfxVideoParam& in, mfxVideoParam&, StorageW&) -> mfxStatus
    {
        return m_pQNCDefaults->PreCheckChromaFormat(in);
    });

    Push(BLK_PreCheckExtBuffers
        , [this, &blocks](const mfxVideoParam& in, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckBuffers(blocks, in, &out);
    });

    Push(BLK_CopyConfigurable
        , [this, &blocks](const mfxVideoParam& in, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CopyConfigurable(blocks, in, out);
    });

    Push(BLK_SetLowPowerDefault
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW& /*strg*/) -> mfxStatus
    {
        auto lowPower = m_pQNCDefaults->GetLowPower(out, m_hw);
        bool bChanged = out.mfx.LowPower && out.mfx.LowPower != lowPower;

        out.mfx.LowPower = lowPower;

        MFX_CHECK(!bChanged, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        return MFX_ERR_NONE;
    });

    Push(BLK_SetGUID
        , [](const mfxVideoParam&, mfxVideoParam& out, StorageRW& strg) -> mfxStatus
    {
        MFX_CHECK(!strg.Contains(Glob::GUID::Key), MFX_ERR_NONE);

        if (strg.Contains(Glob::RealState::Key))
        {
            //don't change GUID in Reset
            auto& initPar = Glob::RealState::Get(strg);
            strg.Insert(Glob::GUID::Key, make_storable<GUID>(Glob::GUID::Get(initPar)));
            return MFX_ERR_NONE;
        }

        VideoCORE& core = Glob::VideoCore::Get(strg);
        auto pGUID = make_storable<GUID>();
        auto& defaults = Glob::Defaults::Get(strg);
        EncodeCapsHevc fakeCaps;
        Defaults::Param defPar(out, fakeCaps, core.GetHWType(), defaults);
        fakeCaps.MaxEncodedBitDepth = (defPar.hw >= MFX_HW_KBL);
        fakeCaps.YUV422ReconSupport = (defPar.hw >= MFX_HW_ICL) && !IsOn(out.mfx.LowPower);
        fakeCaps.YUV444ReconSupport = (defPar.hw >= MFX_HW_ICL) && IsOn(out.mfx.LowPower);

        MFX_CHECK(defaults.GetGUID(defPar, *pGUID), MFX_ERR_NONE);
        strg.Insert(Glob::GUID::Key, std::move(pGUID));

        return MFX_ERR_NONE;
    });
}

void Legacy::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_CheckHeaders
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW& strg) -> mfxStatus
    {
        mfxStatus sts = MFX_ERR_NONE;
        m_pQWCDefaults.reset(
            new Defaults::Param(
                out
                , Glob::EncodeCaps::Get(strg)
                , Glob::VideoCore::Get(strg).GetHWType()
                , Glob::Defaults::Get(strg)));

        if (strg.Contains(Glob::SPS::Key))
            sts = CheckSPS(Glob::SPS::Get(strg), m_pQWCDefaults->caps, m_pQWCDefaults->hw);

        if (!sts && strg.Contains(Glob::PPS::Key))
            sts = CheckPPS(Glob::PPS::Get(strg), m_pQWCDefaults->caps, m_pQWCDefaults->hw);

        return sts;
    });

    Push(BLK_CheckLCUSize
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW& ) -> mfxStatus
    {
        return m_pQWCDefaults->base.CheckLCUSize(*m_pQWCDefaults, out);
    });

    Push(BLK_CheckFormat
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        auto sts = m_pQWCDefaults->base.CheckFourCC(*m_pQWCDefaults, out);
        MFX_CHECK_STS(sts);
        sts = m_pQWCDefaults->base.CheckInputFormatByFourCC(*m_pQWCDefaults, out);
        MFX_CHECK_STS(sts);
        sts = m_pQWCDefaults->base.CheckTargetChromaFormat(*m_pQWCDefaults, out);
        MFX_CHECK_STS(sts);
        sts = m_pQWCDefaults->base.CheckTargetBitDepth(*m_pQWCDefaults, out);
        MFX_CHECK_STS(sts);
        return m_pQWCDefaults->base.CheckFourCCByTargetFormat(*m_pQWCDefaults, out);
    });

    Push(BLK_CheckLowDelayBRC
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return m_pQWCDefaults->base.CheckLowDelayBRC(*m_pQWCDefaults, out);
    });

    Push(BLK_CheckLevel
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return m_pQWCDefaults->base.CheckLevel(*m_pQWCDefaults, out);
    });

    Push(BLK_CheckSurfSize
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return m_pQWCDefaults->base.CheckSurfSize(*m_pQWCDefaults, out);
    });

    Push(BLK_CheckCodedPicSize
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckCodedPicSize(out, *m_pQWCDefaults);
    });

    Push(BLK_CheckTiles
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckTiles(out, *m_pQWCDefaults);
    });

    Push(BLK_CheckTU
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckTU(out, m_pQWCDefaults->caps);
    });

    Push(BLK_CheckTemporalLayers
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckTemporalLayers(out);
    });

    Push(BLK_CheckGopRefDist
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckGopRefDist(out, m_pQWCDefaults->caps);
    });

    Push(BLK_CheckNumRefFrame
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckNumRefFrame(out, *m_pQWCDefaults);
    });

    Push(BLK_CheckIOPattern
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckIOPattern(out);
    });

    Push(BLK_CheckBRC
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckBRC(out, *m_pQWCDefaults);
    });

    Push(BLK_CheckCrops
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckCrops(out, *m_pQWCDefaults);
    });

    Push(BLK_CheckShift
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckShift(out, ExtBuffer::Get(out));
    });

    Push(BLK_CheckFrameRate
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckFrameRate(out);
    });

    Push(BLK_CheckSlices
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckSlices(out, *m_pQWCDefaults);
    });

    Push(BLK_CheckBPyramid
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckBPyramid(out, *m_pQWCDefaults);
    });

    Push(BLK_CheckPPyramid
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckPPyramid(out);
    });

    Push(BLK_CheckIntraRefresh
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckIntraRefresh(out, *m_pQWCDefaults);
    });

    Push(BLK_CheckSkipFrame
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckSkipFrame(out);
    });

    Push(BLK_CheckGPB
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckGPB(out);
    });

    Push(BLK_CheckESPackParam
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckESPackParam(out, m_pQWCDefaults->hw);
    });

    Push(BLK_CheckNumRefActive
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return m_pQWCDefaults->base.CheckNumRefActive(*m_pQWCDefaults, out);
    });

    Push(BLK_CheckSAO
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return m_pQWCDefaults->base.CheckSAO(*m_pQWCDefaults, out);
    });

    Push(BLK_CheckProfile
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return m_pQWCDefaults->base.CheckProfile(*m_pQWCDefaults, out);
    });

    Push(BLK_CheckLevelConstraints
        , [this](const mfxVideoParam&, mfxVideoParam& out, StorageW&) -> mfxStatus
    {
        return CheckLevelConstraints(out, *m_pQWCDefaults);
    });
}

void Legacy::QueryIOSurf(const FeatureBlocks& blocks, TPushQIS Push)
{
    Push(BLK_CheckIOPattern
        , [](const mfxVideoParam& par, mfxFrameAllocRequest&, StorageRW&) -> mfxStatus
    {
        bool check_result = Check<mfxU16
            , MFX_IOPATTERN_IN_VIDEO_MEMORY
            , MFX_IOPATTERN_IN_SYSTEM_MEMORY
            , MFX_IOPATTERN_IN_OPAQUE_MEMORY
            >
            (par.IOPattern);

        MFX_CHECK(!check_result, MFX_ERR_INVALID_VIDEO_PARAM);

        return MFX_ERR_NONE;
    });

    Push(BLK_CheckVideoParam
        , [&blocks](const mfxVideoParam& par, mfxFrameAllocRequest&, StorageRW& strg) -> mfxStatus
    {
        mfxStatus sts = MFX_ERR_NONE;
        auto pTmpPar = make_storable<ExtBuffer::Param<mfxVideoParam>>(par);

        auto& queryNC = FeatureBlocks::BQ<FeatureBlocks::BQ_Query1NoCaps>::Get(blocks);
        sts = RunBlocks(CheckGE<mfxStatus, MFX_ERR_NONE>, queryNC, par, *pTmpPar, strg);
        MFX_CHECK(sts != MFX_ERR_UNSUPPORTED, MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(sts >= MFX_ERR_NONE, sts);

        auto& queryWC = FeatureBlocks::BQ<FeatureBlocks::BQ_Query1WithCaps>::Get(blocks);
        sts = RunBlocks(CheckGE<mfxStatus, MFX_ERR_NONE>, queryWC, par, *pTmpPar, strg);
        MFX_CHECK(sts != MFX_ERR_UNSUPPORTED, MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(sts >= MFX_ERR_NONE, sts);

        strg.Insert(Glob::VideoParam::Key, std::move(pTmpPar));

        return MFX_ERR_NONE;
    });

    Push(BLK_SetDefaults
        , [&blocks](const mfxVideoParam&, mfxFrameAllocRequest&, StorageRW& strg) -> mfxStatus
    {
        ExtBuffer::Param<mfxVideoParam>& par = Glob::VideoParam::Get(strg);
        StorageRW local;

        auto& qSD = FeatureBlocks::BQ<FeatureBlocks::BQ_SetDefaults>::Get(blocks);
        return RunBlocks(IgnoreSts, qSD, par, strg, local);
    });

    Push(BLK_SetFrameAllocRequest
        , [this](const mfxVideoParam&, mfxFrameAllocRequest& req, StorageRW& strg) -> mfxStatus
    {
        ExtBuffer::Param<mfxVideoParam>& par = Glob::VideoParam::Get(strg);
        auto fourCC = par.mfx.FrameInfo.FourCC;

        req.Info = par.mfx.FrameInfo;
        SetDefault(req.Info.Shift, (fourCC == MFX_FOURCC_P010 || fourCC == MFX_FOURCC_Y210));

        bool bSYS = par.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY;
        bool bVID = par.IOPattern == MFX_IOPATTERN_IN_VIDEO_MEMORY;
        bool bOPQ = par.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY;

        req.Type =
            bSYS * (MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_EXTERNAL_FRAME)
            + bVID * (MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_EXTERNAL_FRAME)
            + bOPQ * (MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_OPAQUE_FRAME);
        MFX_CHECK(req.Type, MFX_ERR_INVALID_VIDEO_PARAM);

        req.NumFrameMin = GetMaxRaw(par);
        req.NumFrameSuggested = req.NumFrameMin;

        return MFX_ERR_NONE;
    });
}

void Legacy::SetDefaults(const FeatureBlocks& /*blocks*/, TPushSD Push)
{
    Push(BLK_SetDefaults
        , [this](mfxVideoParam& par, StorageW& strg, StorageRW&)
    {
        auto& core = Glob::VideoCore::Get(strg);
        auto& caps = Glob::EncodeCaps::Get(strg);
        auto& defchain = Glob::Defaults::Get(strg);
        SetDefaults(par, Defaults::Param(par, caps, core.GetHWType(), defchain), core.IsExternalFrameAllocator());
    });
}

void Legacy::InitExternal(const FeatureBlocks& blocks, TPushIE Push)
{
    Push(BLK_SetGUID
        , [&blocks](const mfxVideoParam& in, StorageRW& strg, StorageRW&) -> mfxStatus
    {
        const auto& query = FeatureBlocks::BQ<FeatureBlocks::BQ_Query1NoCaps>::Get(blocks);
        mfxStatus sts = MFX_ERR_NONE;
        auto pPar = make_storable<ExtBuffer::Param<mfxVideoParam>>(in);
        ExtBuffer::Param<mfxVideoParam>& par = *pPar;

        sts = RunBlocks(CheckGE<mfxStatus, MFX_ERR_NONE>, query, in, par, strg);
        MFX_CHECK(sts != MFX_ERR_UNSUPPORTED, MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(sts >= MFX_ERR_NONE, sts);

        strg.Insert(Glob::VideoParam::Key, std::move(pPar));

        return sts;
    });

    Push(BLK_CheckVideoParam
        , [&blocks](const mfxVideoParam& in, StorageRW& strg, StorageRW&) -> mfxStatus
    {
        const auto& query = FeatureBlocks::BQ<FeatureBlocks::BQ_Query1WithCaps>::Get(blocks);
        mfxStatus sts = MFX_ERR_NONE;
        ExtBuffer::Param<mfxVideoParam>& par = Glob::VideoParam::Get(strg);

        sts = RunBlocks(CheckGE<mfxStatus, MFX_ERR_NONE>, query, in, par, strg);
        MFX_CHECK(sts != MFX_ERR_UNSUPPORTED, MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(sts >= MFX_ERR_NONE, sts);

        MFX_CHECK(in.mfx.FrameInfo.Width  == par.mfx.FrameInfo.Width
               && in.mfx.FrameInfo.Height == par.mfx.FrameInfo.Height, MFX_ERR_INVALID_VIDEO_PARAM);

        return sts;
    });

    Push(BLK_SetDefaults
        , [&blocks](const mfxVideoParam&, StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(strg);

        for (auto& eb : blocks.m_ebCopySupported)
            par.NewEB(eb.first, false);

        auto& qSD = FeatureBlocks::BQ<FeatureBlocks::BQ_SetDefaults>::Get(blocks);
        return RunBlocks(IgnoreSts, qSD, par, strg, local);
    });
}

void Legacy::InitInternal(const FeatureBlocks& /*blocks*/, TPushII Push)
{
    Push(BLK_SetReorder
        , [this](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        using namespace std::placeholders;
        auto& par = Glob::VideoParam::Get(strg);

        auto pReorderer = make_storable<Reorderer>();

        pReorderer->BufferSize = par.mfx.GopRefDist - 1;
        pReorderer->MaxReorder = par.mfx.GopRefDist - 1;
        pReorderer->DPB        = &m_prevTask.DPB.After;

        pReorderer->Push(
            [&](Reorderer::TExt, const DpbArray& DPB, TTaskIt begin, TTaskIt end, bool bFlush)
        {
            auto IsIdrFrame = [](TItWrap::reference fi) { return IsIdr(fi.FrameType); };
            auto newEnd = std::find_if(TItWrap(begin), TItWrap(end), IsIdrFrame);

            bFlush |= (newEnd != begin && newEnd != end);

            return Reorder(par, DPB, TItWrap(begin), newEnd, bFlush).it;
        });

        strg.Insert(Glob::Reorder::Key, std::move(pReorderer));

        return MFX_ERR_NONE;
    });

    Push(BLK_SetVPS
        , [this](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        MFX_CHECK(!strg.Contains(Glob::VPS::Key), MFX_ERR_NONE);
        auto dflts = GetRTDefaults(strg);

        auto sts = dflts.base.GetVPS(dflts, Glob::VPS::GetOrConstruct(strg));
        MFX_CHECK_STS(sts);

        return MFX_ERR_NONE;
    });

    Push(BLK_SetSPS
        , [this](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        if (!strg.Contains(Glob::SPS::Key))
        {
            auto pSPS = make_storable<SPS>();
            auto dflts = GetRTDefaults(strg);

            auto sts = dflts.base.GetSPS(dflts, Glob::VPS::Get(strg), *pSPS);
            MFX_CHECK_STS(sts);

            strg.Insert(Glob::SPS::Key, std::move(pSPS));
        }

        if (strg.Contains(Glob::RealState::Key))
        {
            auto& hint = Glob::ResetHint::Get(strg);
            const SPS& oldSPS = Glob::SPS::Get(Glob::RealState::Get(strg));
            SPS& newSPS = Glob::SPS::Get(strg);

            SPS oldSPScopy = oldSPS;
            std::copy(newSPS.strps, newSPS.strps + Size(newSPS.strps), oldSPScopy.strps);
            oldSPScopy.num_short_term_ref_pic_sets = newSPS.num_short_term_ref_pic_sets;

            if (!oldSPS.vui_parameters_present_flag)
                oldSPScopy.vui = newSPS.vui;

            bool bSPSChanged = !!memcmp(&newSPS, &oldSPScopy, sizeof(SPS));

            hint.Flags |= RF_SPS_CHANGED * (bSPSChanged || (hint.Flags & RF_IDR_REQUIRED));
        }

        return CheckSPS(Glob::SPS::Get(strg)
            , Glob::EncodeCaps::Get(strg)
            , Glob::VideoCore::Get(strg).GetHWType());
    });

    Push(BLK_NestSTRPS
        , [](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        MFX_CHECK(
            strg.Contains(Glob::RealState::Key)
            && !(Glob::ResetHint::Get(strg).Flags & RF_SPS_CHANGED)
            , MFX_ERR_NONE);

        SPS& newSPS = Glob::SPS::Get(strg);
        const SPS& oldSPS = Glob::SPS::Get(Glob::RealState::Get(strg));

        //sacrifice STRPS optimization at Reset to avoid IDR insertion
        newSPS.num_short_term_ref_pic_sets = oldSPS.num_short_term_ref_pic_sets;
        std::copy(oldSPS.strps, oldSPS.strps + Size(oldSPS.strps), newSPS.strps);

        return MFX_ERR_NONE;
    });

    Push(BLK_SetSTRPS
        , [this](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        SPS& sps = Glob::SPS::Get(strg);

        MFX_CHECK(
            !sps.num_short_term_ref_pic_sets
            && !(strg.Contains(Glob::RealState::Key)
                && !(Glob::ResetHint::Get(strg).Flags & RF_SPS_CHANGED))
            , MFX_ERR_NONE);

        auto dflts = GetRTDefaults(strg);

        MFX_CHECK(!dflts.base.GetNonStdReordering(dflts), MFX_ERR_NONE);

        SetSTRPS(dflts, sps, Glob::Reorder::Get(strg));

        return MFX_ERR_NONE;
    });


    Push(BLK_SetPPS
        , [this](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        if (!strg.Contains(Glob::PPS::Key))
        {
            std::unique_ptr<MakeStorable<PPS>> pPPS(new MakeStorable<PPS>);
            auto dflts = GetRTDefaults(strg);

            auto sts = dflts.base.GetPPS(dflts, Glob::SPS::Get(strg), *pPPS);
            MFX_CHECK_STS(sts);

            strg.Insert(Glob::PPS::Key, std::move(pPPS));
        }

        if (strg.Contains(Glob::RealState::Key))
        {
            const PPS& oldPPS = Glob::PPS::Get(Glob::RealState::Get(strg));
            PPS& newPPS = Glob::PPS::Get(strg);

            if (memcmp(&oldPPS, &newPPS, sizeof(PPS)))
                Glob::ResetHint::Get(strg).Flags |= RF_PPS_CHANGED;
        }

        return CheckPPS(Glob::PPS::Get(strg)
            , Glob::EncodeCaps::Get(strg)
            , Glob::VideoCore::Get(strg).GetHWType());
    });

    Push(BLK_SetSlices
        , [this](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        auto dflts = GetRTDefaults(strg);
        dflts.base.GetSlices(dflts, Glob::SliceInfo::GetOrConstruct(strg));
        return MFX_ERR_NONE;
    });

    Push(BLK_SetRawInfo
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(strg);
        mfxFrameAllocRequest raw = {};

        raw.Info = par.mfx.FrameInfo;
        auto& rawInfo = Tmp::RawInfo::GetOrConstruct(local, raw);
        SetDefault(rawInfo.NumFrameMin, GetMaxRaw(par));
        SetDefault(rawInfo.Type
            , mfxU16(MFX_MEMTYPE_FROM_ENCODE
                | MFX_MEMTYPE_DXVA2_DECODER_TARGET
                | MFX_MEMTYPE_INTERNAL_FRAME));

        return MFX_ERR_NONE;
    });
}

void Legacy::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_AllocRaw
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        mfxStatus sts = MFX_ERR_NONE;
        auto& par = Glob::VideoParam::Get(strg);
        const mfxExtCodingOption2& CO2 = ExtBuffer::Get(par);
        auto& rawInfo = Tmp::RawInfo::Get(local);
        auto AllocRaw = [&](mfxU16 NumFrameMin)
        {
            std::unique_ptr<IAllocation> pAlloc(Tmp::MakeAlloc::Get(local)(Glob::VideoCore::Get(strg)));
            mfxFrameAllocRequest req = rawInfo;
            req.NumFrameMin = NumFrameMin;

            sts = pAlloc->Alloc(req, true);
            MFX_CHECK_STS(sts);

            strg.Insert(Glob::AllocRaw::Key, std::move(pAlloc));

            return MFX_ERR_NONE;
        };

        if (par.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
        {
            sts = AllocRaw(rawInfo.NumFrameMin);
            MFX_CHECK_STS(sts);
        }
        else if (par.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
        {
            const mfxExtOpaqueSurfaceAlloc& osa = ExtBuffer::Get(par);
            std::unique_ptr<IAllocation> pAllocOpq(Tmp::MakeAlloc::Get(local)(Glob::VideoCore::Get(strg)));

            sts = pAllocOpq->AllocOpaque(par.mfx.FrameInfo, osa.In.Type, osa.In.Surfaces, osa.In.NumSurface);
            MFX_CHECK_STS(sts);

            strg.Insert(Glob::AllocOpq::Key, std::move(pAllocOpq));

            if (osa.In.Type & MFX_MEMTYPE_SYSTEM_MEMORY)
            {
                sts = AllocRaw(osa.In.NumSurface);
                MFX_CHECK_STS(sts);
            }
        }

        bool bSkipFramesMode = (
            (IsSWBRC(par, &CO2)
                && (par.mfx.RateControlMethod == MFX_RATECONTROL_CBR || par.mfx.RateControlMethod == MFX_RATECONTROL_VBR))
            || (CO2.SkipFrame == MFX_SKIPFRAME_INSERT_DUMMY))
            && !strg.Contains(Glob::AllocRaw::Key);

        if (bSkipFramesMode)
        {
            sts = AllocRaw(GetMaxBS(par));
            MFX_CHECK_STS(sts);
        }

        return sts;
    });

    Push(BLK_AllocBS
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        mfxStatus sts = MFX_ERR_NONE;
        auto& par = Glob::VideoParam::Get(strg);
        std::unique_ptr<IAllocation> pAlloc(Tmp::MakeAlloc::Get(local)(Glob::VideoCore::Get(strg)));

        MFX_CHECK(local.Contains(Tmp::BSAllocInfo::Key), MFX_ERR_UNDEFINED_BEHAVIOR);
        auto& req = Tmp::BSAllocInfo::Get(local);

        SetDefault(req.NumFrameMin, GetMaxBS(par));
        SetDefault(req.Type
            , mfxU16(MFX_MEMTYPE_FROM_ENCODE
            | MFX_MEMTYPE_DXVA2_DECODER_TARGET
            | MFX_MEMTYPE_INTERNAL_FRAME));

        mfxU32 minBS = GetMinBsSize(par, ExtBuffer::Get(par), ExtBuffer::Get(par), ExtBuffer::Get(par));

        if (mfxU32(req.Info.Width * req.Info.Height) < minBS)
        {
            MFX_CHECK(req.Info.Width != 0, MFX_ERR_UNDEFINED_BEHAVIOR);
            req.Info.Height = (mfxU16)CeilDiv<mfxU32>(minBS, req.Info.Width);
        }

        sts = pAlloc->Alloc(req, false);
        MFX_CHECK_STS(sts);

        strg.Insert(Glob::AllocBS::Key, std::move(pAlloc));

        return sts;
    });

    Push(BLK_AllocMBQP
        , [this](StorageRW& strg, StorageRW& local) -> mfxStatus
    {
        mfxStatus sts = MFX_ERR_NONE;
        auto& par = Glob::VideoParam::Get(strg);
        const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par);
        auto& core = Glob::VideoCore::Get(strg);

        MFX_CHECK(IsOn(CO3.EnableMBQP) && core.GetVAType() != MFX_HW_VAAPI, MFX_ERR_NONE);

        MFX_CHECK(local.Contains(Tmp::MBQPAllocInfo::Key), MFX_ERR_UNDEFINED_BEHAVIOR);
        auto& req = Tmp::MBQPAllocInfo::Get(local);

        SetDefault(req.NumFrameMin, GetMaxBS(par));
        SetDefault(req.Type
            , mfxU16(MFX_MEMTYPE_FROM_ENCODE
                | MFX_MEMTYPE_DXVA2_DECODER_TARGET
                | MFX_MEMTYPE_INTERNAL_FRAME));

        std::tie(sts, m_CUQPBlkW, m_CUQPBlkH) = GetCUQPMapBlockSize(
            par.mfx.FrameInfo.Width
            , par.mfx.FrameInfo.Height
            , req.Info.Width
            , req.Info.Height);
        MFX_CHECK_STS(sts);

        // need LCU aligned width for the buffer for proper averaging
        const mfxExtHEVCParam& hpar = ExtBuffer::Get(par);
        mfxU16 numVal = mfxU16(hpar.LCUSize / m_CUQPBlkW);
        if (numVal > 1)
            req.Info.Width = (req.Info.Width + (numVal - 1)) & ~(numVal - 1);

        std::unique_ptr<IAllocation> pAlloc(Tmp::MakeAlloc::Get(local)(core));
        sts = pAlloc->Alloc(req, true);
        MFX_CHECK_STS(sts);

        strg.Insert(Glob::AllocMBQP::Key, std::move(pAlloc));

        return sts;
    });

    Push(BLK_ResetState
        , [this](StorageRW& /*strg*/, StorageRW& /*local*/) -> mfxStatus
    {
        ResetState();
        return MFX_ERR_NONE;
    });
}

void Legacy::Reset(const FeatureBlocks& blocks, TPushR Push)
{
    Push(BLK_ResetInit
        , [this, &blocks](
            const mfxVideoParam& par
            , StorageRW& global
            , StorageRW& local) -> mfxStatus
    {
        mfxStatus wrn = MFX_ERR_NONE;
        auto& init = Glob::RealState::Get(global);
        if (par.NumExtParam != 0)
            MFX_CHECK(std::none_of(par.ExtParam, par.ExtParam + par.NumExtParam, [](mfxExtBuffer* buf) { return buf == nullptr; }), MFX_ERR_NULL_PTR);
        auto pParNew = make_storable<ExtBuffer::Param<mfxVideoParam>>(par);
        ExtBuffer::Param<mfxVideoParam>& parNew = *pParNew;
        auto& parOld = Glob::VideoParam::Get(init);
        auto& core = Glob::VideoCore::Get(init);

        global.Insert(Glob::ResetHint::Key, make_storable<ResetHint>(ResetHint{}));
        auto& hint = Glob::ResetHint::Get(global);

        const mfxExtEncoderResetOption* pResetOpt = ExtBuffer::Get(par);

        hint.Flags = RF_IDR_REQUIRED * (pResetOpt && IsOn(pResetOpt->StartNewSequence));


        m_GetMaxRef = [&](const mfxVideoParam& par)
        {
            auto& def = Glob::Defaults::Get(init);
            auto hw = core.GetHWType();
            auto& caps = Glob::EncodeCaps::Get(init);
            return def.GetMaxNumRef(Defaults::Param(par, caps, hw, def));
        };

        std::for_each(std::begin(blocks.m_ebCopySupported)
            , std::end(blocks.m_ebCopySupported)
            , [&](decltype(*std::begin(blocks.m_ebCopySupported)) eb) { parNew.NewEB(eb.first, false); });

        std::for_each(std::begin(blocks.m_mvpInheritDefault)
            , std::end(blocks.m_mvpInheritDefault)
            , [&](decltype(*std::begin(blocks.m_mvpInheritDefault)) inherit) { inherit(&parOld, &parNew); });


        std::for_each(std::begin(blocks.m_ebInheritDefault)
            , std::end(blocks.m_ebInheritDefault)
            , [&](decltype(*std::begin(blocks.m_ebInheritDefault)) eb)
        {
            auto pEbNew = ExtBuffer::Get(parNew, eb.first);
            auto pEbOld = ExtBuffer::Get(parOld, eb.first);

            MFX_CHECK(pEbNew && pEbOld, MFX_ERR_NONE);

            std::for_each(std::begin(eb.second)
                , std::end(eb.second)
                , [&](decltype(*std::begin(eb.second)) inherit) { inherit(parOld, pEbOld, parNew, pEbNew); });

            return MFX_ERR_NONE;
        });

        auto& qInitExternal = FeatureBlocks::BQ<FeatureBlocks::BQ_InitExternal>::Get(blocks);
        auto& qInitInternal = FeatureBlocks::BQ<FeatureBlocks::BQ_InitInternal>::Get(blocks);

        auto sts = RunBlocks(CheckGE<mfxStatus, MFX_ERR_NONE>, qInitExternal, parNew, global, local);
        MFX_CHECK(sts >= MFX_ERR_NONE, sts);
        wrn = sts;

        sts = RunBlocks(CheckGE<mfxStatus, MFX_ERR_NONE>, qInitInternal, global, local);
        MFX_CHECK(sts >= MFX_ERR_NONE, sts);

        return GetWorstSts(sts, wrn);
    });

    Push(BLK_ResetCheck
        , [this](
            const mfxVideoParam& par
            , StorageRW& global
            , StorageRW& local) -> mfxStatus
    {
        auto& init = Glob::RealState::Get(global);
        auto& parOld = Glob::VideoParam::Get(init);
        auto& parNew = Glob::VideoParam::Get(global);
        auto& hint = Glob::ResetHint::Get(global);
        auto defOld = GetRTDefaults(init);
        auto defNew = GetRTDefaults(global);

        const mfxExtEncoderResetOption* pResetOpt = ExtBuffer::Get(par);

        const mfxExtHEVCParam (&hevcPar)[2] = { ExtBuffer::Get(parOld), ExtBuffer::Get(parNew) };
        MFX_CHECK(hevcPar[0].LCUSize == hevcPar[1].LCUSize, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM); // LCU Size Can't be changed

        MFX_CHECK(parOld.AsyncDepth                 == parNew.AsyncDepth,                   MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(parOld.mfx.GopRefDist             >= parNew.mfx.GopRefDist,               MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(parOld.mfx.NumRefFrame            >= parNew.mfx.NumRefFrame,              MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(parOld.mfx.RateControlMethod      == parNew.mfx.RateControlMethod,        MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(parOld.mfx.FrameInfo.ChromaFormat == parNew.mfx.FrameInfo.ChromaFormat,   MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(parOld.IOPattern                  == parNew.IOPattern,                    MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

        MFX_CHECK(local.Contains(Tmp::RecInfo::Key), MFX_ERR_UNDEFINED_BEHAVIOR);
        auto  recOld = Glob::AllocRec::Get(init).GetInfo();
        auto& recNew = Tmp::RecInfo::Get(local).Info;
        MFX_CHECK(recOld.Width  >= recNew.Width,  MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(recOld.Height >= recNew.Height, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(recOld.FourCC == recNew.FourCC, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

        MFX_CHECK(
            !(   parOld.mfx.RateControlMethod == MFX_RATECONTROL_CBR
              || parOld.mfx.RateControlMethod == MFX_RATECONTROL_VBR
              || parOld.mfx.RateControlMethod == MFX_RATECONTROL_VCM)
            ||(  (mfxU32)InitialDelayInKB(parOld.mfx) == (mfxU32)InitialDelayInKB(parNew.mfx)
              && (mfxU32)BufferSizeInKB(parOld.mfx) == (mfxU32)BufferSizeInKB(parNew.mfx))
            , MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

        mfxU32 tempLayerIdx = 0;
        bool changeTScalLayers = false;
        bool isIdrRequired = false;

        // check if change of temporal scalability required by new parameters
        auto numTlOld = defOld.base.GetNumTemporalLayers(defOld);
        auto numTlNew = defNew.base.GetNumTemporalLayers(defNew);
        bool bTlActive = numTlOld > 1 && numTlNew > 1;

        if (bTlActive)
        {
            // calculate temporal layer for next frame
            tempLayerIdx = defOld.base.GetTId(defOld, m_frameOrder + 1);
            changeTScalLayers = numTlOld != numTlNew;
        }

        // check if IDR required after change of encoding parameters
        const mfxExtCodingOption2(&CO2)[2] = { ExtBuffer::Get(parOld), ExtBuffer::Get(parNew) };

        isIdrRequired =
               (hint.Flags & RF_SPS_CHANGED)
            || (hint.Flags & RF_IDR_REQUIRED)
            || (tempLayerIdx != 0 && changeTScalLayers)
            || parOld.mfx.GopPicSize != parNew.mfx.GopPicSize
            || CO2[0].IntRefType != CO2[1].IntRefType;

        hint.Flags |= RF_IDR_REQUIRED * isIdrRequired;

        MFX_CHECK(!isIdrRequired || !(pResetOpt && IsOff(pResetOpt->StartNewSequence))
            , MFX_ERR_INVALID_VIDEO_PARAM); // Reset can't change parameters w/o IDR. Report an error

        bool brcReset =
            (      parOld.mfx.RateControlMethod == MFX_RATECONTROL_CBR
                || parOld.mfx.RateControlMethod == MFX_RATECONTROL_VBR
                || parOld.mfx.RateControlMethod == MFX_RATECONTROL_VCM)
            && (   (mfxU32)TargetKbps(parOld.mfx) != (mfxU32)TargetKbps(parNew.mfx)
                || (mfxU32)BufferSizeInKB(parOld.mfx) != (mfxU32)BufferSizeInKB(parNew.mfx)
                || (mfxU32)InitialDelayInKB(parOld.mfx) != (mfxU32)InitialDelayInKB(parNew.mfx)
                || parOld.mfx.FrameInfo.FrameRateExtN != parNew.mfx.FrameInfo.FrameRateExtN
                || parOld.mfx.FrameInfo.FrameRateExtD != parNew.mfx.FrameInfo.FrameRateExtD);

        brcReset |=
            (      parOld.mfx.RateControlMethod == MFX_RATECONTROL_VBR
                || parOld.mfx.RateControlMethod == MFX_RATECONTROL_VCM)
            && ((mfxU32)MaxKbps(parOld.mfx) != (mfxU32)MaxKbps(parNew.mfx));

        const mfxExtCodingOption(&CO)[2] = { ExtBuffer::Get(parOld), ExtBuffer::Get(parNew) };
        bool HRDConformance = !(IsOff(CO[1].NalHrdConformance) || IsOff(CO[1].VuiNalHrdParameters));

        MFX_CHECK(
           !(   brcReset
             && parOld.mfx.RateControlMethod == MFX_RATECONTROL_CBR
             && (HRDConformance || !isIdrRequired))
            , MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

        hint.Flags |= RF_BRC_RESET * (brcReset || isIdrRequired);

        return MFX_ERR_NONE;
    });
}

void Legacy::ResetState(const FeatureBlocks& blocks, TPushRS Push)
{
    Push(BLK_ResetState
        , [this, &blocks](
            StorageRW& global
            , StorageRW&) -> mfxStatus
    {
        auto& real = Glob::RealState::Get(global);
        auto& parInt = Glob::VideoParam::Get(real);
        auto& parNew = Glob::VideoParam::Get(global);
        auto& hint = Glob::ResetHint::Get(global);

        CopyConfigurable(blocks, parNew, parInt);
        Glob::VPS::Get(real) = Glob::VPS::Get(global);
        Glob::SPS::Get(real) = Glob::SPS::Get(global);
        Glob::PPS::Get(real) = Glob::PPS::Get(global);
        Glob::SliceInfo::Get(real) = Glob::SliceInfo::Get(global);

        m_forceHeaders |= !!(hint.Flags & RF_PPS_CHANGED) * INSERT_PPS;

        MFX_CHECK(hint.Flags & RF_IDR_REQUIRED, MFX_ERR_NONE);

        Glob::AllocRec::Get(real).UnlockAll();
        Glob::AllocBS::Get(real).UnlockAll();

        if (real.Contains(Glob::AllocMBQP::Key))
            Glob::AllocMBQP::Get(real).UnlockAll();

        if (real.Contains(Glob::AllocRaw::Key))
            Glob::AllocRaw::Get(real).UnlockAll();

        ResetState();

        return MFX_ERR_NONE;
    });
}

void Legacy::FrameSubmit(const FeatureBlocks& /*blocks*/, TPushFS Push)
{
    Push(BLK_CheckSurf
        , [](
            const mfxEncodeCtrl* /*pCtrl*/
            , const mfxFrameSurface1* pSurf
            , mfxBitstream& /*bs*/
            , StorageW& global
            , StorageRW& /*local*/) -> mfxStatus
    {
        MFX_CHECK(pSurf, MFX_ERR_NONE);

        auto& par = Glob::VideoParam::Get(global);
        MFX_CHECK(LumaIsNull(pSurf) == (pSurf->Data.UV == 0), MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(pSurf->Info.Width >= par.mfx.FrameInfo.Width, MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(pSurf->Info.Height >= par.mfx.FrameInfo.Height, MFX_ERR_INVALID_VIDEO_PARAM);
        
        return MFX_ERR_NONE;
    });

    Push(BLK_CheckBS
        , [](
            const mfxEncodeCtrl* /*pCtrl*/
            , const mfxFrameSurface1* /*pSurf*/
            , mfxBitstream& bs
            , StorageW& global
            , StorageRW& local) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(global);
        BsDataInfo bsData = {};

        bsData.Data       = bs.Data;
        bsData.DataLength = bs.DataLength;
        bsData.DataOffset = bs.DataOffset;
        bsData.MaxLength  = bs.MaxLength;

        if (local.Contains(Tmp::BsDataInfo::Key))
            bsData = Tmp::BsDataInfo::Get(local);

        MFX_CHECK(bsData.DataOffset <= bsData.MaxLength, MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(bsData.DataOffset + bsData.DataLength + BufferSizeInKB(par.mfx) * 1000u <= bsData.MaxLength, MFX_ERR_NOT_ENOUGH_BUFFER);
        MFX_CHECK_NULL_PTR1(bsData.Data);

        return MFX_ERR_NONE;
    });
}

void Legacy::AllocTask(const FeatureBlocks& /*blocks*/, TPushAT Push)
{
    Push(BLK_AllocTask
        , [](
            StorageR& /*global*/
            , StorageRW& task) -> mfxStatus
    {
        task.Insert(Task::Common::Key, new Task::Common::TRef);
        task.Insert(Task::SSH::Key, new MakeStorable<Task::SSH::TRef>);
        return MFX_ERR_NONE;
    });
}

void Legacy::InitTask(const FeatureBlocks& /*blocks*/, TPushIT Push)
{
    Push(BLK_InitTask
        , [](
            mfxEncodeCtrl* pCtrl
            , mfxFrameSurface1* pSurf
            , mfxBitstream* pBs
            , StorageW& global
            , StorageW& task) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(global);
        auto& core = Glob::VideoCore::Get(global);
        auto& tpar = Task::Common::Get(task);

        auto stage = tpar.stage;
        tpar = TaskCommonPar();
        tpar.stage = stage;
        tpar.pBsOut = pBs;

        MFX_CHECK(pSurf, MFX_ERR_NONE);

        tpar.pSurfIn = pSurf;

        if (pCtrl)
            tpar.ctrl = *pCtrl;

        if (par.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
        {
            tpar.pSurfReal = core.GetNativeSurface(tpar.pSurfIn);
            MFX_CHECK(tpar.pSurfReal, MFX_ERR_UNDEFINED_BEHAVIOR);

            tpar.pSurfReal->Info             = tpar.pSurfIn->Info;
            tpar.pSurfReal->Data.TimeStamp   = tpar.pSurfIn->Data.TimeStamp;
            tpar.pSurfReal->Data.FrameOrder  = tpar.pSurfIn->Data.FrameOrder;
            tpar.pSurfReal->Data.Corrupted   = tpar.pSurfIn->Data.Corrupted;
            tpar.pSurfReal->Data.DataFlag    = tpar.pSurfIn->Data.DataFlag;
        }
        else
            tpar.pSurfReal = tpar.pSurfIn;

        core.IncreaseReference(&tpar.pSurfIn->Data);

        return MFX_ERR_NONE;
    });
}

void Legacy::PreReorderTask(const FeatureBlocks& /*blocks*/, TPushPreRT Push)
{
    Push(BLK_PrepareTask
        , [this](
            StorageW& global
            , StorageW& s_task) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(global);
        auto& task = Task::Common::Get(s_task);
        auto  dflts = GetRTDefaults(global);

        m_frameOrder = dflts.base.GetFrameOrder(dflts, s_task, m_frameOrder);

        auto sts = dflts.base.GetPreReorderInfo(
            dflts, task, task.pSurfIn, &task.ctrl, m_lastIDR, m_prevTask.PrevIPoc, m_frameOrder);
        MFX_CHECK_STS(sts);

        if (par.mfx.EncodedOrder)
        {
            auto BufferSize     = Glob::Reorder::Get(global).BufferSize;
            auto MaxReorder     = Glob::Reorder::Get(global).MaxReorder;
            bool bFrameFromPast = m_frameOrder && (m_frameOrder < m_prevTask.DisplayOrder);

            MFX_CHECK(!bFrameFromPast || ((m_prevTask.DisplayOrder - m_frameOrder) <= MaxReorder), MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(m_frameOrder <= (m_prevTask.EncodedOrder + 1 + BufferSize), MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(isValid(m_prevTask.DPB.After[0]) || IsIdr(task.FrameType), MFX_ERR_UNDEFINED_BEHAVIOR);
        }
        task.DisplayOrder = m_frameOrder;
        task.PrevIPoc     = m_prevTask.PrevIPoc;
        
        SetIf(m_lastIDR, IsIdr(task.FrameType), m_frameOrder);
        SetIf(task.PrevIPoc, IsI(task.FrameType), task.POC);

        return MFX_ERR_NONE;
    });
}

void Legacy::PostReorderTask(const FeatureBlocks& /*blocks*/, TPushPostRT Push)
{
    Push(BLK_ConfigureTask
        , [this](
            StorageW& global
            , StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);

        if (global.Contains(Glob::AllocRaw::Key))
        {
            task.Raw = Glob::AllocRaw::Get(global).Acquire();
            MFX_CHECK(task.Raw.Mid, MFX_ERR_UNDEFINED_BEHAVIOR);
        }
        if (global.Contains(Glob::AllocMBQP::Key))
        {
            task.CUQP = Glob::AllocMBQP::Get(global).Acquire();
            MFX_CHECK(task.CUQP.Mid, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        task.Rec = Glob::AllocRec::Get(global).Acquire();
        task.BS = Glob::AllocBS::Get(global).Acquire();
        MFX_CHECK(task.BS.Idx != IDX_INVALID, MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(task.Rec.Idx != IDX_INVALID, MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(task.Rec.Mid && task.BS.Mid, MFX_ERR_UNDEFINED_BEHAVIOR);

        auto& par = Glob::VideoParam::Get(global);
        auto& sps = Glob::SPS::Get(global);
        auto& pps = Glob::PPS::Get(global);
        auto  def = GetRTDefaults(global);

        ConfigureTask(task, def, sps);

        auto sts = GetSliceHeader(par, task, sps, pps, Task::SSH::Get(s_task));
        MFX_CHECK_STS(sts);

        return sts;
    });
}

void Legacy::SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_SkipFrame
        , [](
            StorageW& global
            , StorageW& s_task) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(global);
        auto& task = Task::Common::Get(s_task);

        bool bCheckSkip =
            !task.bSkip
            && IsB(task.FrameType)
            && !task.isLDB
            && IsSWBRC(par, ExtBuffer::Get(par));

        auto& allocRec = Glob::AllocRec::Get(global);

        task.bSkip |=
            bCheckSkip
            && !!(allocRec.GetFlag(task.DPB.Active[task.RefPicList[1][0]].Rec.Idx) & REC_SKIPPED);

        MFX_CHECK(task.bSkip, MFX_ERR_NONE);

        task.bForceSync = true;

        if (IsI(task.FrameType))
        {
            MFX_CHECK(
                par.mfx.FrameInfo.FourCC == MFX_FOURCC_NV12
                || par.mfx.FrameInfo.FourCC == MFX_FOURCC_P010
                , MFX_ERR_UNDEFINED_BEHAVIOR);

            FrameLocker raw(Glob::VideoCore::Get(global), task.Raw.Mid);

            mfxU32 size = raw.Pitch * par.mfx.FrameInfo.Height;
            int UVFiller = (par.mfx.FrameInfo.FourCC == MFX_FOURCC_NV12) * 126;

            memset(raw.Y, 0, size);
            memset(raw.UV, UVFiller, size >> 1);

            allocRec.SetFlag(task.Rec.Idx, REC_SKIPPED);

            return MFX_ERR_NONE;
        }

        auto& core = Glob::VideoCore::Get(global);
        bool  bL1  = (IsB(task.FrameType) && !task.isLDB && task.NumRefActive[1] && !task.b2ndField);
        auto  idx  = task.RefPicList[bL1][0];

        mfxFrameSurface1 surfSrc = {};
        mfxFrameSurface1 surfDst = {};

        surfSrc.Info = par.mfx.FrameInfo;
        surfDst.Info = allocRec.GetInfo();

        MFX_CHECK(!memcmp(&surfSrc.Info, &surfDst.Info, sizeof(mfxFrameInfo)), MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(idx < MAX_DPB_SIZE, MFX_ERR_UNDEFINED_BEHAVIOR);

        auto& ref = task.DPB.Active[idx];

        allocRec.SetFlag(task.Rec.Idx, REC_SKIPPED);

        MFX_CHECK(allocRec.GetFlag(ref.Rec.Idx) & REC_READY, MFX_ERR_NONE);

        surfSrc.Data.MemId = ref.Rec.Mid;
        surfDst.Data.MemId = task.Raw.Mid;

        mfxStatus sts = core.DoFastCopyWrapper(
            &surfDst, MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_FROM_ENCODE,
            &surfSrc, MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_FROM_ENCODE);
        MFX_CHECK_STS(sts);

        allocRec.SetFlag(ref.Rec.Idx, REC_SKIPPED * !!idx);

        return MFX_ERR_NONE;
    });

    Push(BLK_GetRawHDL
        , [](
            StorageW& global
            , StorageW& s_task) -> mfxStatus
    {
        auto& core = Glob::VideoCore::Get(global);
        auto& par = Glob::VideoParam::Get(global);
        const mfxExtOpaqueSurfaceAlloc& opaq = ExtBuffer::Get(par);
        auto& task = Task::Common::Get(s_task);

        bool bInternalFrame =
            par.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY
            || (par.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY
                && (opaq.In.Type & MFX_MEMTYPE_SYSTEM_MEMORY))
            || task.bSkip;

        MFX_CHECK(!bInternalFrame, core.GetFrameHDL(task.Raw.Mid, &task.HDLRaw.first));

        MFX_CHECK(par.IOPattern != MFX_IOPATTERN_IN_VIDEO_MEMORY
            , core.GetExternalFrameHDL(task.pSurfReal->Data.MemId, &task.HDLRaw.first));

        MFX_CHECK(par.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY
            , MFX_ERR_UNDEFINED_BEHAVIOR);

        return core.GetFrameHDL(task.pSurfReal->Data.MemId, &task.HDLRaw.first);
    });

    Push(BLK_CopySysToRaw
        , [this](
            StorageW& global
            , StorageW& s_task)->mfxStatus
    {
        auto& par = Glob::VideoParam::Get(global);
        const mfxExtOpaqueSurfaceAlloc& opaq = ExtBuffer::Get(par);
        auto& task = Task::Common::Get(s_task);
        auto dflts = GetRTDefaults(global);

        MFX_CHECK(
            !(task.bSkip
            || par.IOPattern == MFX_IOPATTERN_IN_VIDEO_MEMORY
            || (    par.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY
                 && !(opaq.In.Type & MFX_MEMTYPE_SYSTEM_MEMORY)))
            , MFX_ERR_NONE);

        mfxFrameSurface1 surfSrc = { {}, par.mfx.FrameInfo, task.pSurfReal->Data };
        mfxFrameSurface1 surfDst = { {}, par.mfx.FrameInfo, {} };
        surfDst.Data.MemId = task.Raw.Mid;

        surfDst.Info.Shift =
            surfDst.Info.FourCC == MFX_FOURCC_P010
            || surfDst.Info.FourCC == MFX_FOURCC_Y210; // convert to native shift in core.CopyFrame() if required

        return  dflts.base.RunFastCopyWrapper(
            surfDst,
            MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_FROM_ENCODE,
            surfSrc,
            MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_SYSTEM_MEMORY);
    });

    Push(BLK_FillCUQPSurf
        , [this](
            StorageW& global
            , StorageW& s_task)->mfxStatus
    {
        auto& task = Task::Common::Get(s_task);

        MFX_CHECK(task.CUQP.Mid && task.bCUQPMap, MFX_ERR_NONE);

        mfxExtMBQP *mbqp = ExtBuffer::Get(task.ctrl);
        auto& par = Glob::VideoParam::Get(global);
        auto& core = Glob::VideoCore::Get(global);
        auto CUQPFrameInfo = Glob::AllocMBQP::Get(global).GetInfo();

        MFX_CHECK(CUQPFrameInfo.Width && CUQPFrameInfo.Height, MFX_ERR_UNDEFINED_BEHAVIOR);
        MFX_CHECK(m_CUQPBlkW && m_CUQPBlkH, MFX_ERR_UNDEFINED_BEHAVIOR);

        mfxU32 drBlkW = m_CUQPBlkW;  // block size of driver
        mfxU32 drBlkH = m_CUQPBlkH;  // block size of driver
        mfxU16 inBlkSize = 16; //mbqp->BlockSize ? mbqp->BlockSize : 16;  //input block size

        mfxU32 pitch_MBQP = (par.mfx.FrameInfo.Width + inBlkSize - 1) / inBlkSize;

        MFX_CHECK(mbqp && mbqp->NumQPAlloc, MFX_ERR_NONE);

        // CUQPFrameInfo.Width is LCU aligned, so compute unaligned
        mfxU32 unalignedWidth = (par.mfx.FrameInfo.Width + drBlkW - 1) / drBlkW;
        bool bInvalid =
            (mbqp->NumQPAlloc * inBlkSize * inBlkSize) < (drBlkW * drBlkH * unalignedWidth * CUQPFrameInfo.Height);
        bInvalid &= (drBlkW < inBlkSize || drBlkH < inBlkSize); // needs changing filling loop

        task.bCUQPMap &= !bInvalid;
        MFX_CHECK(!bInvalid, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);

        FrameLocker lock(core, task.CUQP.Mid);
        MFX_CHECK(lock.Y, MFX_ERR_LOCK_MEMORY);

        auto itSrcRow   = MakeStepIter(mbqp->QP, drBlkH / inBlkSize * pitch_MBQP);
        auto itDstRow   = MakeStepIter(lock.Y, lock.Pitch);
        auto stepSrcRow = drBlkW / inBlkSize;

        std::for_each(itSrcRow, std::next(itSrcRow, CUQPFrameInfo.Height)
            , [&](mfxU8& rSrcRowBegin)
        {
            std::copy_n(MakeStepIter(&rSrcRowBegin, stepSrcRow), unalignedWidth, &*itDstRow);
            // fill till EO LCU aligned width
            auto ItLastPos = MakeStepIter(&*itDstRow++ + unalignedWidth - 1);
            std::fill_n(std::next(ItLastPos), CUQPFrameInfo.Width - unalignedWidth, *ItLastPos);
        });

        return MFX_ERR_NONE;
    });
}

void Legacy::QueryTask(const FeatureBlocks& /*blocks*/, TPushQT Push)
{
    Push(BLK_CopyBS
        , [](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);

        if (!task.pBsData)
        {
            auto& bs              = *task.pBsOut;
            task.pBsData          = bs.Data + bs.DataOffset + bs.DataLength;
            task.pBsDataLength    = &bs.DataLength;
            task.BsBytesAvailable = bs.MaxLength - bs.DataOffset - bs.DataLength;
        }

        MFX_CHECK(task.BsDataLength, MFX_ERR_NONE);

        mfxStatus sts = MFX_ERR_NONE;

        MFX_CHECK(task.BsBytesAvailable >= task.BsDataLength, MFX_ERR_NOT_ENOUGH_BUFFER);

        FrameLocker codedFrame(Glob::VideoCore::Get(global), task.BS.Mid);
        MFX_CHECK(codedFrame.Y, MFX_ERR_LOCK_MEMORY);

        sts = FastCopy::Copy(
            task.pBsData
            , task.BsDataLength
            , codedFrame.Y
            , codedFrame.Pitch
            , { int(task.BsDataLength), 1 }
            , COPY_VIDEO_TO_SYS);
        MFX_CHECK_STS(sts);

        task.BsBytesAvailable -= task.BsDataLength;

        return MFX_ERR_NONE;
    });

    Push(BLK_DoPadding
        , [](StorageW& /*global*/, StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);

        MFX_CHECK(task.MinFrameSize >= task.BsDataLength, MFX_ERR_NONE);
        MFX_CHECK(!task.bDontPatchBS, MFX_ERR_UNDEFINED_BEHAVIOR);

        mfxU32 padding = task.MinFrameSize - task.BsDataLength;

        MFX_CHECK(task.BsBytesAvailable >= padding, MFX_ERR_NOT_ENOUGH_BUFFER);

        memset(task.pBsData + task.BsDataLength, 0, padding);

        task.BsDataLength += padding;
        task.BsBytesAvailable -= padding;

        return MFX_ERR_NONE;
    });

    Push(BLK_UpdateBsInfo
        , [](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        const auto& par = Glob::VideoParam::Get(global);
        auto& task = Task::Common::Get(s_task);
        auto& sps = Glob::SPS::Get(global);
        auto& bs = *task.pBsOut;
        mfxI32 dpbOutputDelay =
            task.DisplayOrder
            + sps.sub_layer[sps.max_sub_layers_minus1].max_num_reorder_pics
            - task.EncodedOrder;

        bs.TimeStamp = task.pSurfIn->Data.TimeStamp;
        bs.DecodeTimeStamp = MFX_TIMESTAMP_UNKNOWN;

        if (bs.TimeStamp != mfxU64(MFX_TIMESTAMP_UNKNOWN))
        {
            mfxF64 tcDuration90KHz = (mfxF64)par.mfx.FrameInfo.FrameRateExtD / par.mfx.FrameInfo.FrameRateExtN * 90000;
            bs.DecodeTimeStamp = mfxI64(bs.TimeStamp - tcDuration90KHz * dpbOutputDelay);
        }

        bs.PicStruct = task.pSurfIn->Info.PicStruct;
        bs.FrameType = task.FrameType;
        bs.FrameType &= ~(task.isLDB * MFX_FRAMETYPE_B);
        bs.FrameType |= task.isLDB * MFX_FRAMETYPE_P;

        *task.pBsDataLength += task.BsDataLength;

        return MFX_ERR_NONE;
    });
}

inline bool ReleaseResource(IAllocation& a, Resource& r)
{
    if (r.Mid)
    {
        a.Release(r.Idx);
        r = Resource();
        return true;
    }

    return r.Idx == IDX_INVALID;
}

void Legacy::FreeTask(const FeatureBlocks& /*blocks*/, TPushFT Push)
{
    Push(BLK_FreeTask
        , [](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);
        auto& core = Glob::VideoCore::Get(global);

        ThrowAssert(
            !ReleaseResource(Glob::AllocBS::Get(global), task.BS)
            , "task.BS resource is invalid");
        ThrowAssert(
            global.Contains(Glob::AllocMBQP::Key)
            && !ReleaseResource(Glob::AllocMBQP::Get(global), task.CUQP)
            , "task.CUQP resource is invalid");
        ThrowAssert(
            global.Contains(Glob::AllocRaw::Key)
            && !ReleaseResource(Glob::AllocRaw::Get(global), task.Raw)
            , "task.Raw resource is invalid");

        SetIf(task.pSurfIn, task.pSurfIn && !core.DecreaseReference(&task.pSurfIn->Data), nullptr);
        ThrowAssert(!!task.pSurfIn, "failed in core.DecreaseReference");

        auto& atrRec = Glob::AllocRec::Get(global);

        if (task.Rec.Idx != IDX_INVALID)
            atrRec.SetFlag(task.Rec.Idx, REC_READY);

        ThrowAssert(
            !IsRef(task.FrameType)
            && !ReleaseResource(atrRec, task.Rec)
            , "task.Rec resource is invalid");

        auto pDPBBeforeEnd = std::find_if_not(
            task.DPB.Before, task.DPB.Before + Size(task.DPB.Before), isValid);
        auto pDPBAfterEnd = std::find_if_not(
            task.DPB.After, task.DPB.After + Size(task.DPB.After), isValid);
        auto DPBFrameReleaseVerify = [&](DpbFrame& ref)
        {
            auto IsSameRecIdx = [&](DpbFrame& refA) { return refA.Rec.Idx == ref.Rec.Idx; };
            return pDPBAfterEnd != std::find_if(task.DPB.After, pDPBAfterEnd, IsSameRecIdx)
                || ReleaseResource(atrRec, ref.Rec);
        };
        auto nDPBFramesValid = std::count_if(task.DPB.Before, pDPBBeforeEnd, DPBFrameReleaseVerify);

        ThrowAssert(nDPBFramesValid != (pDPBBeforeEnd - task.DPB.Before), "task.DPB.Before is invalid");

        return MFX_ERR_NONE;
    });
}

void Legacy::GetVideoParam(const FeatureBlocks& blocks, TPushGVP Push)
{
    Push(BLK_CopyConfigurable
        , [this, &blocks](mfxVideoParam& out, StorageR& global) -> mfxStatus
    {
        return CopyConfigurable(blocks, Glob::VideoParam::Get(global), out);
    });
}

IntraRefreshState GetIntraRefreshState(
    const ExtBuffer::Param<mfxVideoParam> & par
    , const mfxEncodeCtrl& ctrl
    , mfxU32                frameOrderInGopDispOrder
    , mfxU32                IntraRefreshBlockUnitSize)
{
    IntraRefreshState state={};
    const mfxExtCodingOption2& CO2       = ExtBuffer::Get(par);
    const mfxExtCodingOption3& CO3       = ExtBuffer::Get(par);
    const mfxExtHEVCParam&     HEVCParam = ExtBuffer::Get(par);
    const mfxExtCodingOption2* pCO2RT    = ExtBuffer::Get(ctrl);

    mfxU32 refreshPeriod             = std::max<mfxU32>(CO3.IntRefCycleDist + (!CO3.IntRefCycleDist * CO2.IntRefCycleSize), 1);
    mfxU32 offsetFromStartOfGop      = std::max<mfxU32>(!!CO3.IntRefCycleDist * refreshPeriod, 1); // 1st refresh cycle in GOP starts with offset
    mfxI32 frameOrderMinusOffset     = frameOrderInGopDispOrder - offsetFromStartOfGop;
    mfxU32 frameOrderInRefreshPeriod = frameOrderMinusOffset % refreshPeriod;

    state.firstFrameInCycle = false;

    bool bNoUpdate =
        CO2.IntRefType == 0
        || frameOrderMinusOffset < 0 // too early to start refresh
        || frameOrderInRefreshPeriod >= CO2.IntRefCycleSize; // for current refresh period refresh cycle is already passed

    if (bNoUpdate)
        return state;

    mfxU32 IRBlockSize = 1 << (3 + IntraRefreshBlockUnitSize);
    // refreshing parts (stripes) in frame
    mfxU32 refreshDimension = CO2.IntRefType == MFX_REFRESH_HORIZONTAL
        ? CeilDiv<mfxU32>(HEVCParam.PicHeightInLumaSamples, IRBlockSize)
        : CeilDiv<mfxU32>(HEVCParam.PicWidthInLumaSamples, IRBlockSize);

    // In most cases number of refresh stripes is no aligned with number of frames for refresh.
    // In head frames are refreshed min stripes (can be 0), in tail min+1
    // min * head + (min+1) * tail == min * frames + tail == refreshDimension
    mfxU32 frames   = CO2.IntRefCycleSize;          // frames to commit full refresh
    mfxU32 minStr   = refreshDimension / frames;    // minimal refreshed stripes
    mfxU32 tail     = refreshDimension % frames;    // tail frames have minStr+1 stripes
    mfxU32 head     = frames - tail;                // head frames with minStr stripes

    if (frameOrderInRefreshPeriod < head) // min, can be 0
    {
        if (!minStr)
            return state; // actual refresh isn't started yet within current refresh cycle, no Intra column/row required for current frame

        state.IntraSize     = (mfxU16)minStr;
        state.IntraLocation = (mfxU16)(frameOrderInRefreshPeriod * minStr);
    }
    else
    {
        state.IntraSize     = (mfxU16)(minStr + 1);
        state.IntraLocation = (mfxU16)(frameOrderInRefreshPeriod * minStr + (frameOrderInRefreshPeriod - head));
    }

    state.firstFrameInCycle = (frameOrderInRefreshPeriod == 0);
    state.refrType          = CO2.IntRefType;

    // set QP for Intra macroblocks within refreshing line
    state.IntRefQPDelta = CO2.IntRefQPDelta;

    bool bUpdateQPDelta = pCO2RT && pCO2RT->IntRefQPDelta <= 51 && pCO2RT->IntRefQPDelta >= -51;
    if (bUpdateQPDelta)
        state.IntRefQPDelta = pCO2RT->IntRefQPDelta;

    return state;
}

mfxU8 GetCodingType(const TaskCommonPar & task)
{
    const mfxU8 I  = 1; // I picture.
    const mfxU8 P  = 2; // P or GPB picture at base temporal level.
    const mfxU8 B  = 3; // P, GPB or B picture at temporal level 1.
    const mfxU8 B1 = 4; // P, GPB or B picture at temporal level 2.
    const mfxU8 B2 = 5; // P, GPB or B picture at temporal level 3.

    auto IsBX = [&](mfxU8 idx)
    {
        auto& ref = task.DPB.Active[idx];
        return !ref.isLDB && ref.CodingType > B;
    };
    auto IsB0 = [&](mfxU8 idx)
    {
        auto& ref = task.DPB.Active[idx];
        return !ref.isLDB && ref.CodingType == B;
    };
    mfxU8 type = 0;
    type += I * IsI(task.FrameType);
    type += P * (!type && IsP(task.FrameType));
    type += B * (!type && task.isLDB);
    type += B2 * (!type && std::any_of(task.RefPicList[0], task.RefPicList[0] + task.NumRefActive[0], IsBX));
    type += B2 * (!type && std::any_of(task.RefPicList[1], task.RefPicList[1] + task.NumRefActive[1], IsBX));
    type += B1 * (!type && std::any_of(task.RefPicList[0], task.RefPicList[0] + task.NumRefActive[0], IsB0));
    type += B1 * (!type && std::any_of(task.RefPicList[1], task.RefPicList[1] + task.NumRefActive[1], IsB0));
    type += B * !type;

    return type;
}

class SkipMode
{
private:
    eSkipMode m_mode;
    mfxU32 m_cmd;

    void SetCMD()
    {
        m_cmd = 0;
        m_cmd |= NeedInputReplacement()     * SKIPCMD_NeedInputReplacement;
        m_cmd |= NeedDriverCall()           * SKIPCMD_NeedDriverCall;
        m_cmd |= NeedSkipSliceGen()         * SKIPCMD_NeedSkipSliceGen;
        m_cmd |= NeedCurrentFrameSkipping() * SKIPCMD_NeedCurrentFrameSkipping;
        m_cmd |= NeedNumSkipAdding()        * SKIPCMD_NeedNumSkipAdding;
    }
public:
    SkipMode(eSkipMode mode = SKIPFRAME_NO)
        : m_mode(mode)
    {
        SetCMD();
    }

    SkipMode(mfxU16 mode, bool bProtected)
    {
        SetMode(mode, bProtected);
    }

    void SetMode(mfxU16 skipModeMFX, bool bProtected)
    {
        m_mode = eSkipMode(
            SKIPFRAME_INSERT_DUMMY_PROTECTED * (skipModeMFX == MFX_SKIPFRAME_INSERT_DUMMY && bProtected)
            + SKIPFRAME_INSERT_DUMMY         * (skipModeMFX == MFX_SKIPFRAME_INSERT_DUMMY && !bProtected)
            + SKIPFRAME_INSERT_NOTHING       * (skipModeMFX == MFX_SKIPFRAME_INSERT_NOTHING)
            + SKIPFRAME_BRC_ONLY             * (skipModeMFX == MFX_SKIPFRAME_BRC_ONLY));
        SetCMD();
    }

    void SetPseudoSkip()
    {
        m_mode = SKIPFRAME_EXT_PSEUDO;
    }

    eSkipMode GetMode() { return m_mode; }
    mfxU32 GetCMD() { return m_cmd; }

    bool NeedInputReplacement()
    {
        return m_mode == SKIPFRAME_EXT_PSEUDO;
    }
    bool NeedDriverCall()
    {
        return m_mode == SKIPFRAME_INSERT_DUMMY_PROTECTED
            || m_mode == SKIPFRAME_EXT_PSEUDO
            || m_mode == SKIPFRAME_NO
            || m_mode == SKIPFRAME_EXT_PSEUDO;
    }
    bool NeedSkipSliceGen()
    {
        return m_mode == SKIPFRAME_INSERT_DUMMY_PROTECTED
            || m_mode == SKIPFRAME_INSERT_DUMMY;
    }
    bool NeedCurrentFrameSkipping()
    {
        return m_mode == SKIPFRAME_INSERT_DUMMY_PROTECTED
            || m_mode == SKIPFRAME_INSERT_DUMMY
            || m_mode == SKIPFRAME_INSERT_NOTHING;
    }
    bool NeedNumSkipAdding()
    {
        return m_mode == SKIPFRAME_BRC_ONLY;
    }
};

static void SetTaskQpY(
    TaskCommonPar & task
    , const ExtBuffer::Param<mfxVideoParam> & par
    , const SPS& sps
    , const Defaults::Param& dflts)
{
    const auto& hw = dflts.hw;
    const mfxExtCodingOption2& CO2 = ExtBuffer::Get(par);
    const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par);
    const mfxU8 maxQP = mfxU8(51 + 6 * (CO3.TargetBitDepthLuma - 8));

    if (par.mfx.RateControlMethod != MFX_RATECONTROL_CQP)
    {
        task.QpY &= 0xff * (par.mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT);
        return;
    }

    bool bUseQPP = IsP(task.FrameType) || task.isLDB;
    bool bUseQPB = !bUseQPP && IsB(task.FrameType);
    bool bUseQPOffset =
        (bUseQPB && CO2.BRefType == MFX_B_REF_PYRAMID)
        || (bUseQPP && CO3.PRefType == MFX_P_REF_PYRAMID);

    // set coding type and QP
    if (bUseQPB)
    {
        task.QpY = (mfxI8)par.mfx.QPB;
        if (bUseQPOffset)
        {
            task.QpY = (mfxI8)mfx::clamp<mfxI32>(
                CO3.QPOffset[mfx::clamp<mfxI32>(task.PyramidLevel - 1, 0, 7)] + task.QpY
                , 1, maxQP);
        }
    }
    else if (bUseQPP)
    {
        // encode P as GPB
        task.QpY = (mfxI8)par.mfx.QPP;

        if (dflts.base.GetNumTemporalLayers(dflts) > 1)
        {
            task.QpY = (mfxI8)mfx::clamp<mfxI32>(CO3.QPOffset[task.TemporalID] + task.QpY, 1, maxQP);
        }
        else if (bUseQPOffset)
        {
            task.QpY = (mfxI8)mfx::clamp<mfxI32>(
                CO3.QPOffset[std::min<size_t>(task.PyramidLevel, Size(CO3.QPOffset) - 1)] + task.QpY
                , 1, maxQP);
        }
    }
    else
    {
        assert(IsI(task.FrameType));
        task.QpY = (mfxI8)par.mfx.QPI;
    }

    SetIf(task.QpY, !!task.ctrl.QP, (mfxI8)task.ctrl.QP);

    task.QpY -= 6 * sps.bit_depth_luma_minus8;
    task.QpY &= 0xff * !(task.QpY < 0 && (IsOn(par.mfx.LowPower) || (hw >= MFX_HW_KBL && hw <= MFX_HW_CNL)));
}

void Legacy::ConfigureTask(
    TaskCommonPar & task
    , const Defaults::Param& dflts
    , const SPS& sps)
{
    auto&                      par = dflts.mvp;
    const mfxExtCodingOption&  CO  = ExtBuffer::Get(par);
    const mfxExtCodingOption2& CO2 = ExtBuffer::Get(par);
    const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par);
    const bool isI    = IsI(task.FrameType);
    const bool isP    = IsP(task.FrameType);
    const bool isIDR  = IsIdr(task.FrameType);

    mfxExtAVCRefLists*    pExtLists    = ExtBuffer::Get(task.ctrl);
    mfxExtAVCRefListCtrl* pExtListCtrl = ExtBuffer::Get(task.ctrl);

    {
        const mfxExtCodingOption2* pCO2 = ExtBuffer::Get(task.ctrl);
        SkipMode mode;

        SetDefault(pCO2, &CO2);

        mode.SetMode(mfxU16(!!task.ctrl.SkipFrame * pCO2->SkipFrame), !!par.Protected);

        task.FrameType &= ~(MFX_FRAMETYPE_REF
            * (mode.NeedCurrentFrameSkipping() && par.mfx.GopRefDist == 1 && isP));


        if (IsRef(task.FrameType))
        {
            task.ctrl.SkipFrame = 0;
            mode.SetMode(MFX_SKIPFRAME_NO_SKIP, !!par.Protected);
        }

        task.SkipCMD = mode.GetCMD();
    }

    task.ctrl.MfxNalUnitType &= 0xffff * IsOn(CO3.EnableNalUnitType);

    const mfxExtMBQP *pMBQP = ExtBuffer::Get(task.ctrl);
    task.bCUQPMap |= (IsOn(CO3.EnableMBQP) && pMBQP && pMBQP->NumQPAlloc > 0);

    bool bUpdateIRState = task.TemporalID == 0 && CO2.IntRefType;
    if (bUpdateIRState)
    {
        m_baseLayerOrder *= !isI;
        task.IRState = GetIntraRefreshState(
            par, task.ctrl, m_baseLayerOrder++, dflts.caps.IntraRefreshBlockUnitSize);
    }

    if (IsTCBRC(par, dflts.caps.TCBRCSupport) && task.TCBRCTargetFrameSize == 0)
    {
        ThrowAssert(par.mfx.FrameInfo.FrameRateExtD == 0, "FrameRateExtD = 0");

        mfxU32 AvgFrameSizeInBytes = mfxU32(1000.0 / 8.0*(par.mfx.TargetKbps) * std::max<mfxU32>(par.mfx.BRCParamMultiplier,1) /
            (mfxF64(par.mfx.FrameInfo.FrameRateExtN) / par.mfx.FrameInfo.FrameRateExtD));
        task.TCBRCTargetFrameSize = AvgFrameSizeInBytes;
    }

    mfxU32 needRecoveryPointSei = (CO.RecoveryPointSEI == MFX_CODINGOPTION_ON
        && (   (CO2.IntRefType && task.IRState.firstFrameInCycle && task.IRState.IntraLocation == 0)
            || (CO2.IntRefType == 0 && isI)));
    mfxU32 needCpbRemovalDelay = isIDR || needRecoveryPointSei;
    const bool isRef = IsRef(task.FrameType);

    // encode P as GPB
    task.isLDB      = IsOn(CO3.GPB) && isP;
    task.FrameType &= ~(MFX_FRAMETYPE_P * task.isLDB);
    task.FrameType |= (MFX_FRAMETYPE_B * task.isLDB);

    task.PrevIPoc = m_prevTask.PrevIPoc;
    task.PrevRAP = m_prevTask.PrevRAP;
    task.EncodedOrder = m_prevTask.EncodedOrder + 1;

    InitDPB(task, m_prevTask, pExtListCtrl);

    //construct ref lists
    std::tie(task.NumRefActive[0], task.NumRefActive[1]) = dflts.base.GetFrameNumRefActive(dflts, task);

    if (!isI)
    {
        ConstructRPL(dflts, task.DPB.Active, task, task.RefPicList, task.NumRefActive, pExtLists, pExtListCtrl);
    }

    SetTaskQpY(task, par, sps, dflts);
    task.CodingType = GetCodingType(task);

    task.InsertHeaders |= m_forceHeaders;
    m_forceHeaders = 0;

    task.InsertHeaders |= (INSERT_VPS | INSERT_SPS | INSERT_PPS) * isIDR;
    task.InsertHeaders |= INSERT_BPSEI * (needCpbRemovalDelay && sps.vui.hrd_parameters_present_flag );
    task.InsertHeaders |= INSERT_PTSEI
                * (sps.vui.frame_field_info_present_flag
                || sps.vui.hrd.nal_hrd_parameters_present_flag
                || sps.vui.hrd.vcl_hrd_parameters_present_flag);
    task.InsertHeaders |= INSERT_PPS * IsOn(CO2.RepeatPPS);
    task.InsertHeaders |= INSERT_AUD * IsOn(CO.AUDelimiter);

    // update dpb
    std::copy(task.DPB.Active, task.DPB.Active + Size(task.DPB.Active), task.DPB.After);
    Remove(task.DPB.After, 0, MAX_DPB_SIZE * isIDR);

    if (isRef)
    {
        task.PrevIPoc = isI * task.POC + !isI * task.PrevIPoc;

        UpdateDPB(dflts, task, task.DPB.After, pExtListCtrl);

        using TRLtrDesc = decltype(pExtListCtrl->LongTermRefList[0]);
        auto IsCurFrame = [&](TRLtrDesc ltr)
        {
            return ltr.FrameOrder == task.DisplayOrder;
        };

        task.isLTR |=
            pExtListCtrl
            && std::any_of(
                std::begin(pExtListCtrl->LongTermRefList)
                , std::end(pExtListCtrl->LongTermRefList)
                , IsCurFrame);
    }

    task.SliceNUT = dflts.base.GetSHNUT(dflts, task, true);

    bool bRAP =
        task.SliceNUT == CRA_NUT
        || task.SliceNUT == IDR_W_RADL
        || task.SliceNUT == IDR_N_LP;

    task.PrevRAP = bRAP * task.POC + !bRAP * task.PrevRAP;

    task.StatusReportId = std::max<mfxU32>(1, m_prevTask.StatusReportId + 1);
    task.bForceSync = !!(task.InsertHeaders & INSERT_BPSEI);

    if (task.FrameType & MFX_FRAMETYPE_I)
    {
        task.m_minQP = CO2.MinQPI;
        task.m_maxQP = CO2.MaxQPI;
    }
    else if (task.FrameType & MFX_FRAMETYPE_P)
    {
        task.m_minQP = CO2.MinQPP;
        task.m_maxQP = CO2.MaxQPP;
    }
    else if (task.FrameType & MFX_FRAMETYPE_B)
    {
        task.m_minQP = CO2.MinQPB;
        task.m_maxQP = CO2.MaxQPB;
    }

    m_prevTask = task;
}

static mfxU32 CountL1(DpbArray const & dpb, mfxI32 poc)
{
    mfxU32 c = 0;
    for (mfxU32 i = 0; !isDpbEnd(dpb, i); i++)
        c += dpb[i].POC > poc;
    return c;
}

static mfxU32 GetEncodingOrder(
    mfxU32 displayOrder
    , mfxU32 begin
    , mfxU32 end
    , mfxU32 &level
    , mfxU32 before
    , bool & ref)
{
    assert(displayOrder >= begin);
    assert(displayOrder <  end);

    ref = (end - begin > 1);

    mfxU32 pivot = (begin + end) / 2;
    if (displayOrder == pivot)
        return level + before;

    level++;
    if (displayOrder < pivot)
        return GetEncodingOrder(displayOrder, begin, pivot, level, before, ref);
    else
        return GetEncodingOrder(displayOrder, pivot + 1, end, level, before + pivot - begin, ref);
}

mfxU32 Legacy::GetBiFrameLocation(mfxU32 i, mfxU32 num, bool &ref, mfxU32 &level)
{
    ref = false;
    level = 1;
    return GetEncodingOrder(i, 0, num, level, 0, ref);
}

Legacy::TItWrapIt Legacy::BPyrReorder(TItWrapIt begin, TItWrapIt end)
{
    using TRef = std::iterator_traits<TItWrapIt>::reference;

    mfxU32 num = mfxU32(std::distance(begin, end));
    bool bSetOrder = num && (*begin)->BPyramidOrder == mfxU32(MFX_FRAMEORDER_UNKNOWN);

    if (bSetOrder)
    {
        mfxU32 i = 0;
        std::for_each(begin, end, [&](TRef bref)
        {
            bool bRef = false;
            bref->BPyramidOrder = Legacy::GetBiFrameLocation(i++, num, bRef, bref->PyramidLevel);
            bref->FrameType |= mfxU16(MFX_FRAMETYPE_REF * bRef);
        });
    }

    return std::min_element(begin, end
        , [](TRef a, TRef b) { return a->BPyramidOrder < b->BPyramidOrder; });
}

Legacy::TItWrap Legacy::Reorder(
    ExtBuffer::Param<mfxVideoParam> const & par
    , DpbArray const & dpb
    , TItWrap begin
    , TItWrap end
    , bool flush)
{
    using TRef = TItWrap::reference;

    const mfxExtCodingOption2& CO2 = ExtBuffer::Get(par);
    bool isBPyramid = (CO2.BRefType == MFX_B_REF_PYRAMID);
    TItWrap top = begin;
    std::list<TItWrap> brefs;
    auto IsB = [](TRef f) { return HEVCEHW::IsB(f.FrameType); };
    auto NoL1 = [&](TItWrap& f) { return !CountL1(dpb, f->POC); };

    std::generate_n(
        std::back_inserter(brefs)
        , std::distance(begin, std::find_if_not(begin, end, IsB))
        , [&]() { return top++; });

    brefs.remove_if(NoL1);

    bool bNoPyramidB = !isBPyramid && !brefs.empty();
    if (bNoPyramidB)
    {
        auto B0POC = brefs.front()->POC;
        auto RefB = [B0POC](TItWrap& f)
        {
            return IsRef(f->FrameType) && (f->POC - B0POC < 2);
        };
        TItWrapIt BCand[2] =
        {
            std::find_if(brefs.begin() , brefs.end(), RefB)
            , brefs.begin()
        };

        return *BCand[BCand[0] == brefs.end()];
    }

    if (!brefs.empty())
        return *BPyrReorder(brefs.begin(), brefs.end());

    bool bForcePRef = flush && top == end && begin != end;
    if (bForcePRef)
    {
        --top;
        top->FrameType = mfxU16(MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF);
    }

    return top;
}

std::tuple<mfxStatus, mfxU16, mfxU16>
Legacy::GetCUQPMapBlockSize(
    mfxU16 frameWidth
    , mfxU16 frameHeight
    , mfxU16 CUQPWidth
    , mfxU16 CUHeight)
{
    bool bValid = CUQPWidth && CUHeight;

    if (bValid)
    {
        const mfxU16 BlkSizes[] = { 4, 8, 16, 32, 64 };
        auto itBlkWidth = std::lower_bound(BlkSizes, std::end(BlkSizes), frameWidth / CUQPWidth);
        auto itBlkHeight = std::lower_bound(BlkSizes, std::end(BlkSizes), frameHeight / CUHeight);

        bValid =
            itBlkWidth != std::end(BlkSizes)
            && itBlkHeight != std::end(BlkSizes)
            && (*itBlkWidth   * (CUQPWidth - 1) < frameWidth)
            && (*itBlkHeight  * (CUHeight - 1) < frameHeight);

        if (bValid)
            return std::make_tuple(MFX_ERR_NONE, *itBlkWidth, *itBlkHeight);
    }

    return std::make_tuple(MFX_ERR_UNDEFINED_BEHAVIOR, mfxU16(0), mfxU16(0));
}

mfxU32 Legacy::GetMinBsSize(
    const mfxVideoParam & par
    , const mfxExtHEVCParam& HEVCParam
    , const mfxExtCodingOption2& CO2
    , const mfxExtCodingOption3& CO3)
{
    mfxU32 size = HEVCParam.PicHeightInLumaSamples * HEVCParam.PicWidthInLumaSamples;
    
    SetDefault(size, par.mfx.FrameInfo.Width * par.mfx.FrameInfo.Height);

    bool b10bit = (CO3.TargetBitDepthLuma == 10);
    bool b422   = (CO3.TargetChromaFormatPlus1 == (MFX_CHROMAFORMAT_YUV422 + 1));
    bool b444   = (CO3.TargetChromaFormatPlus1 == (MFX_CHROMAFORMAT_YUV444 + 1));

    mfxF64 k = 2.0
        + (b10bit * 0.3)
        + (b422   * 0.5)
        + (b444   * 1.5);

    size = mfxU32(k * size);

    bool bUseAvgSize =
        par.mfx.RateControlMethod == MFX_RATECONTROL_CBR
        && IsSWBRC(par, &CO2)
        && par.mfx.FrameInfo.FrameRateExtD != 0;

    if (!bUseAvgSize)
        return size;

    mfxU32 avgSize = TargetKbps(par.mfx) * 1000 * par.mfx.FrameInfo.FrameRateExtD / (par.mfx.FrameInfo.FrameRateExtN * 8);

    return std::max<mfxU32>(size, avgSize * 2);
}

typedef std::remove_reference<decltype(((mfxExtAVCRefListCtrl*)nullptr)->PreferredRefList[0])>::type TLCtrlRLE;

void Legacy::InitDPB(
    TaskCommonPar &        task,
    TaskCommonPar const &  prevTask,
    const mfxExtAVCRefListCtrl* pLCtrl)
{
    bool b1stTrail =
        task.POC > task.PrevRAP
        && prevTask.POC <= prevTask.PrevRAP;

    if ((task.IRState.refrType && !task.IRState.firstFrameInCycle) // IntRefCycle
        || (!task.IRState.refrType && prevTask.IRState.refrType)) // First frame after IntRefCycle
    {
        Remove(task.DPB.Active, 0, MAX_DPB_SIZE);

        for (mfxU8 i = 0; !isDpbEnd(prevTask.DPB.After, i); i++)
        {
            const DpbFrame& ref = prevTask.DPB.After[i]; // initial POC = -1
            if (ref.POC > task.DPB.Active[0].POC && ref.TemporalID == 0) // disable multiref within IntraRefCycle and next frame
                                                                         // and update DPB.Active only if temporal layer id = 0 for IntraRef cases
                task.DPB.Active[0] = ref;
        }
    }
    else if (b1stTrail)
    {
        Remove(task.DPB.Active, 0, MAX_DPB_SIZE);

        // TODO: add mode to disable this check
        std::copy_if(
            prevTask.DPB.After
            , prevTask.DPB.After + Size(prevTask.DPB.After)
            , task.DPB.Active
            , [&](const DpbFrame& ref)
        {
            return isValid(ref) && (ref.POC == task.PrevRAP || ref.isLTR);
        });
    }
    else
    {
        std::copy_n(prevTask.DPB.After, Size(prevTask.DPB.After), task.DPB.Active);
    }

    std::copy_n(prevTask.DPB.After, Size(prevTask.DPB.After), task.DPB.Before);

    DpbArray& dpb = task.DPB.Active;
    auto dpbEnd = std::find_if_not(dpb, dpb + Size(dpb), isValid);

    dpbEnd = RemoveIf(dpb, dpbEnd
        , [&](const DpbFrame& ref)
    {
        return ref.TemporalID > 0 && ref.TemporalID >= task.TemporalID;
    });

    if (pLCtrl)
    {
        std::list<mfxU32> rejectIDX;

        std::transform(
            std::begin(pLCtrl->RejectedRefList)
            , std::end(pLCtrl->RejectedRefList)
            , std::back_inserter(rejectIDX)
            , [&](const TLCtrlRLE& lt)
        {
            mfxU16 idx = GetDPBIdxByFO(dpb, lt.FrameOrder);
            bool bInvalid = (idx >= MAX_DPB_SIZE) || !dpb[idx].isLTR;
            return std::max(lt.FrameOrder, mfxU32(bInvalid * MFX_FRAMEORDER_UNKNOWN));
        });

        rejectIDX.sort();
        rejectIDX.unique();
        rejectIDX.remove(mfxU32(MFX_FRAMEORDER_UNKNOWN));

        std::for_each(rejectIDX.begin(), rejectIDX.end()
            , [&](mfxU32 fo)
        {
            Remove(dpb, GetDPBIdxByFO(dpb, fo));
        });
    }
}

mfxU16 Legacy::UpdateDPB(
    const Defaults::Param& def
    , const DpbFrame& task
    , DpbArray & dpb
    , const mfxExtAVCRefListCtrl * pLCtrl)
{
    auto& par = def.mvp;
    const mfxExtCodingOption2& CO2  = ExtBuffer::Get(par);
    bool isBPyramid                 = (CO2.BRefType == MFX_B_REF_PYRAMID);
    mfxU16 end                      = 0; // DPB end
    mfxU16 st0                      = 0; // first ST ref in DPB
    bool bClearCodingType           = isBPyramid && (task.isLDB || task.CodingType < CODING_TYPE_B);
    auto ClearCodingType            = [](DpbFrame& ref) { ref.CodingType = 0; };
    auto IsLTR                      = [](DpbFrame& f) { return f.isLTR; };
    auto POCLess                    = [](DpbFrame& l, DpbFrame& r) { return l.POC < r.POC; };

    end = mfxU16(std::find_if_not(dpb, dpb + Size(dpb), isValid) - dpb);
    st0 = mfxU16(std::find_if_not(dpb, dpb + end, IsLTR) - dpb);

    // frames stored in DPB in POC ascending order,
    // LTRs before STRs (use LTR-candidate as STR as long as it is possible)
    std::sort(dpb, dpb + st0, POCLess);
    std::sort(dpb + st0, dpb + end, POCLess);

    // sliding window over STRs
    bool bRunSlidingWindow = end && end == par.mfx.NumRefFrame;
    if (bRunSlidingWindow)
    {
        st0 = mfxU16(def.base.GetWeakRef(def, task, dpb + st0, dpb + end) - dpb);

        Remove(dpb, st0 * (st0 < end));
        --end;
    }

    ThrowAssert(end >= MAX_DPB_SIZE, "DPB overflow, no space for new frame");

    //don't keep coding types for prev. mini-GOP
    std::for_each(dpb, dpb + (end * bClearCodingType), ClearCodingType);

    dpb[end++] = task;

    if (pLCtrl)
    {
        st0 = mfxU16(std::find_if_not(dpb, dpb + end, IsLTR) - dpb);

        auto lctrlLtrEnd = std::find_if(
            pLCtrl->LongTermRefList
            , pLCtrl->LongTermRefList + Size(pLCtrl->LongTermRefList)
            , [](const TLCtrlRLE& lt) { return lt.FrameOrder == mfxU32(MFX_FRAMEORDER_UNKNOWN); });

        std::list<mfxU16> markLTR;

        std::transform(pLCtrl->LongTermRefList, lctrlLtrEnd, std::back_inserter(markLTR)
            , [&](const TLCtrlRLE& lt)
        {
            mfxU16 idx = GetDPBIdxByFO(dpb, lt.FrameOrder);
            idx += !!dpb[idx].isLTR * MAX_DPB_SIZE;
            return std::min<mfxU16>(idx, MAX_DPB_SIZE);
        });

        markLTR.sort();
        markLTR.remove(mfxU16(MAX_DPB_SIZE));
        markLTR.unique();

        std::for_each(markLTR.begin(), markLTR.end()
            , [&](mfxU16 idx)
        {
            DpbFrame ltr = dpb[idx];
            ltr.isLTR = true;
            Remove(dpb, idx);
            Insert(dpb, st0, ltr);
            st0++;
        });

        std::sort(dpb, dpb + st0, POCLess);
    }

    return end;
}

void Legacy::ConstructRPL(
    const Defaults::Param& dflts
    , const DpbArray & DPB
    , const FrameBaseInfo& cur
    , mfxU8(&RPL)[2][MAX_DPB_SIZE]
    , mfxU8(&numRefActive)[2]
    , const mfxExtAVCRefLists * pExtLists
    , const mfxExtAVCRefListCtrl * pLCtrl)
{
    auto GetRPLFromExt = [&]()
    {
        return dflts.base.GetRPLFromExt(
            dflts
            , DPB
            , numRefActive[0]
            , numRefActive[1]
            , *pExtLists
            , RPL);
    };
    auto GetRPLFromCtrl = [&]()
    {
        return dflts.base.GetRPLFromCtrl (
            dflts
            , DPB
            , numRefActive[0]
            , numRefActive[1]
            , cur
            , *pLCtrl
            , RPL);
    };

    std::tuple<mfxU8, mfxU8> nRef(mfxU8(0), mfxU8(0));

    SetIf(nRef, !!pExtLists, GetRPLFromExt);

    SetIf(nRef, !std::get<0>(nRef)
        , dflts.base.GetRPL
        , dflts
        , DPB
        , numRefActive[0]
        , numRefActive[1]
        , cur
        , RPL);

    SetIf(nRef, !!pLCtrl, GetRPLFromCtrl);
    ThrowAssert(!std::get<0>(nRef), "L0 is empty");

    nRef = dflts.base.GetRPLMod(
        dflts
        , DPB
        , numRefActive[0]
        , numRefActive[1]
        , cur
        , RPL);

    numRefActive[0] = std::get<0>(nRef);
    numRefActive[1] = std::get<1>(nRef);
}

void Legacy::ConstructSTRPS(
    const DpbArray & DPB
    , const mfxU8(&RPL)[2][MAX_DPB_SIZE]
    , const mfxU8(&numRefActive)[2]
    , mfxI32 poc
    , STRPS& rps)
{
    mfxU32 i, nRef;

    for (i = 0, nRef = 0; !isDpbEnd(DPB, i); i ++)
    {
        if (DPB[i].isLTR)
            continue;

        rps.pic[nRef].DeltaPocSX = (mfxI16)(DPB[i].POC - poc);
        rps.pic[nRef].used_by_curr_pic_sx_flag = IsCurrRef(DPB, RPL, numRefActive, DPB[i].POC);

        rps.num_negative_pics += rps.pic[nRef].DeltaPocSX < 0;
        rps.num_positive_pics += rps.pic[nRef].DeltaPocSX > 0;
        nRef ++;
    }

    std::sort(rps.pic, rps.pic + nRef
        , [](decltype(rps.pic[0]) l, decltype(rps.pic[0]) r) { return l.DeltaPocSX < r.DeltaPocSX; });
    std::sort(rps.pic, rps.pic + rps.num_negative_pics
        , [](decltype(rps.pic[0]) l, decltype(rps.pic[0]) r) { return l.DeltaPocSX > r.DeltaPocSX; });

    for (i = 0; i < nRef; i ++)
    {
        mfxI16 prev  = (!i || i == rps.num_negative_pics) ? 0 : rps.pic[i-1].DeltaPocSX;
        rps.pic[i].delta_poc_sx_minus1 = mfxU16(abs(rps.pic[i].DeltaPocSX - prev) - 1);
    }
}

mfxU32 EstimateRpsBits(const STRPS* pSpsRps, mfxU8 nSet, const STRPS & rps, mfxU8 idx)
{
    auto NBitsUE = [](mfxU32 b) -> mfxU32
    {
        return CeilLog2(b + 2) * 2 - 1;
    };
    auto IsNotUsed = [](const STRPSPic& pic)
    {
        return !pic.used_by_curr_pic_flag;
    };
    auto AccNBitsDPoc = [&](mfxU32 x, const STRPSPic& pic)
    {
        return std::move(x) + NBitsUE(pic.delta_poc_sx_minus1) + 1;
    };
    mfxU32 n = (idx != 0);

    if (!rps.inter_ref_pic_set_prediction_flag)
    {
        mfxU32 nPic = mfxU32(rps.num_negative_pics + rps.num_positive_pics);

        n += NBitsUE(rps.num_negative_pics);
        n += NBitsUE(rps.num_positive_pics);

        return std::accumulate(rps.pic, rps.pic + nPic, n, AccNBitsDPoc);
    }

    assert(idx > rps.delta_idx_minus1);
    STRPS const & ref = pSpsRps[idx - rps.delta_idx_minus1 - 1];
    mfxU32 nPic = mfxU32(ref.num_negative_pics + ref.num_positive_pics);

    if (idx == nSet)
        n += NBitsUE(rps.delta_idx_minus1);

    n += 1;
    n += NBitsUE(rps.abs_delta_rps_minus1);
    n += nPic;
    n += mfxU32(std::count_if(rps.pic, rps.pic + nPic + 1, IsNotUsed));

    return n;
}

bool GetInterRps(STRPS const & refRPS, STRPS& rps, mfxU8 dIdxMinus1)
{
    auto oldRPS = rps;
    auto newRPS = oldRPS;
    newRPS.inter_ref_pic_set_prediction_flag = 1;
    newRPS.delta_idx_minus1 = dIdxMinus1;

    std::list<mfxI16> dPocs[2];
    auto AddDPoc = [&dPocs](mfxI16 dPoc)
    {
        if (dPoc)
            dPocs[dPoc > 0].push_back(dPoc);
    };

    std::for_each(oldRPS.pic, oldRPS.pic + oldRPS.num_negative_pics + oldRPS.num_positive_pics
        , [&](STRPSPic& oldPic)
    {
        AddDPoc(oldPic.DeltaPocSX);

        std::for_each(refRPS.pic, refRPS.pic + refRPS.num_negative_pics + refRPS.num_positive_pics
            , [&](const STRPSPic& refPic)
        {
            AddDPoc(oldPic.DeltaPocSX - refPic.DeltaPocSX);
        });
    });

    dPocs[0].sort(std::greater<mfxI16>());
    dPocs[1].sort(std::less<mfxI16>());
    dPocs[0].unique();
    dPocs[1].unique();

    mfxI16 dPoc = 0;
    bool bDPocFound = false;
    auto NextDPock = [&]()
    {
        return ((!dPocs[0].empty() || !dPocs[1].empty()) && !bDPocFound);
    };

    while (NextDPock())
    {
        dPoc *= -1;
        bool bPositive = (dPoc > 0 && !dPocs[1].empty()) || dPocs[0].empty();
        dPoc = dPocs[bPositive].front();
        dPocs[bPositive].pop_front();

        std::for_each(newRPS.pic
            , newRPS.pic + refRPS.num_negative_pics + refRPS.num_positive_pics + 1
            , [&](STRPSPic& newPic)
        {
            newPic.used_by_curr_pic_flag = 0;
            newPic.use_delta_flag = 0;
        });

        auto pOldPic = oldRPS.pic;
        auto UseDelta = [&pOldPic, dPoc](STRPSPic& newPic, mfxI16 refDeltaPocSX, mfxI16 sign)
        {
            bool bUse =
                ((pOldPic->DeltaPocSX * sign) > 0)
                && ((pOldPic->DeltaPocSX - refDeltaPocSX) == dPoc);

            newPic.used_by_curr_pic_flag =
                (bUse && pOldPic->used_by_curr_pic_sx_flag)
                || (!bUse && newPic.used_by_curr_pic_flag);
            newPic.use_delta_flag = bUse || (!bUse && newPic.use_delta_flag);
            pOldPic += bUse;
        };

        auto pRefPic = refRPS.pic + refRPS.num_negative_pics + refRPS.num_positive_pics - 1;

        std::for_each(
            MakeRIter(newRPS.pic + refRPS.num_negative_pics + refRPS.num_positive_pics)
            , MakeRIter(newRPS.pic + refRPS.num_negative_pics)
            , [&](STRPSPic& newPic)
        {
            UseDelta(newPic, pRefPic->DeltaPocSX, -1);
            --pRefPic;
        });

        UseDelta(newRPS.pic[refRPS.num_negative_pics + refRPS.num_positive_pics], 0, (dPoc < 0) * -1);

        pRefPic = refRPS.pic;

        std::for_each(newRPS.pic, newRPS.pic + refRPS.num_negative_pics
            , [&](STRPSPic& newPic)
        {
            UseDelta(newPic, pRefPic->DeltaPocSX, -1);
            ++pRefPic;
        });

        if (pOldPic != (oldRPS.pic + oldRPS.num_negative_pics))
            continue;

        pRefPic = refRPS.pic + refRPS.num_negative_pics - 1;

        std::for_each(
            MakeRIter(newRPS.pic + refRPS.num_negative_pics)
            , MakeRIter(newRPS.pic)
            , [&](STRPSPic& newPic)
        {
            UseDelta(newPic, pRefPic->DeltaPocSX, +1);
            --pRefPic;
        });

        UseDelta(newRPS.pic[refRPS.num_negative_pics + refRPS.num_positive_pics], 0, dPoc > 0);

        pRefPic = refRPS.pic + refRPS.num_negative_pics;

        std::for_each(
            newRPS.pic + refRPS.num_negative_pics
            , newRPS.pic + refRPS.num_negative_pics + refRPS.num_positive_pics
            , [&](STRPSPic& newPic)
        {
            UseDelta(newPic, pRefPic->DeltaPocSX, +1);
            ++pRefPic;
        });

        bDPocFound = (pOldPic == (oldRPS.pic + oldRPS.num_negative_pics + oldRPS.num_positive_pics));
    }

    newRPS.delta_rps_sign       = (dPoc < 0);
    newRPS.abs_delta_rps_minus1 = mfxU16(abs(dPoc) - 1);

    SetIf(rps, bDPocFound, newRPS);

    return bDPocFound;
}

void OptimizeSTRPS(const STRPS* pSpsRps, mfxU8 n, STRPS& oldRPS, mfxU8 idx)
{
    auto  IsEnoughPics   = [&](const STRPS& refRPS)
    {
        return (refRPS.num_negative_pics + refRPS.num_positive_pics + 1)
            < (oldRPS.num_negative_pics + oldRPS.num_positive_pics);
    };
    auto  itRBegin       = MakeRIter(pSpsRps + idx);
    auto  itREnd         = MakeRIter(pSpsRps + ((idx < n && idx > 1) * (idx - 1)));
    auto  itSearchRBegin = std::find_if_not(itRBegin, itREnd, IsEnoughPics);
    mfxU8 dIdxMinus1     = mfxU8(std::distance(itRBegin, itSearchRBegin));
    auto  UpdateRPS      = [&](const STRPS& refRPS)
    {
        STRPS newRPS = oldRPS;
        bool bUpdateRps =
            GetInterRps(refRPS, newRPS, dIdxMinus1++)
            && (EstimateRpsBits(pSpsRps, n, newRPS, idx) < EstimateRpsBits(pSpsRps, n, oldRPS, idx));
        SetIf(oldRPS, bUpdateRps, newRPS);
    };
    auto    itSearchREnd = itREnd;
    bool    b1Ref = (idx < n) && (itSearchRBegin != itREnd);

    SetIf(itSearchREnd, b1Ref, std::next(itSearchRBegin, b1Ref));
    std::for_each(itSearchRBegin, itSearchREnd, UpdateRPS);
}

bool Equal(const STRPS & l, const STRPS & r)
{
    //ignore inter_ref_pic_set_prediction_flag, check only DeltaPocSX
    auto IsSame = [](const STRPSPic & l, const STRPSPic & r)
    {
        return
            l.DeltaPocSX == r.DeltaPocSX
            && l.used_by_curr_pic_sx_flag == r.used_by_curr_pic_sx_flag;
    };
    auto nPic = l.num_negative_pics + l.num_positive_pics;

    return
        l.num_negative_pics == r.num_negative_pics
        && l.num_positive_pics == r.num_positive_pics
        && (std::mismatch(l.pic, l.pic + nPic, r.pic, IsSame) == std::make_pair(l.pic + nPic, r.pic + nPic));
}

void Legacy::SetSTRPS(
    const Defaults::Param& dflts
    , SPS& sps
    , const Reorderer& reorder)
{
    std::list<StorageRW> frames;
    auto&     par        = dflts.mvp;
    STRPS     sets[65]   = {};
    auto      pSetsBegin = sets;
    auto      pSetsEnd   = pSetsBegin;
    bool      bTL        = dflts.base.GetNumTemporalLayers(dflts) > 1;
    mfxI32    nGops      = par.mfx.IdrInterval + !par.mfx.IdrInterval * 4;
    mfxI32    stDist     = std::min<mfxI32>(par.mfx.GopPicSize * nGops, 128);
    mfxI32    lastIPoc   = 0;
    bool      bDone      = false;
    mfxI32    i          = 0;

    mfxI32    RAPPOC     = -1;   // if >= 0 first frame with bigger POC clears refs previous to RAP
    bool      bFields    = (par.mfx.FrameInfo.PicStruct & (MFX_PICSTRUCT_FIELD_TOP | MFX_PICSTRUCT_FIELD_BOTTOM));
    bool      bIisRAP    = !bFields; // control to match real encoding here

    Reorderer localReorder;
    DpbArray  dpb;
    localReorder        = reorder;
    localReorder.DPB    = &dpb; //use own DPB

    do
    {
        {
            FrameBaseInfo fi;
            auto sts = dflts.base.GetPreReorderInfo(dflts, fi, nullptr, nullptr, 0, lastIPoc, mfxU32(i));
            ThrowIf(!!sts, "failed at GetPreReorderInfo");

            frames.push_back(StorageRW());
            frames.back().Insert(Task::Common::Key, new FrameBaseInfo(fi));
        }

        auto frIt  = localReorder(frames.begin(), frames.end(), false);
        bool bNext = frIt == frames.end();

        FrameBaseInfo* cur = nullptr;
        SetIf(cur, !bNext, [&]() { return &frIt->Write<FrameBaseInfo>(Task::Common::Key); });

        bDone =
            (i > 0 && !bNext && IsIdr(cur->FrameType))
            || (!bNext && cur->POC >= stDist)
            || (pSetsEnd + 1) >= std::end(sets);

        bNext |= bDone;

        if (!bNext)
        {
            bool bAfterRAP = (RAPPOC >= 0) && (cur->POC > RAPPOC); // if true - need to remove refs <RAPPOC

            RemoveIf(dpb, dpb + Size(dpb) * (bTL | bAfterRAP)
                , [&](DpbFrame& ref)
            {
                return
                    isValid(ref)
                    && ((bAfterRAP && ref.POC != RAPPOC && !ref.isLTR) // only RAP and LTR remains
                        || (ref.TemporalID > 0 && ref.TemporalID >= cur->TemporalID));
            });

            bool bIDR = IsIdr(cur->FrameType);
            bool bI   = IsI(cur->FrameType);
            bool bB   = IsB(cur->FrameType);
            bool bP   = IsP(cur->FrameType);
            bool bRef = IsRef(cur->FrameType);

            SetIf(RAPPOC, bAfterRAP, -1);           // clear after use
            SetIf(RAPPOC, bI && bIisRAP, cur->POC); // enable at I if conrol allows

            if (!bIDR)
            {
                mfxU8 nRef[2] = {};
                mfxU8 rpl[2][MAX_DPB_SIZE];

                std::fill_n(rpl[0], Size(rpl[0]), IDX_INVALID);
                std::fill_n(rpl[1], Size(rpl[1]), IDX_INVALID);

                auto SetRPL = [&]()
                {
                    ConstructRPL(dflts, dpb, *cur, rpl, nRef);
                    return true;
                };

                std::tie(nRef[0], nRef[1]) = dflts.base.GetFrameNumRefActive(dflts, *cur);

                bool bRPL =
                       (bB && nRef[0] && SetRPL())
                    || (bP && nRef[0] && SetRPL())
                    || (bI); //I picture is not using any refs, but saves them in RPS to be used by future pics.
                // but currently every I is RAP and frames after it won't use refs before it

                ThrowAssert(!bRPL, "failed to construct RPL");

                STRPS rps = {};
                ConstructSTRPS(dpb, rpl, nRef, cur->POC, rps);

                auto pCurSet = std::find_if(pSetsBegin, pSetsEnd
                    , [&](const STRPS& x) { return Equal(x, rps); });

                pSetsEnd += SetIf(*pCurSet, pSetsEnd == pCurSet, rps);
                ++pCurSet->WeightInGop;
            }

            SetIf(lastIPoc, bI, cur->POC);

            DpbFrame tmp;
            (FrameBaseInfo&)tmp = *cur;
            tmp.Rec.Idx += bRef;
            ThrowAssert(bRef && !UpdateDPB(dflts, tmp, dpb), "failed to UpdateDPB");

            frames.erase(frIt);
        }

        ++i;
    } while (!bDone);

    mfxU8 nSet         = mfxU8(std::distance(pSetsBegin, pSetsEnd));
    auto  IsRpsOptimal = [&nSet, &par, pSetsBegin](const STRPS& curRps)
    {
        STRPS  rps   = curRps;
        mfxU32 n     = curRps.WeightInGop; //current RPS used for N frames
        //bits for RPS in SPS and SSHs
        mfxU32 bits0 =
            EstimateRpsBits(pSetsBegin, nSet, rps, nSet - 1) //bits for RPS in SPS
            + (CeilLog2(nSet + 1) - CeilLog2(nSet)) * 2 //diff of bits for STRPS num in SPS (ue() coded)
            + (nSet > 1) * (par.mfx.NumSlice * CeilLog2(nSet) * n); //bits for RPS idx in SSHs

        // count frames that use SPS RPS
        auto AccFrWithRPS = [](mfxU32 x, const STRPS& r)
        {
            return std::move(x) + r.inter_ref_pic_set_prediction_flag * r.WeightInGop;
        };
        if (CeilLog2(nSet) - CeilLog2(nSet - 1)) //diff RPS idx bits with bigger RPS for ALL frames
            bits0 = par.mfx.NumSlice * std::accumulate(pSetsBegin, pSetsBegin + nSet - 1, bits0, AccFrWithRPS);

        //emulate removal of current RPS from SPS
        --nSet;
        rps.inter_ref_pic_set_prediction_flag = 0;
        OptimizeSTRPS(pSetsBegin, nSet, rps, nSet);

        //bits for RPS in SSHs (no RPS in SPS)
        mfxU32 bits1 =
            EstimateRpsBits(pSetsBegin, nSet, rps, nSet)
            * par.mfx.NumSlice
            * n;

        return bits0 <= bits1;
    };

    std::sort(pSetsBegin, pSetsEnd
        , [&](const STRPS& l, const STRPS& r) { return l.WeightInGop > r.WeightInGop; });

    i = 0;
    std::for_each(pSetsBegin, pSetsEnd
        , [&](STRPS& sf) { OptimizeSTRPS(sets, nSet, sf, mfxU8(i++)); });
    
    auto ritLastRps = std::find_if(MakeRIter(pSetsEnd), MakeRIter(pSetsBegin), IsRpsOptimal);
    // Also makes sense to try cut nSet to 2^n. Shorter idx code can overweight

    sps.num_short_term_ref_pic_sets = mfxU8(std::distance(ritLastRps, MakeRIter(pSetsBegin)));
    
    std::copy_n(pSetsBegin, sps.num_short_term_ref_pic_sets, sps.strps);
}

mfxStatus Legacy::CheckSPS(const SPS& sps, const ENCODE_CAPS_HEVC& caps, eMFXHWType hw)
{
    MFX_CHECK_COND(
           sps.log2_min_luma_coding_block_size_minus3 == 0
        && sps.separate_colour_plane_flag == 0
        && sps.pcm_enabled_flag == 0);

    if (hw >= MFX_HW_CNL)
    {
        MFX_CHECK_COND(sps.amp_enabled_flag == 1);
    }
    else
    {
        MFX_CHECK_COND(sps.amp_enabled_flag == 0);
        MFX_CHECK_COND(sps.sample_adaptive_offset_enabled_flag == 0);
    }

    MFX_CHECK_COND(
      !(   (!caps.YUV444ReconSupport && (sps.chroma_format_idc == 3))
        || (!caps.YUV422ReconSupport && (sps.chroma_format_idc == 2))
        || (caps.Color420Only && (sps.chroma_format_idc != 1))));

    MFX_CHECK_COND(
      !(   sps.pic_width_in_luma_samples > caps.MaxPicWidth
        || sps.pic_height_in_luma_samples > caps.MaxPicHeight));

    MFX_CHECK_COND(
      !(   (caps.MaxEncodedBitDepth == 0 || caps.BitDepth8Only)
        && (sps.bit_depth_luma_minus8 != 0 || sps.bit_depth_chroma_minus8 != 0)));

    MFX_CHECK_COND(
      !(   (caps.MaxEncodedBitDepth == 2 || caps.MaxEncodedBitDepth == 1 || !caps.BitDepth8Only)
        && ( !(sps.bit_depth_luma_minus8 == 0
            || sps.bit_depth_luma_minus8 == 2
            || sps.bit_depth_luma_minus8 == 4)
          || !(sps.bit_depth_chroma_minus8 == 0
            || sps.bit_depth_chroma_minus8 == 2
            || sps.bit_depth_chroma_minus8 == 4))));

    MFX_CHECK_COND(
      !(   caps.MaxEncodedBitDepth == 2
        && ( !(sps.bit_depth_luma_minus8 == 0
            || sps.bit_depth_luma_minus8 == 2
            || sps.bit_depth_luma_minus8 == 4)
          || !(sps.bit_depth_chroma_minus8 == 0
            || sps.bit_depth_chroma_minus8 == 2
            || sps.bit_depth_chroma_minus8 == 4))));

    MFX_CHECK_COND(
      !(   caps.MaxEncodedBitDepth == 3
        && ( !(sps.bit_depth_luma_minus8 == 0
            || sps.bit_depth_luma_minus8 == 2
            || sps.bit_depth_luma_minus8 == 4
            || sps.bit_depth_luma_minus8 == 8)
          || !(sps.bit_depth_chroma_minus8 == 0
            || sps.bit_depth_chroma_minus8 == 2
            || sps.bit_depth_chroma_minus8 == 4
            || sps.bit_depth_chroma_minus8 == 8))));

    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckPPS(const PPS& pps, const ENCODE_CAPS_HEVC& caps, eMFXHWType /*hw*/)
{
    if (pps.tiles_enabled_flag)
    {
        MFX_CHECK_COND(pps.loop_filter_across_tiles_enabled_flag);
    }

    MFX_CHECK_COND(!((mfxU32)(((pps.num_tile_columns_minus1 + 1) * (pps.num_tile_rows_minus1 + 1)) > 1) > caps.TileSupport));

    return MFX_ERR_NONE;
}

void SetDefaultFormat(
    mfxVideoParam& par
    , const Defaults::Param& defPar
    , mfxExtCodingOption3* pCO3)
{
    auto& fi = par.mfx.FrameInfo;

    assert(fi.FourCC);

    SetDefault(fi.BitDepthLuma, [&]() { return defPar.base.GetMaxBitDepth(defPar); });
    SetDefault(fi.BitDepthChroma, fi.BitDepthLuma);

    if (pCO3)
    {
        pCO3->TargetChromaFormatPlus1 = defPar.base.GetTargetChromaFormat(defPar);
        pCO3->TargetBitDepthLuma = defPar.base.GetTargetBitDepthLuma(defPar);
        SetDefault(pCO3->TargetBitDepthChroma, pCO3->TargetBitDepthLuma);
    }
}

void SetDefaultSize(
    mfxVideoParam & par
    , const Defaults::Param& defPar
    , mfxExtHEVCParam* pHEVC)
{
    auto& fi = par.mfx.FrameInfo;
    mfxU16 PicWidthInLumaSamples = defPar.base.GetCodedPicWidth(defPar);
    mfxU16 PicHeightInLumaSamples = defPar.base.GetCodedPicHeight(defPar);

    if (pHEVC)
    {
        SetDefault(pHEVC->PicWidthInLumaSamples, PicWidthInLumaSamples);
        SetDefault(pHEVC->PicHeightInLumaSamples, PicHeightInLumaSamples);
    }

    SetDefault(fi.CropW, mfxU16(PicWidthInLumaSamples - fi.CropX));
    SetDefault(fi.CropH, mfxU16(PicHeightInLumaSamples - fi.CropY));
    SetDefault(fi.AspectRatioW, mfxU16(1));
    SetDefault(fi.AspectRatioH, mfxU16(1));

    std::tie(fi.FrameRateExtN, fi.FrameRateExtD) = defPar.base.GetFrameRate(defPar);
}

void SetDefaultGOP(
    mfxVideoParam& par
    , const Defaults::Param& defPar
    , mfxExtCodingOption2* pCO2
    , mfxExtCodingOption3* pCO3)
{
    par.mfx.GopPicSize = defPar.base.GetGopPicSize(defPar);
    par.mfx.GopRefDist = defPar.base.GetGopRefDist(defPar);

    SetIf(pCO2->BRefType, pCO2 && !pCO2->BRefType, [&]() { return defPar.base.GetBRefType(defPar); });
    SetIf(pCO3->PRefType, pCO3 && !pCO3->PRefType, [&]() { return defPar.base.GetPRefType(defPar); });

    par.mfx.NumRefFrame = defPar.base.GetNumRefFrames(defPar);

    if (pCO3)
    {
        SetDefault<mfxU16>(pCO3->GPB, MFX_CODINGOPTION_ON);

        defPar.base.GetNumRefActive(
            defPar
            , &pCO3->NumRefActiveP
            , &pCO3->NumRefActiveBL0
            , &pCO3->NumRefActiveBL1);
    }
}

void SetDefaultBRC(
    mfxVideoParam& par
    , const Defaults::Param& defPar
    , mfxExtCodingOption2* pCO2
    , mfxExtCodingOption3* pCO3)
{
    par.mfx.RateControlMethod = defPar.base.GetRateControlMethod(defPar);
    BufferSizeInKB(par.mfx) = defPar.base.GetBufferSizeInKB(defPar);

    if (pCO2)
        pCO2->MBBRC = defPar.base.GetMBBRC(defPar);

    bool bSetQP = par.mfx.RateControlMethod == MFX_RATECONTROL_CQP
        && !(par.mfx.QPI && par.mfx.QPP && par.mfx.QPB);
    bool bSetRCPar = (par.mfx.RateControlMethod == MFX_RATECONTROL_CBR
        || par.mfx.RateControlMethod == MFX_RATECONTROL_VBR
        || par.mfx.RateControlMethod == MFX_RATECONTROL_QVBR
        || par.mfx.RateControlMethod == MFX_RATECONTROL_VCM
        || par.mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT);
    bool bSetICQ  = (par.mfx.RateControlMethod == MFX_RATECONTROL_ICQ);
    bool bSetQVBR = (par.mfx.RateControlMethod == MFX_RATECONTROL_QVBR && pCO3);

    if (bSetQP)
    {
        std::tie(par.mfx.QPI, par.mfx.QPP, par.mfx.QPB) = defPar.base.GetQPMFX(defPar);
    }

    if (bSetRCPar)
    {
        TargetKbps(par.mfx) = defPar.base.GetTargetKbps(defPar);
        SetDefault<mfxU16>(par.mfx.MaxKbps, par.mfx.TargetKbps);
        SetDefault<mfxU16>(par.mfx.InitialDelayInKB
            , par.mfx.BufferSizeInKB * (2 + (par.mfx.RateControlMethod == MFX_RATECONTROL_VBR && Legacy::IsSWBRC(par, ExtBuffer::Get(par)))) / 4);
    }

    if (bSetICQ)
        SetDefault<mfxU16>(par.mfx.ICQQuality, 26);

    if (bSetQVBR)
        SetDefault<mfxU16>(pCO3->QVBRQuality, 26);

    if (pCO3)
    {
        SetDefault<mfxU16>(pCO3->LowDelayBRC, MFX_CODINGOPTION_OFF);

        pCO3->EnableQPOffset = defPar.base.GetQPOffset(defPar, &pCO3->QPOffset);

        SetDefault<mfxU16>(pCO3->EnableMBQP
            , Bool2CO(
              !(   par.mfx.RateControlMethod == MFX_RATECONTROL_CQP
                || Legacy::IsSWBRC(par, pCO2)
                || !defPar.caps.MbQpDataSupport)));

        bool bSetWinBRC = pCO3->WinBRCSize || pCO3->WinBRCMaxAvgKbps;

        if (bSetWinBRC)
        {
            SetDefault<mfxU16>(pCO3->WinBRCSize
                , (mfxU16)CeilDiv(par.mfx.FrameInfo.FrameRateExtN, par.mfx.FrameInfo.FrameRateExtD));
            SetDefault<mfxU16>(pCO3->WinBRCMaxAvgKbps, par.mfx.MaxKbps);
        }

        SetDefault<mfxU16>(pCO3->BRCPanicMode, Bool2CO(defPar.caps.HRDConformanceSupport));
    }
}

void SetDefaultEsOptions(
    mfxVideoParam& par
    , const Defaults::Param& defPar
    , mfxExtHEVCParam* pHEVC
    , mfxExtCodingOption* pCO
    , mfxExtCodingOption2* pCO2
    , mfxExtCodingOption3* pCO3)
{
    if (pCO)
    {
        bool bHRDConformance = defPar.base.GetHRDConformanceON(defPar);

        SetDefault(pCO->NalHrdConformance, Bool2CO(bHRDConformance));
        SetDefault(pCO->VuiNalHrdParameters, Bool2CO(bHRDConformance));
        SetDefault(pCO->AUDelimiter, mfxU16(MFX_CODINGOPTION_OFF));

        pCO->PicTimingSEI = defPar.base.GetPicTimingSEI(defPar);
    }

    if (pCO2)
        SetDefault(pCO2->RepeatPPS, mfxU16(MFX_CODINGOPTION_OFF));

    if (pCO3)
    {
        SetDefault(pCO3->TransformSkip, mfxU16(MFX_CODINGOPTION_ON * (defPar.hw >= MFX_HW_ICL)));
        SetDefault(pCO3->TransformSkip, mfxU16(MFX_CODINGOPTION_OFF));
        SetDefault(pCO3->EnableNalUnitType, Bool2CO(!!par.mfx.EncodedOrder));
    }

    if (pHEVC)
    {
        bool bNoSAO =
            SetDefault(pHEVC->SampleAdaptiveOffset, mfxU16(MFX_SAO_ENABLE_LUMA | MFX_SAO_ENABLE_CHROMA))
            && defPar.base.CheckSAO(defPar, par);

        pHEVC->SampleAdaptiveOffset *= !bNoSAO;
        SetDefault(pHEVC->SampleAdaptiveOffset, mfxU16(MFX_SAO_DISABLE));
    }
}

void Legacy::SetDefaults(
    mfxVideoParam& par
    , const Defaults::Param& defPar
    , bool bExternalFrameAllocator)
{
    auto&                    fi            = par.mfx.FrameInfo;
    mfxExtHEVCParam*         pHEVC         = ExtBuffer::Get(par);
    mfxExtHEVCTiles*         pTile         = ExtBuffer::Get(par);
    mfxExtAvcTemporalLayers* pTL           = ExtBuffer::Get(par);
    mfxExtCodingOption*      pCO           = ExtBuffer::Get(par);
    mfxExtCodingOption2*     pCO2          = ExtBuffer::Get(par);
    mfxExtCodingOption3*     pCO3          = ExtBuffer::Get(par);
    mfxU16                   IOPByAlctr[2] = { MFX_IOPATTERN_IN_SYSTEM_MEMORY, MFX_IOPATTERN_IN_VIDEO_MEMORY };
    auto GetNumSlices = [&]()
    {
        std::vector<SliceInfo> slices;
        return defPar.base.GetSlices(defPar, slices);
    };
    auto GetDefaultLevel = [&]()
    {
        mfxU16 nCol, nRow;
        std::tie(nCol, nRow) = defPar.base.GetNumTiles(defPar);

        return GetMinLevel(
            fi.FrameRateExtN
            , fi.FrameRateExtD
            , defPar.base.GetCodedPicWidth(defPar)
            , defPar.base.GetCodedPicHeight(defPar)
            , par.mfx.NumRefFrame
            , nCol
            , nRow
            , par.mfx.NumSlice
            , BufferSizeInKB(par.mfx)
            , MaxKbps(par.mfx)
            , MFX_LEVEL_HEVC_1);
    };

    if (pHEVC)
    {
        pHEVC->LCUSize = defPar.base.GetLCUSize(defPar);
    }

    SetDefaultFormat(par, defPar, pCO3);
    SetDefault(par.mfx.CodecProfile, defPar.base.GetProfile(defPar));
    SetDefault(par.AsyncDepth, defPar.base.GetAsyncDepth(defPar));
    SetDefault(par.IOPattern, IOPByAlctr[!!bExternalFrameAllocator]);
    SetDefault(par.mfx.TargetUsage, mfxU16(4)) && CheckTU(par, defPar.caps);

    if (pTile)
    {
        std::tie(pTile->NumTileColumns, pTile->NumTileRows) = defPar.base.GetNumTiles(defPar);
    }

    SetDefault(par.mfx.NumSlice, GetNumSlices);

    SetDefaultSize(par, defPar, pHEVC);
    SetDefaultGOP(par, defPar, pCO2, pCO3);
    SetDefaultBRC(par, defPar, pCO2, pCO3);

    if (pTL)
    {
        SetDefault(pTL->Layer[0].Scale, mfxU16(1));
    }

    SetDefault(par.mfx.CodecLevel, GetDefaultLevel);

    SetDefaultEsOptions(par, defPar, pHEVC, pCO, pCO2, pCO3);

    if (pCO2)
    {
        auto minQP = defPar.base.GetMinQPMFX(defPar);
        auto maxQP = defPar.base.GetMaxQPMFX(defPar);

        SetDefault(pCO2->IntRefCycleSize, mfxU16(CeilDiv(fi.FrameRateExtN, fi.FrameRateExtD) * !!pCO2->IntRefType));

        SetDefault(pCO2->MinQPI, minQP);
        SetDefault(pCO2->MinQPP, minQP);
        SetDefault(pCO2->MinQPB, minQP);
        SetDefault(pCO2->MaxQPI, maxQP);
        SetDefault(pCO2->MaxQPP, maxQP);
        SetDefault(pCO2->MaxQPB, maxQP);
    }

    bool bSetConstr = pHEVC && (par.mfx.CodecProfile >= MFX_PROFILE_HEVC_REXT && !pHEVC->GeneralConstraintFlags);

    if (bSetConstr)
    {
        auto& constr = pHEVC->GeneralConstraintFlags;
        mfxU16 bdY = defPar.base.GetTargetBitDepthLuma(defPar);
        mfxU16 cf = defPar.base.GetTargetChromaFormat(defPar) - 1;

        constr |= MFX_HEVC_CONSTR_REXT_MAX_422CHROMA * (cf <= MFX_CHROMAFORMAT_YUV422);
        constr |= MFX_HEVC_CONSTR_REXT_MAX_420CHROMA * (cf <= MFX_CHROMAFORMAT_YUV420);
        constr |= MFX_HEVC_CONSTR_REXT_MAX_12BIT * (bdY <= 12);
        constr |= MFX_HEVC_CONSTR_REXT_MAX_10BIT * (bdY <= 10);
        //there is no Main 4:2:2 in current standard spec.(2016/12), only Main 4:2:2 10
        constr |= MFX_HEVC_CONSTR_REXT_MAX_8BIT * (bdY <= 8 && (cf != MFX_CHROMAFORMAT_YUV422));
        constr |= MFX_HEVC_CONSTR_REXT_LOWER_BIT_RATE;
    }
}

mfxStatus Legacy::CheckLevelConstraints(
    mfxVideoParam & par
    , const Defaults::Param& defPar)
{
    MFX_CHECK(par.mfx.CodecLevel, MFX_ERR_NONE);

    mfxU16 PicWidthInLumaSamples    = defPar.base.GetCodedPicWidth(defPar);
    mfxU16 PicHeightInLumaSamples   = defPar.base.GetCodedPicHeight(defPar);
    mfxU16 MinRef                   = defPar.base.GetNumRefFrames(defPar);
    mfxU32 NumSlice                 = defPar.base.GetNumSlices(defPar);
    mfxU32 BufferSizeInKB           = defPar.base.GetBufferSizeInKB(defPar);
    mfxU32 MaxKbps                  = 0;
    mfxU16 rc                       = defPar.base.GetRateControlMethod(defPar);
    auto   tiles                    = defPar.base.GetNumTiles(defPar);
    auto   frND                     = defPar.base.GetFrameRate(defPar);

    SetIf(MaxKbps
        , rc != MFX_RATECONTROL_CQP && rc != MFX_RATECONTROL_ICQ
        , [&]() { return defPar.base.GetMaxKbps(defPar); });

    mfxU16 minLevel = GetMinLevel(
          std::get<0>(frND)
        , std::get<1>(frND)
        , PicWidthInLumaSamples
        , PicHeightInLumaSamples
        , MinRef
        , std::get<0>(tiles)
        , std::get<1>(tiles)
        , NumSlice
        , BufferSizeInKB
        , MaxKbps
        , par.mfx.CodecLevel);

    MFX_CHECK(!CheckMinOrClip(par.mfx.CodecLevel, minLevel), MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);

    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckESPackParam(mfxVideoParam & par, eMFXHWType hw)
{
    mfxU32 changed = 0;
    mfxExtCodingOption*     pCO = ExtBuffer::Get(par);
    mfxExtCodingOption2*    pCO2 = ExtBuffer::Get(par);
    mfxExtCodingOption3*    pCO3 = ExtBuffer::Get(par);
    mfxExtVideoSignalInfo*  pVSI = ExtBuffer::Get(par);

    if (pCO)
    {
        bool bNoHRD = par.mfx.RateControlMethod != MFX_RATECONTROL_CBR
            && par.mfx.RateControlMethod != MFX_RATECONTROL_VBR
            && par.mfx.RateControlMethod != MFX_RATECONTROL_VCM
            && par.mfx.RateControlMethod != MFX_RATECONTROL_QVBR;

        changed += CheckOrZero<mfxU16>(
            pCO->NalHrdConformance
            , mfxU16(MFX_CODINGOPTION_UNKNOWN)
            , mfxU16(MFX_CODINGOPTION_OFF)
            , mfxU16(MFX_CODINGOPTION_ON * !bNoHRD));

        changed += CheckOrZero<mfxU16>(
            pCO->VuiNalHrdParameters
            , mfxU16(MFX_CODINGOPTION_UNKNOWN)
            , mfxU16(MFX_CODINGOPTION_OFF)
            , mfxU16(MFX_CODINGOPTION_ON * !(bNoHRD || IsOff(pCO->NalHrdConformance))));

        changed += CheckTriStateOrZero(pCO->PicTimingSEI);
        changed += CheckTriStateOrZero(pCO->AUDelimiter);
    }

    if (pCO2)
        changed += CheckTriStateOrZero(pCO2->RepeatPPS);

    if (pVSI)
    {
        changed += CheckRangeOrSetDefault<mfxU16>(pVSI->VideoFormat, 0, 8, 5);
        changed += CheckRangeOrSetDefault<mfxU16>(pVSI->ColourPrimaries, 0, 255, 2);
        changed += CheckRangeOrSetDefault<mfxU16>(pVSI->TransferCharacteristics, 0, 255, 2);
        changed += CheckRangeOrSetDefault<mfxU16>(pVSI->MatrixCoefficients, 0, 255, 2);
        changed += CheckOrZero<mfxU16, 0, 1>(pVSI->VideoFullRange);
        changed += CheckOrZero<mfxU16, 0, 1>(pVSI->ColourDescriptionPresent);
    }

    if (pCO3)
    {
        changed += CheckOrZero<mfxU16>(
            pCO3->TransformSkip
            , mfxU16(MFX_CODINGOPTION_UNKNOWN)
            , mfxU16(MFX_CODINGOPTION_OFF)
            , mfxU16(MFX_CODINGOPTION_ON * (hw >= MFX_HW_CNL)));

        changed += CheckOrZero<mfxU16>(
            pCO3->EnableNalUnitType
            , mfxU16(MFX_CODINGOPTION_UNKNOWN)
            , mfxU16(MFX_CODINGOPTION_OFF)
            , mfxU16(MFX_CODINGOPTION_ON * !!par.mfx.EncodedOrder));

#if MFX_VERSION >= MFX_VERSION_NEXT
        changed += CheckRangeOrSetDefault<mfxI16>(pCO3->DeblockingAlphaTcOffset, -12, 12, 0);
        changed += CheckRangeOrSetDefault<mfxI16>(pCO3->DeblockingBetaOffset, -12, 12, 0);
#endif
    }

    MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckGPB(mfxVideoParam & par)
{
    mfxU32 changed = 0;
    mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

    if (pCO3)
    {
        changed += CheckOrZero<mfxU16>(
            pCO3->GPB
            , mfxU16(MFX_CODINGOPTION_ON)
            , mfxU16(MFX_CODINGOPTION_OFF * !!m_pQWCDefaults->caps.msdk.PSliceSupport)
            , mfxU16(MFX_CODINGOPTION_UNKNOWN));
    }

    MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckSkipFrame(mfxVideoParam & par)
{
    mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par);

    if (pCO2 && CheckOrZero<mfxU16
        , MFX_SKIPFRAME_NO_SKIP
        , MFX_SKIPFRAME_INSERT_DUMMY
        , MFX_SKIPFRAME_INSERT_NOTHING>
        (pCO2->SkipFrame))
        return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;

    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckIntraRefresh(
    mfxVideoParam & par
    , const Defaults::Param& defPar)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxU32 changed = 0;
    mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par);
    mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

    if (pCO2)
    {
        MFX_CHECK_NO_RET(
            !CheckMaxOrClip(pCO2->IntRefType, MFX_REFRESH_HORIZONTAL)
            , sts, MFX_ERR_UNSUPPORTED);

        MFX_CHECK_NO_RET(
            defPar.caps.RollingIntraRefresh
            || !CheckOrZero<mfxU16>(pCO2->IntRefType, MFX_REFRESH_NO)
            , sts, MFX_ERR_UNSUPPORTED);

        MFX_CHECK_NO_RET(
            defPar.caps.RollingIntraRefresh
            || !CheckOrZero<mfxU16>(pCO2->IntRefCycleSize, 0)
            , sts, MFX_ERR_UNSUPPORTED);

        MFX_CHECK_NO_RET(
            defPar.caps.RollingIntraRefresh
            || !pCO3
            || !CheckOrZero<mfxU16>(pCO3->IntRefCycleDist, 0)
            , sts, MFX_ERR_UNSUPPORTED);

        // B-Frames should be disabled for intra refresh
        if (pCO2->IntRefType && par.mfx.GopRefDist > 1)
        {
            pCO2->IntRefType = MFX_REFRESH_NO;
            ++changed;
        }

        // refresh cycle length shouldn't be greater or equal to GOP size
        bool bInvalidCycle =
            pCO2->IntRefCycleSize != 0
            && par.mfx.GopPicSize != 0
            && pCO2->IntRefCycleSize >= par.mfx.GopPicSize;

        pCO2->IntRefType *= !bInvalidCycle;
        pCO2->IntRefCycleSize *= !bInvalidCycle;
        changed += bInvalidCycle;

        // refresh period shouldn't be greater than refresh cycle size
        bool bInvalidDist =
            pCO3
            && pCO3->IntRefCycleDist != 0
            && pCO2->IntRefCycleSize != 0
            && pCO2->IntRefCycleSize > pCO3->IntRefCycleDist;

        if (bInvalidDist)
        {
            pCO3->IntRefCycleDist = 0;
            ++changed;
        }

        mfxI16 qpDiff = defPar.base.GetMaxQPMFX(defPar) - defPar.base.GetMinQPMFX(defPar);

        changed += CheckRangeOrSetDefault(pCO2->IntRefQPDelta, mfxI16(-qpDiff), qpDiff, mfxI16(0));
    }
    MFX_CHECK_STS(sts);

    MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckTemporalLayers(mfxVideoParam & par)
{
    mfxU32 changed = 0;
    mfxExtAvcTemporalLayers* pTL = ExtBuffer::Get(par);

    MFX_CHECK(pTL, MFX_ERR_NONE);
    MFX_CHECK(!CheckOrZero<mfxU16>(pTL->Layer[0].Scale, 0, 1), MFX_ERR_UNSUPPORTED);
    MFX_CHECK(!CheckOrZero<mfxU16>(pTL->Layer[7].Scale, 0), MFX_ERR_UNSUPPORTED);

    mfxU16 nTL = 1;

    for (mfxU16 i = 1, prev = 0; i < 7; ++i)
    {
        if (!pTL->Layer[i].Scale)
            continue;

        auto& scaleCurr = pTL->Layer[i].Scale;
        auto  scalePrev = pTL->Layer[prev].Scale;

        MFX_CHECK(!CheckMinOrZero(scaleCurr, scalePrev + 1), MFX_ERR_UNSUPPORTED);
        MFX_CHECK(scalePrev, MFX_ERR_UNSUPPORTED);
        MFX_CHECK(!CheckOrZero(scaleCurr, mfxU16(scaleCurr - (scaleCurr % scalePrev))), MFX_ERR_UNSUPPORTED);

        prev = i;
        ++nTL;
    }

    changed += CheckOrZero<mfxU16>(par.mfx.GopRefDist, 0, 1, par.mfx.GopRefDist * (nTL <= 1));

    MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckPPyramid(mfxVideoParam & par)
{
    mfxU32 changed = 0;
    mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

    if (pCO3)
    {
        changed += CheckOrZero<mfxU16
            , MFX_P_REF_DEFAULT
            , MFX_P_REF_SIMPLE
            , MFX_P_REF_PYRAMID>
            (pCO3->PRefType);

        if (pCO3->PRefType == MFX_P_REF_PYRAMID && par.mfx.GopRefDist > 1)
        {
            pCO3->PRefType = MFX_P_REF_DEFAULT;
            changed++;
        }
    }

    MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckBPyramid(
    mfxVideoParam & par
    , const Defaults::Param& defPar)
{
    mfxU32 changed = 0;
    mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par);

    if (pCO2)
    {
        mfxU16 minRefForPyramid = 3;

        if (par.mfx.GopRefDist)
            minRefForPyramid = defPar.base.GetMinRefForBPyramid(defPar);

        bool bNoBPyramid =
            par.mfx.GopRefDist > 0
            && (   par.mfx.GopRefDist < 2
                || minRefForPyramid > 16
                || (par.mfx.NumRefFrame
                    && (minRefForPyramid > par.mfx.NumRefFrame)
                    && !defPar.base.GetNonStdReordering(defPar)));

        changed += CheckOrZero(pCO2->BRefType
            , mfxU16(MFX_B_REF_UNKNOWN)
            , mfxU16(MFX_B_REF_OFF)
            , mfxU16(MFX_B_REF_PYRAMID * !bNoBPyramid));
    }

    MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckSlices(
    mfxVideoParam & par
    , const Defaults::Param& defPar)
{
    mfxU32 changed = 0;
    mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par);
    bool bCheckNMB = pCO2 && pCO2->NumMbPerSlice;

    if (bCheckNMB)
    {
        auto   tiles            = defPar.base.GetNumTiles(defPar);
        mfxU16 W                = defPar.base.GetCodedPicWidth(defPar);
        mfxU16 H                = defPar.base.GetCodedPicHeight(defPar);
        mfxU16 LCUSize          = defPar.base.GetLCUSize(defPar);
        mfxU32 nLCU             = CeilDiv(W, LCUSize) * CeilDiv(H, LCUSize);
        mfxU32 nTile            = std::get<0>(tiles) * std::get<1>(tiles);
        mfxU32 maxSlicesPerTile = MAX_SLICES / nTile;
        mfxU32 maxSlicesTotal   = maxSlicesPerTile * nTile;
        mfxU32 maxNumMbPerSlice = CeilDiv(nLCU, nTile);
        mfxU32 minNumMbPerSlice = CeilDiv(nLCU, maxSlicesTotal);

        changed += CheckMinOrClip(pCO2->NumMbPerSlice, minNumMbPerSlice);
        changed += CheckMaxOrClip(pCO2->NumMbPerSlice, maxNumMbPerSlice);
    }

    std::vector<SliceInfo> slices;

    auto supportedNslices = defPar.base.GetSlices(defPar, slices);
    if (par.mfx.NumSlice)
    {
        changed += CheckRangeOrSetDefault(par.mfx.NumSlice, supportedNslices, supportedNslices, supportedNslices);
    }

    if (bCheckNMB)
    {
        auto itMaxSlice = std::max_element(slices.begin(), slices.end()
            , [](SliceInfo a, SliceInfo b){ return a.NumLCU < b.NumLCU; });

        if (itMaxSlice != std::end(slices))
            changed += CheckMinOrClip(pCO2->NumMbPerSlice, itMaxSlice->NumLCU);
    }

    MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckNumRefFrame(
    mfxVideoParam & par
    , const Defaults::Param& defPar)
{
    mfxU32 changed = 0;

    changed += CheckMaxOrClip(par.mfx.NumRefFrame, defPar.base.GetMaxDPB(defPar) - 1);
    changed += SetIf(
        par.mfx.NumRefFrame
        , (par.mfx.GopRefDist > 1
            && par.mfx.NumRefFrame == 1
            && !defPar.base.GetNonStdReordering(defPar))
        , defPar.base.GetMinRefForBNoPyramid(defPar));

    MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckFrameRate(mfxVideoParam & par)
{
    auto& fi = par.mfx.FrameInfo;

    if (fi.FrameRateExtN && fi.FrameRateExtD) // FR <= 300
    {
        if (fi.FrameRateExtN > mfxU32(300 * fi.FrameRateExtD))
        {
            fi.FrameRateExtN = fi.FrameRateExtD = 0;
            return MFX_ERR_UNSUPPORTED;
        }
    }

    if ((fi.FrameRateExtN == 0) != (fi.FrameRateExtD == 0))
    {
        fi.FrameRateExtN = 0;
        fi.FrameRateExtD = 0;
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

bool Legacy::IsInVideoMem(const mfxVideoParam & par, const mfxExtOpaqueSurfaceAlloc* pOSA)
{
    if (par.IOPattern == MFX_IOPATTERN_IN_VIDEO_MEMORY)
        return true;

    if (par.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
        return (!pOSA || (pOSA->In.Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET)));

    return false;
}

mfxStatus Legacy::CheckShift(mfxVideoParam & par, mfxExtOpaqueSurfaceAlloc* pOSA)
{
    auto& fi = par.mfx.FrameInfo;
    bool bVideoMem = IsInVideoMem(par, pOSA);

    if (bVideoMem && !fi.Shift)
    {
        if (fi.FourCC == MFX_FOURCC_P010 || fi.FourCC == MFX_FOURCC_P210)
        {
            fi.Shift = 1;
            return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckCrops(
    mfxVideoParam & par
    , const Defaults::Param& defPar)
{
    mfxU32 changed = 0;

    auto W = defPar.base.GetCodedPicWidth(defPar);
    auto H = defPar.base.GetCodedPicHeight(defPar);
    auto& fi = par.mfx.FrameInfo;

    changed += CheckMaxOrClip(fi.CropX, W);
    changed += CheckMaxOrClip(fi.CropW, W - fi.CropX);
    changed += CheckMaxOrClip(fi.CropY, H);
    changed += CheckMaxOrClip(fi.CropH, H - fi.CropY);

    MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    return MFX_ERR_NONE;
}

bool Legacy::IsSWBRC(mfxVideoParam const & par, const mfxExtCodingOption2* pCO2)
{
    return
        (      pCO2 && IsOn(pCO2->ExtBRC)
            && (   par.mfx.RateControlMethod == MFX_RATECONTROL_CBR
                || par.mfx.RateControlMethod == MFX_RATECONTROL_VBR))
        || par.mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT;
}

bool CheckBufferSizeInKB(
    mfxVideoParam & par
    , const Defaults::Param& defPar
    , mfxU16 bd)
{
    mfxU32 changed = 0;
    auto W = defPar.base.GetCodedPicWidth(defPar);
    auto H = defPar.base.GetCodedPicHeight(defPar);
    mfxU16 cf = defPar.base.GetTargetChromaFormat(defPar) - 1;

    mfxU32 rawBytes = Legacy::GetRawBytes(W, H, cf, bd) / 1000;

    bool bCqpOrIcq =
        par.mfx.RateControlMethod == MFX_RATECONTROL_CQP
        || par.mfx.RateControlMethod == MFX_RATECONTROL_ICQ;

    bool bSetToRaw = bCqpOrIcq && BufferSizeInKB(par.mfx) < rawBytes;

    if (bSetToRaw)
    {
        BufferSizeInKB(par.mfx) = rawBytes;
        changed++;
    }
    else if (!bCqpOrIcq)
    {
        mfxU32 frN, frD;

        std::tie(frN, frD) = defPar.base.GetFrameRate(defPar);
        mfxU32 avgFS = mfxU32(std::ceil((mfxF64)TargetKbps(par.mfx) * frD / frN / 8));

        if (BufferSizeInKB(par.mfx) < avgFS * 2 + 1)
        {
            BufferSizeInKB(par.mfx) = avgFS * 2 + 1;
            changed++;
        }

        if (par.mfx.CodecLevel)
        {
            mfxU32 maxCPB = GetMaxCpbInKBByLevel(par);

            if (BufferSizeInKB(par.mfx) > maxCPB)
            {
                BufferSizeInKB(par.mfx) = maxCPB;
                changed++;
            }
        }
    }

    return !!changed;
}

mfxStatus Legacy::CheckBRC(
    mfxVideoParam & par
    , const Defaults::Param& defPar)
{
    mfxU32 changed = 0;
    mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par);
    mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

    if (par.mfx.RateControlMethod == MFX_RATECONTROL_AVBR)
    {
        par.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
        par.mfx.Accuracy          = 0;
        par.mfx.Convergence       = 0;
        changed++;
    }

    MFX_CHECK(!CheckOrZero<mfxU16>(par.mfx.RateControlMethod
        , 0
        , !!defPar.caps.msdk.CBRSupport * MFX_RATECONTROL_CBR
        , !!defPar.caps.msdk.VBRSupport * MFX_RATECONTROL_VBR
        , !!defPar.caps.msdk.CQPSupport * MFX_RATECONTROL_CQP
        , MFX_RATECONTROL_LA_EXT
        , !!defPar.caps.msdk.ICQSupport * MFX_RATECONTROL_ICQ
        , !!defPar.caps.VCMBitRateControl * MFX_RATECONTROL_VCM
        , !!defPar.caps.QVBRBRCSupport * MFX_RATECONTROL_QVBR), MFX_ERR_UNSUPPORTED);

    MFX_CHECK(par.mfx.RateControlMethod != MFX_RATECONTROL_ICQ
        || !CheckMaxOrZero(par.mfx.ICQQuality, 51), MFX_ERR_UNSUPPORTED);

    changed += ((   par.mfx.RateControlMethod == MFX_RATECONTROL_VBR
                 || par.mfx.RateControlMethod == MFX_RATECONTROL_QVBR
                 || par.mfx.RateControlMethod == MFX_RATECONTROL_VCM)
                && par.mfx.MaxKbps != 0
                && par.mfx.TargetKbps != 0)
        && CheckMinOrClip(par.mfx.MaxKbps, par.mfx.TargetKbps)
        && CheckMaxOrClip(par.mfx.TargetKbps, par.mfx.MaxKbps);

    auto bd = defPar.base.GetTargetBitDepthLuma(defPar);
    auto minQP = defPar.base.GetMinQPMFX(defPar);
    auto maxQP = defPar.base.GetMaxQPMFX(defPar);

    if (par.mfx.RateControlMethod == MFX_RATECONTROL_CQP)
    {
        changed += par.mfx.QPI && CheckMinOrClip<mfxU16>(par.mfx.QPI, minQP);
        changed += par.mfx.QPI && CheckMaxOrClip<mfxU16>(par.mfx.QPI, maxQP);
        changed += par.mfx.QPP && CheckMinOrClip<mfxU16>(par.mfx.QPP, minQP);
        changed += par.mfx.QPP && CheckMaxOrClip<mfxU16>(par.mfx.QPP, maxQP);
        changed += par.mfx.QPB && CheckMinOrClip<mfxU16>(par.mfx.QPB, minQP);
        changed += par.mfx.QPB && CheckMaxOrClip<mfxU16>(par.mfx.QPB, maxQP);
    }

    changed += par.mfx.BufferSizeInKB && CheckBufferSizeInKB(par, defPar, bd);
    
    if (pCO3)
    {
        MFX_CHECK(par.mfx.RateControlMethod != MFX_RATECONTROL_QVBR
            || !CheckMaxOrZero(pCO3->QVBRQuality, 51), MFX_ERR_UNSUPPORTED);

        auto    GopRefDist  = defPar.base.GetGopRefDist(defPar);
        mfxU16  QPPB[2]     = { par.mfx.QPP, par.mfx.QPB };
        mfxI16  QPX         = QPPB[GopRefDist != 1] * (par.mfx.RateControlMethod == MFX_RATECONTROL_CQP);
        bool    bNoQpOffset = (par.mfx.RateControlMethod != MFX_RATECONTROL_CQP)
            || (GopRefDist > 1 && defPar.base.GetBRefType(defPar) == MFX_B_REF_OFF)
            || (GopRefDist == 1 && defPar.base.GetPRefType(defPar) == MFX_P_REF_SIMPLE);

        changed += CheckOrZero<mfxU16>(pCO3->EnableQPOffset
            , mfxU16(MFX_CODINGOPTION_UNKNOWN)
            , mfxU16(MFX_CODINGOPTION_OFF)
            , mfxU16(MFX_CODINGOPTION_ON * !bNoQpOffset));

        auto CheckQPOffset = [&](mfxI16& QPO)
        {
            return
                CheckMinOrClip(QPO, minQP - QPX)
                + CheckMaxOrClip(QPO, maxQP - QPX);
        };

        changed +=
            IsOn(pCO3->EnableQPOffset)
            && QPX
            && std::count_if(std::begin(pCO3->QPOffset), std::end(pCO3->QPOffset), CheckQPOffset);

        changed += CheckOrZero(pCO3->EnableMBQP
            , mfxU16(MFX_CODINGOPTION_UNKNOWN)
            , mfxU16(MFX_CODINGOPTION_OFF)
            , mfxU16(MFX_CODINGOPTION_ON * !!(defPar.caps.MbQpDataSupport)));

        auto sts = defPar.base.CheckWinBRC(defPar, par);
        MFX_CHECK(sts >= MFX_ERR_NONE, sts);

        changed += (sts > MFX_ERR_NONE);

        changed += CheckOrZero(pCO3->BRCPanicMode
            , mfxU16(MFX_CODINGOPTION_UNKNOWN)
            , mfxU16(MFX_CODINGOPTION_OFF)
            , mfxU16(MFX_CODINGOPTION_ON * !!(defPar.caps.HRDConformanceSupport)));
    }

    if (pCO2)
    {
        changed += CheckTriStateOrZero(pCO2->MBBRC);
        changed += (   defPar.caps.MBBRCSupport == 0
                    || par.mfx.RateControlMethod == MFX_RATECONTROL_CQP
                    || IsSWBRC(par, pCO2))
                && CheckOrZero<mfxU16, 0, MFX_CODINGOPTION_OFF>(pCO2->MBBRC);

        changed += !defPar.caps.SliceByteSizeCtrl && CheckOrZero<mfxU32, 0>(pCO2->MaxSliceSize);

        bool bMinMaxQpAllowed = (par.mfx.RateControlMethod != MFX_RATECONTROL_CQP) && (IsSWBRC(par, pCO2)||IsOn(par.mfx.LowPower));

        if (bMinMaxQpAllowed)
        {
            changed += pCO2->MinQPI && CheckRangeOrSetDefault<mfxU8>(pCO2->MinQPI,                        minQP,  maxQP, 0);
            changed += pCO2->MaxQPI && CheckRangeOrSetDefault<mfxU8>(pCO2->MaxQPI, std::max(pCO2->MinQPI, minQP), maxQP, 0);
            changed += pCO2->MinQPP && CheckRangeOrSetDefault<mfxU8>(pCO2->MinQPP,                        minQP,  maxQP, 0);
            changed += pCO2->MaxQPP && CheckRangeOrSetDefault<mfxU8>(pCO2->MaxQPP, std::max(pCO2->MinQPP, minQP), maxQP, 0);
            changed += pCO2->MinQPB && CheckRangeOrSetDefault<mfxU8>(pCO2->MinQPB,                        minQP,  maxQP, 0);
            changed += pCO2->MaxQPB && CheckRangeOrSetDefault<mfxU8>(pCO2->MaxQPB, std::max(pCO2->MinQPB, minQP), maxQP, 0);
        }
        else
        {
            changed += CheckOrZero<mfxU8>(pCO2->MinQPI, 0, minQP);
            changed += CheckOrZero<mfxU8>(pCO2->MaxQPI, 0, maxQP);
            changed += CheckOrZero<mfxU8>(pCO2->MinQPP, 0, minQP);
            changed += CheckOrZero<mfxU8>(pCO2->MaxQPP, 0, maxQP);
            changed += CheckOrZero<mfxU8>(pCO2->MinQPB, 0, minQP);
            changed += CheckOrZero<mfxU8>(pCO2->MaxQPB, 0, maxQP);
        }
    }

    MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);

    return MFX_ERR_NONE;
}

mfxU32 Legacy::GetRawBytes(mfxU16 w, mfxU16 h, mfxU16 ChromaFormat, mfxU16 BitDepth)
{
    mfxU32 s = w * h;

    if (ChromaFormat == MFX_CHROMAFORMAT_YUV420)
        s = s * 3 / 2;
    else if (ChromaFormat == MFX_CHROMAFORMAT_YUV422)
        s *= 2;
    else if (ChromaFormat == MFX_CHROMAFORMAT_YUV444)
        s *= 3;

    assert(BitDepth >= 8);
    if (BitDepth != 8)
        s = (s * BitDepth + 7) / 8;

    return s;
}
mfxStatus Legacy::CheckIOPattern(mfxVideoParam & par)
{
    bool check_result = Check<mfxU16
                            , MFX_IOPATTERN_IN_VIDEO_MEMORY
                            , MFX_IOPATTERN_IN_SYSTEM_MEMORY
                            , MFX_IOPATTERN_IN_OPAQUE_MEMORY
                            , 0>
                            (par.IOPattern);

    MFX_CHECK(!check_result, MFX_ERR_INVALID_VIDEO_PARAM);

    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckGopRefDist(mfxVideoParam & par, const ENCODE_CAPS_HEVC& caps)
{
    MFX_CHECK(par.mfx.GopRefDist, MFX_ERR_NONE);

    mfxU16 maxRefDist = std::max<mfxU16>(1, !caps.SliceIPOnly * (par.mfx.GopPicSize - 1));
    MFX_CHECK(!CheckMaxOrClip(par.mfx.GopRefDist, maxRefDist), MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);

    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckTU(mfxVideoParam & par, const ENCODE_CAPS_HEVC& caps)
{
    auto& tu = par.mfx.TargetUsage;

    if (CheckMaxOrZero(tu, 7u))
        return MFX_ERR_UNSUPPORTED;

    if (!tu)
        return MFX_ERR_NONE;

    auto support = caps.TUSupport;
    mfxI16 abs_diff = 0;
    bool   sign = 0;
    mfxI16 newtu = tu;

    do
    {
        newtu = tu + (1 - 2 * sign) * abs_diff;
        abs_diff += !sign;
        sign = !sign;
    } while (!(support & (1 << (newtu - 1))) && newtu > 0);

    if (tu != newtu)
    {
        tu = newtu;
        return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    return MFX_ERR_NONE;

}

mfxStatus Legacy::CheckTiles(
    mfxVideoParam & par
    , const Defaults::Param& defPar)
{
    mfxExtHEVCTiles* pTile = ExtBuffer::Get(par);
    MFX_CHECK(pTile, MFX_ERR_NONE);

    mfxU16
        MaxTileColumns = MAX_NUM_TILE_COLUMNS
        , MaxTileRows    = MAX_NUM_TILE_ROWS
        , changed = 0;

    if (!defPar.caps.TileSupport)
    {
        MaxTileColumns = 1;
        MaxTileRows = 1;
    }
    else
    {
        mfxU32 minTileWidth  = MIN_TILE_WIDTH_IN_SAMPLES;
        mfxU32 minTileHeight = MIN_TILE_HEIGHT_IN_SAMPLES;
        
        // min 2x2 lcu is supported on VDEnc
        // TODO: replace indirect NumScalablePipesMinus1 by platform
        SetIf(minTileHeight, defPar.caps.NumScalablePipesMinus1 > 0 && IsOn(par.mfx.LowPower), 128);

        mfxU16 maxCol = std::max<mfxU16>(1, mfxU16(defPar.base.GetCodedPicWidth(defPar) / minTileWidth));
        mfxU16 maxRow = std::max<mfxU16>(1, mfxU16(defPar.base.GetCodedPicHeight(defPar) / minTileHeight));

        changed += CheckMaxOrClip(pTile->NumTileColumns, maxCol);
        changed += CheckMaxOrClip(pTile->NumTileRows, maxRow);

        // for ICL VDEnc only 1xN or Nx1 configurations are allowed for single pipe
        // we ignore "Rows" condition
        bool bForce1Row =
            (defPar.hw == MFX_HW_ICL || defPar.hw == MFX_HW_ICL_LP)
            && IsOn(par.mfx.LowPower)
            && pTile->NumTileColumns > 1
            && pTile->NumTileRows > 1;

        changed += bForce1Row && CheckMaxOrClip(pTile->NumTileRows, 1);
    }

    MFX_CHECK(!CheckMaxOrClip(pTile->NumTileColumns, MaxTileColumns), MFX_ERR_UNSUPPORTED);
    MFX_CHECK(!CheckMaxOrClip(pTile->NumTileRows, MaxTileRows), MFX_ERR_UNSUPPORTED);


    MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);

    return MFX_ERR_NONE;
}

void Legacy::CheckQuery0(const ParamSupport& sprt, mfxVideoParam& par)
{
    std::vector<mfxU8> onesBuf(sizeof(par), 1);

    auto ExtParam    = par.ExtParam;
    auto NumExtParam = par.NumExtParam;

    par = mfxVideoParam{};

    for (auto& copy : sprt.m_mvpCopySupported)
        copy((mfxVideoParam*)onesBuf.data(), &par);

    par.ExtParam    = ExtParam;
    par.NumExtParam = NumExtParam;

    if (par.ExtParam)
    {
        for (mfxU32 i = 0; i < par.NumExtParam; i++)
        {
            if (!par.ExtParam[i])
                continue;

            mfxExtBuffer header = *par.ExtParam[i];

            memset(par.ExtParam[i], 0, header.BufferSz);
            *par.ExtParam[i] = header;

            auto it = sprt.m_ebCopySupported.find(header.BufferId);

            if (it != sprt.m_ebCopySupported.end())
            {
                if (onesBuf.size() < header.BufferSz)
                    onesBuf.insert(onesBuf.end(), header.BufferSz - mfxU32(onesBuf.size()), 1);

                auto pSrc = (mfxExtBuffer*)onesBuf.data();
                *pSrc = header;

                for (auto& copy : it->second)
                    copy(pSrc, par.ExtParam[i]);
            }
        }
    }
}

mfxStatus Legacy::CheckBuffers(const ParamSupport& sprt, const mfxVideoParam& in, const mfxVideoParam* out)
{
    MFX_CHECK(!(!in.NumExtParam && (!out || !out->NumExtParam)), MFX_ERR_NONE);
    MFX_CHECK(in.ExtParam, MFX_ERR_UNDEFINED_BEHAVIOR);
    MFX_CHECK(!(out && (!out->ExtParam || out->NumExtParam != in.NumExtParam))
        , MFX_ERR_UNDEFINED_BEHAVIOR);

    std::map<mfxU32, mfxU32> detected[2];
    mfxU32 dId = 0;

    for (auto pPar : { &in, out })
    {
        if (!pPar)
            continue;

        for (mfxU32 i = 0; i < pPar->NumExtParam; i++)
        {
            MFX_CHECK_NULL_PTR1(pPar->ExtParam[i]);

            auto id = pPar->ExtParam[i]->BufferId;

            MFX_CHECK(sprt.m_ebCopySupported.find(id) != sprt.m_ebCopySupported.end(), MFX_ERR_UNSUPPORTED);
            MFX_CHECK(!(detected[dId][id]++), MFX_ERR_UNDEFINED_BEHAVIOR);
        }
        dId++;
    }

    MFX_CHECK(!(out && detected[0] != detected[1]), MFX_ERR_UNDEFINED_BEHAVIOR);

    return MFX_ERR_NONE;
}

mfxStatus Legacy::CopyConfigurable(const ParamSupport& sprt, const mfxVideoParam& in, mfxVideoParam& out)
{
    using TFnCopyMVP = std::function<void(const mfxVideoParam*, mfxVideoParam*)>;
    using TFnCopyEB  = std::function<void(const mfxExtBuffer*, mfxExtBuffer*)>;

    auto CopyMVP = [&](const mfxVideoParam& src, mfxVideoParam& dst)
    {
        std::for_each(sprt.m_mvpCopySupported.begin(), sprt.m_mvpCopySupported.end()
            , [&](const TFnCopyMVP& copy)
        {
            copy(&src, &dst);
        });
    };
    auto CopyEB = [](const std::list<TFnCopyEB>& copyList, const mfxExtBuffer* pIn, mfxExtBuffer* pOut)
    {
        std::for_each(copyList.begin(), copyList.end()
            , [&](const TFnCopyEB& copy)
        {
            copy(pIn, pOut);
        });
    };
    mfxVideoParam tmpMVP = {};

    CopyMVP(in, tmpMVP);

    tmpMVP.NumExtParam  = out.NumExtParam;
    tmpMVP.ExtParam     = out.ExtParam;

    out = tmpMVP;

    std::list<mfxExtBuffer*> outBufs(out.ExtParam, out.ExtParam + out.NumExtParam);
    outBufs.sort();
    outBufs.remove(nullptr);

    std::for_each(outBufs.begin(), outBufs.end()
        , [&](mfxExtBuffer* pEbOut)
    {
        std::vector<mfxU8> ebTmp(pEbOut->BufferSz, mfxU8(0));
        auto               pEbIn      = ExtBuffer::Get(in, pEbOut->BufferId);
        auto               copyIt     = sprt.m_ebCopySupported.find(pEbOut->BufferId);
        auto               copyPtrsIt = sprt.m_ebCopyPtrs.find(pEbOut->BufferId);
        mfxExtBuffer*      pEbTmp     = (mfxExtBuffer*)ebTmp.data();
        bool               bCopyPar   = pEbIn && copyIt != sprt.m_ebCopySupported.end();
        bool               bCopyPtr   = copyPtrsIt != sprt.m_ebCopyPtrs.end();

        *pEbTmp = *pEbOut;

        if (bCopyPtr)
        {
            CopyEB(copyPtrsIt->second, pEbOut, pEbTmp);
        }

        if (bCopyPar)
        {
            CopyEB(copyIt->second, pEbIn, pEbTmp);
        }

        std::copy_n(ebTmp.data(), ebTmp.size(), (mfxU8*)pEbOut);
    });

    return MFX_ERR_NONE;
}

mfxStatus Legacy::CheckCodedPicSize(
    mfxVideoParam & par
    , const Defaults::Param& defPar)
{
    mfxExtHEVCParam* pHEVC = ExtBuffer::Get(par);
    MFX_CHECK(pHEVC, MFX_ERR_NONE);

    auto alignment = defPar.base.GetCodedPicAlignment(defPar);
    auto& W = pHEVC->PicWidthInLumaSamples;
    auto& H = pHEVC->PicHeightInLumaSamples;
    auto AW = mfx::align2_value(W, alignment);
    auto AH = mfx::align2_value(H, alignment);

    MFX_CHECK(!CheckMaxOrZero(W, par.mfx.FrameInfo.Width), MFX_ERR_UNSUPPORTED);
    MFX_CHECK(!CheckMaxOrZero(H, par.mfx.FrameInfo.Height), MFX_ERR_UNSUPPORTED);

    if ((W != AW) || (H != AH))
    {
        W = AW;
        H = AH;
        return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    return MFX_ERR_NONE;
}

mfxU16 FrameType2SliceType(mfxU32 ft)
{
    bool bB = IsB(ft);
    bool bP = !bB && IsP(ft);
    return 1 * bP + 2 * !(bB || bP);
}

bool isCurrLt(
    DpbArray const & DPB,
    mfxU8 const (&RPL)[2][MAX_DPB_SIZE],
    mfxU8 const (&numRefActive)[2],
    mfxI32 poc)
{
    for (mfxU32 i = 0; i < 2; i++)
        for (mfxU32 j = 0; j < numRefActive[i]; j++)
            if (poc == DPB[RPL[i][j]].POC)
                return DPB[RPL[i][j]].isLTR;
    return false;
}

inline bool isCurrLt(const TaskCommonPar & task, mfxI32 poc)
{
    return isCurrLt(task.DPB.Active, task.RefPicList, task.NumRefActive, poc);
}

template<class T> inline T Lsb(T val, mfxU32 maxLSB)
{
    if (val >= 0)
        return val % maxLSB;
    return (maxLSB - ((-val) % maxLSB)) % maxLSB;
}

bool isForcedDeltaPocMsbPresent(
    const TaskCommonPar & prevTask,
    mfxI32 poc,
    mfxU32 MaxPocLsb)
{
    DpbArray const & DPB = prevTask.DPB.Active;

    if (Lsb(prevTask.POC, MaxPocLsb) == Lsb(poc, MaxPocLsb))
        return true;

    for (mfxU16 i = 0; !isDpbEnd(DPB, i); i++)
        if (DPB[i].POC != poc && Lsb(DPB[i].POC, MaxPocLsb) == Lsb(poc, MaxPocLsb))
            return true;

    return false;
}

mfxU16 GetSliceHeaderLTRs(
    const TaskCommonPar& task
    , const TaskCommonPar& prevTask
    , const DpbArray & DPB
    , const SPS& sps
    , mfxI32 (&LTR)[MAX_NUM_LONG_TERM_PICS]
    , Slice & s)
{
    mfxU16 nLTR = 0;
    size_t nDPBLT = 0;
    mfxU32 MaxPocLsb  = (1<<(sps.log2_max_pic_order_cnt_lsb_minus4+4));
    mfxU32 dPocCycleMSBprev = 0;
    mfxI32 DPBLT[MAX_DPB_SIZE] = {};
    mfxI32 InvalidPOC = -9000;

    std::transform(DPB, DPB + Size(DPB), DPBLT
        , [InvalidPOC](const DpbFrame& x) { return (x.isLTR && isValid(x)) ? x.POC : InvalidPOC; });

    nDPBLT = std::remove_if(DPBLT, DPBLT + Size(DPBLT)
        , [InvalidPOC](mfxI32 x) { return x == InvalidPOC; }) - DPBLT;

    std::sort(DPBLT, DPBLT + nDPBLT, std::greater<mfxI32>()); // sort for DeltaPocMsbCycleLt (may only increase)

    // insert LTR using lt_ref_pic_poc_lsb_sps
    std::for_each(DPBLT, DPBLT + nDPBLT, [&](mfxI32& ltpoc) {
        mfxU32 dPocCycleMSB = (task.POC / MaxPocLsb - ltpoc / MaxPocLsb);
        mfxU32 dPocLSB      = ltpoc - (task.POC - dPocCycleMSB * MaxPocLsb - s.pic_order_cnt_lsb);

        size_t ltId = std::find_if(
            sps.lt_ref_pic_poc_lsb_sps
            , sps.lt_ref_pic_poc_lsb_sps + sps.num_long_term_ref_pics_sps
            , [&](const mfxU16& ltPocLsb) {
            return dPocLSB == ltPocLsb
                &&     isCurrLt(task, DPBLT[ltpoc])
                    == !!sps.used_by_curr_pic_lt_sps_flag[sps.lt_ref_pic_poc_lsb_sps - &ltPocLsb]
                && dPocCycleMSB >= dPocCycleMSBprev;
        }) - sps.lt_ref_pic_poc_lsb_sps;

        if (ltId >= sps.num_long_term_ref_pics_sps)
            return;

        auto& curlt = s.lt[s.num_long_term_sps];

        curlt.lt_idx_sps                    = ltId;
        curlt.used_by_curr_pic_lt_flag      = !!sps.used_by_curr_pic_lt_sps_flag[ltId];
        curlt.poc_lsb_lt                    = sps.lt_ref_pic_poc_lsb_sps[ltId];
        curlt.delta_poc_msb_cycle_lt        = dPocCycleMSB - dPocCycleMSBprev;
        curlt.delta_poc_msb_present_flag    =
            !!curlt.delta_poc_msb_cycle_lt || isForcedDeltaPocMsbPresent(prevTask, ltpoc, MaxPocLsb);
        dPocCycleMSBprev                    = dPocCycleMSB;

        ++s.num_long_term_sps;

        if (curlt.used_by_curr_pic_lt_flag)
        {
            assert(nLTR < MAX_NUM_LONG_TERM_PICS);
            LTR[nLTR++] = ltpoc;
        }

        ltpoc = InvalidPOC;
    });

    nDPBLT = std::remove_if(DPBLT, DPBLT + nDPBLT
        , [InvalidPOC](mfxI32 x) { return x == InvalidPOC; }) - DPBLT;
    dPocCycleMSBprev = 0;

    std::for_each(DPBLT, DPBLT + nDPBLT, [&](mfxI32 ltpoc) {
        auto& curlt = s.lt[s.num_long_term_sps + s.num_long_term_pics];
        mfxU32 dPocCycleMSB = (task.POC / MaxPocLsb - ltpoc / MaxPocLsb);
        mfxU32 dPocLSB      = ltpoc - (task.POC - dPocCycleMSB * MaxPocLsb - s.pic_order_cnt_lsb);

        assert(dPocCycleMSB >= dPocCycleMSBprev);

        curlt.used_by_curr_pic_lt_flag      = isCurrLt(task, ltpoc);
        curlt.poc_lsb_lt                    = dPocLSB;
        curlt.delta_poc_msb_cycle_lt        = dPocCycleMSB - dPocCycleMSBprev;
        curlt.delta_poc_msb_present_flag    =
            !!curlt.delta_poc_msb_cycle_lt || isForcedDeltaPocMsbPresent(prevTask, ltpoc, MaxPocLsb);
        dPocCycleMSBprev                    = dPocCycleMSB;

        ++s.num_long_term_pics;

        if (curlt.used_by_curr_pic_lt_flag)
        {
            if (nLTR < MAX_NUM_LONG_TERM_PICS) //KW
                LTR[nLTR++] = ltpoc;
            else
                assert(!"too much LTRs");
        }
    });

    return nLTR;
}

void GetSliceHeaderRPLMod(
    const TaskCommonPar& task
    , const DpbArray & DPB
    , const mfxI32 (&STR)[2][MAX_DPB_SIZE]
    , const mfxU16 (&nSTR)[2]
    , const mfxI32 LTR[MAX_NUM_LONG_TERM_PICS]
    , mfxU16 nLTR
    , Slice & s)
{
    auto& RPL = task.RefPicList;
    auto ModLX = [&](mfxU16 lx, mfxU16 NumRpsCurrTempListX)
    {
        mfxU16 rIdx = 0;
        mfxI32 RPLTempX[16] = {}; // default ref. list without modifications
        auto AddRef = [&](mfxI32 ref) { RPLTempX[rIdx++] = ref; };

        while (rIdx < NumRpsCurrTempListX)
        {
            std::for_each(STR[lx], STR[lx] + std::min<mfxU16>(nSTR[lx], NumRpsCurrTempListX - rIdx), AddRef);
            std::for_each(STR[!lx], STR[!lx] + std::min<mfxU16>(nSTR[!lx], NumRpsCurrTempListX - rIdx), AddRef);
            std::for_each(LTR, LTR + std::min<mfxU16>(nLTR, NumRpsCurrTempListX - rIdx), AddRef);
        }

        for (rIdx = 0; rIdx < task.NumRefActive[lx]; rIdx++)
        {
            auto pRef = std::find_if(RPLTempX, RPLTempX + NumRpsCurrTempListX
                , [&](mfxI32 ref) {return DPB[RPL[lx][rIdx]].POC == ref; });
            s.list_entry_lx[lx][rIdx] = mfxU8(pRef - RPLTempX);
            s.ref_pic_list_modification_flag_lx[lx] |= (s.list_entry_lx[lx][rIdx] != rIdx);
        }
    };

    ModLX(0, std::max<mfxU16>(nSTR[0] + nSTR[1] + nLTR, task.NumRefActive[0]));
    ModLX(1, std::max<mfxU16>(nSTR[0] + nSTR[1] + nLTR, task.NumRefActive[1]));
}

mfxStatus Legacy::GetSliceHeader(
    const ExtBuffer::Param<mfxVideoParam> & par
    , const TaskCommonPar& task
    , const SPS& sps
    , const PPS& pps
    , Slice & s)
{
    bool isP = IsP(task.FrameType);
    bool isB = IsB(task.FrameType);
    bool isI = IsI(task.FrameType);

    ThrowAssert(isP + isB + isI != 1, "task.FrameType is invalid");

    s = {};

    s.first_slice_segment_in_pic_flag = 1;
    s.no_output_of_prior_pics_flag    = 0;
    s.pic_parameter_set_id            = pps.pic_parameter_set_id;
    s.type                            = FrameType2SliceType(task.FrameType);
    s.pic_output_flag                 = !!pps.output_flag_present_flag;

    assert(0 == sps.separate_colour_plane_flag);

    bool bNonIdrNut = task.SliceNUT != IDR_W_RADL && task.SliceNUT != IDR_N_LP;
    if (bNonIdrNut)
    {
        mfxU16 nLTR = 0, nSTR[2] = {};
        mfxI32 STR[2][MAX_DPB_SIZE] = {};         // used short-term references
        mfxI32 LTR[MAX_NUM_LONG_TERM_PICS] = {};  // used long-term references
        const auto& DPB = task.DPB.Active;        // DPB before encoding
        const auto& RPL = task.RefPicList;        // Ref. Pic. List
        auto IsSame     = [&s](const STRPS& x)      { return Equal(x, s.strps); };
        auto IsLTR      = [](const DpbFrame& ref)   { return isValid(ref) && ref.isLTR; };
        auto IsL0Pic    = [](const STRPSPic& pic)   { return pic.used_by_curr_pic_sx_flag && (pic.DeltaPocSX <= 0); };
        auto IsL1Pic    = [](const STRPSPic& pic)   { return pic.used_by_curr_pic_sx_flag && (pic.DeltaPocSX > 0); };
        auto PicToPOC   = [&](const STRPSPic& pic)  { return (task.POC + pic.DeltaPocSX); };

        ConstructSTRPS(DPB, RPL, task.NumRefActive, task.POC, s.strps);

        s.pic_order_cnt_lsb = (task.POC & ~(0xFFFFFFFF << (sps.log2_max_pic_order_cnt_lsb_minus4 + 4)));

        s.short_term_ref_pic_set_idx      = mfxU8(std::find_if(sps.strps, sps.strps + sps.num_short_term_ref_pic_sets, IsSame) - sps.strps);
        s.short_term_ref_pic_set_sps_flag = (s.short_term_ref_pic_set_idx < sps.num_short_term_ref_pic_sets);

        if (!s.short_term_ref_pic_set_sps_flag)
            OptimizeSTRPS(sps.strps, sps.num_short_term_ref_pic_sets, s.strps, sps.num_short_term_ref_pic_sets);

        std::list<STRPSPic> picsUsed[2];
        auto itStrpsPicsBegin = s.strps.pic;
        auto itStrpsPicsEnd   = s.strps.pic + mfxU32(s.strps.num_negative_pics + s.strps.num_positive_pics);

        std::copy_if(itStrpsPicsBegin, itStrpsPicsEnd, std::back_inserter(picsUsed[0]), IsL0Pic);
        std::copy_if(itStrpsPicsBegin, itStrpsPicsEnd, std::back_inserter(picsUsed[1]), IsL1Pic);

        nSTR[0] = mfxU16(std::transform(picsUsed[0].begin(), picsUsed[0].end(), STR[0], PicToPOC) - STR[0]);
        nSTR[1] = mfxU16(std::transform(picsUsed[1].begin(), picsUsed[1].end(), STR[1], PicToPOC) - STR[1]);

        if (std::any_of(DPB, DPB + Size(DPB), IsLTR))
        {
            assert(sps.long_term_ref_pics_present_flag);
            nLTR = GetSliceHeaderLTRs(task, m_prevTask, DPB, sps, LTR, s);
        }


        if (pps.lists_modification_present_flag)
        {
            GetSliceHeaderRPLMod(task, DPB, STR, nSTR, LTR, nLTR, s);
        }
    }

    s.temporal_mvp_enabled_flag = !!sps.temporal_mvp_enabled_flag;

    if (sps.sample_adaptive_offset_enabled_flag)
    {
        const mfxExtHEVCParam* rtHEVCParam = ExtBuffer::Get(task.ctrl);
        const mfxExtHEVCParam& HEVCParam   = ExtBuffer::Get(par);
        mfxU16 FrameSAO = (rtHEVCParam && rtHEVCParam->SampleAdaptiveOffset)
            ? rtHEVCParam->SampleAdaptiveOffset
            : HEVCParam.SampleAdaptiveOffset;

        s.sao_luma_flag   = !!(FrameSAO & MFX_SAO_ENABLE_LUMA);
        s.sao_chroma_flag = !!(FrameSAO & MFX_SAO_ENABLE_CHROMA);
    }

    if (!isI)
    {
        s.num_ref_idx_active_override_flag =
           (   pps.num_ref_idx_l0_default_active_minus1 + 1 != task.NumRefActive[0]
            || (isB && pps.num_ref_idx_l1_default_active_minus1 + 1 != task.NumRefActive[1]));

        s.num_ref_idx_l0_active_minus1  = task.NumRefActive[0] - 1;
        s.num_ref_idx_l1_active_minus1  = isB * (task.NumRefActive[1] - 1);
        s.mvd_l1_zero_flag              &= !isB;
        s.cabac_init_flag               = 0;
        s.collocated_from_l0_flag       = !!s.temporal_mvp_enabled_flag;
        s.five_minus_max_num_merge_cand = 0;
    }

    bool bSetQPd =
        par.mfx.RateControlMethod == MFX_RATECONTROL_CQP
        || par.mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT;
    s.slice_qp_delta     = mfxI8((task.QpY - (pps.init_qp_minus26 + 26)) * bSetQPd);
    s.slice_cb_qp_offset = 0;
    s.slice_cr_qp_offset = 0;

    const mfxExtCodingOption2& CO2  = ExtBuffer::Get(par);
    const mfxExtCodingOption2* pCO2 = ExtBuffer::Get(task.ctrl);

    SetDefault(pCO2, &CO2);

    s.deblocking_filter_disabled_flag = !!pCO2->DisableDeblockingIdc;
    s.deblocking_filter_override_flag = (s.deblocking_filter_disabled_flag != pps.deblocking_filter_disabled_flag);
    s.beta_offset_div2                = pps.beta_offset_div2;
    s.tc_offset_div2                  = pps.tc_offset_div2;
#if MFX_VERSION >= MFX_VERSION_NEXT
    const mfxExtCodingOption3&   CO3 = ExtBuffer::Get(par);
    const mfxExtCodingOption3  *pCO3 = ExtBuffer::Get(task.ctrl);

    SetDefault(pCO3, &CO3);

    s.beta_offset_div2 = mfxI8(mfx::clamp(mfxI32(pCO3->DeblockingBetaOffset),    -12, 12) * 0.5 * !s.deblocking_filter_disabled_flag);
    s.tc_offset_div2   = mfxI8(mfx::clamp(mfxI32(pCO3->DeblockingAlphaTcOffset), -12, 12) * 0.5 * !s.deblocking_filter_disabled_flag);

    s.deblocking_filter_override_flag |=
        !s.deblocking_filter_disabled_flag
        && (s.beta_offset_div2 != pps.beta_offset_div2
            || s.tc_offset_div2 != pps.tc_offset_div2);
#endif

    s.loop_filter_across_slices_enabled_flag = pps.loop_filter_across_slices_enabled_flag;
    s.num_entry_point_offsets *= !(pps.tiles_enabled_flag || pps.entropy_coding_sync_enabled_flag);

    return MFX_ERR_NONE;
}


#endif
