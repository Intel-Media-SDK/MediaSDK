/* ****************************************************************************** *\

Copyright (C) 2012-2020 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: dump_mfxstructures.cpp

\* ****************************************************************************** */

#include "dump.h"
#include "../loggers/log.h"

std::string DumpContext::dump(const std::string structName, const mfxDecodeStat &decodeStat)
{
    std::string str;
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(decodeStat.reserved) + "\n";
    str += structName + ".NumFrame=" + ToString(decodeStat.NumFrame) + "\n";
    str += structName + ".NumSkippedFrame=" + ToString(decodeStat.NumSkippedFrame) + "\n";
    str += structName + ".NumError=" + ToString(decodeStat.NumError) + "\n";
    str += structName + ".NumCachedFrame=" + ToString(decodeStat.NumCachedFrame);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxEncodeCtrl &EncodeCtrl)
{
    std::string str;
    str += dump(structName + ".Header", EncodeCtrl.Header) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(EncodeCtrl.reserved) + "\n";
#if (MFX_VERSION >= 1025)
    str += structName + ".reserved1=" + ToString(EncodeCtrl.reserved1) + "\n";
    str += structName + ".MfxNalUnitType=" + ToString(EncodeCtrl.MfxNalUnitType) + "\n";
#endif
    str += structName + ".SkipFrame=" + ToString(EncodeCtrl.SkipFrame) + "\n";
    str += structName + ".QP=" + ToString(EncodeCtrl.QP) + "\n";
    str += structName + ".FrameType=" + ToString(EncodeCtrl.FrameType) + "\n";
    str += structName + ".NumPayload=" + ToString(EncodeCtrl.NumPayload) + "\n";
    str += structName + ".reserved2=" + ToString(EncodeCtrl.reserved2) + "\n";
    str += dump_mfxExtParams(structName, EncodeCtrl) + "\n";
    str += structName + ".Payload=" + ToString(EncodeCtrl.Payload);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxEncodeStat &encodeStat)
{
    std::string str;
    str += structName + ".NumBit=" + ToString(encodeStat.NumBit) + "\n";
    str += structName + ".NumCachedFrame=" + ToString(encodeStat.NumCachedFrame) + "\n";
    str += structName + ".NumFrame=" + ToString(encodeStat.NumFrame) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(encodeStat.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCodingOption &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    str += structName + ".reserved1=" + ToString(_struct.reserved1) + "\n";
    str += structName + ".RateDistortionOpt=" + ToString(_struct.RateDistortionOpt) + "\n";
    str += structName + ".MECostType=" + ToString(_struct.MECostType) + "\n";
    str += structName + ".MESearchType=" + ToString(_struct.MESearchType) + "\n";
    str += structName + ".MVSearchWindow.x=" + ToString(_struct.MVSearchWindow.x) + "\n";
    str += structName + ".MVSearchWindow.y=" + ToString(_struct.MVSearchWindow.y) + "\n";

    str += structName + ".EndOfSequence=" + ToString(_struct.EndOfSequence) + "\n";
    str += structName + ".FramePicture=" + ToString(_struct.FramePicture) + "\n";

    str += structName + ".CAVLC=" + ToString(_struct.CAVLC) + "\n";
    str += structName + ".reserved2[]=" + DUMP_RESERVED_ARRAY(_struct.reserved2) + "\n";
    str += structName + ".RecoveryPointSEI=" + ToString(_struct.RecoveryPointSEI) + "\n";
    str += structName + ".ViewOutput=" + ToString(_struct.ViewOutput) + "\n";
    str += structName + ".NalHrdConformance=" + ToString(_struct.NalHrdConformance) + "\n";
    str += structName + ".SingleSeiNalUnit=" + ToString(_struct.SingleSeiNalUnit) + "\n";
    str += structName + ".VuiVclHrdParameters=" + ToString(_struct.VuiVclHrdParameters) + "\n";

    str += structName + ".RefPicListReordering=" + ToString(_struct.RefPicListReordering) + "\n";
    str += structName + ".ResetRefList=" + ToString(_struct.ResetRefList) + "\n";
    str += structName + ".RefPicMarkRep=" + ToString(_struct.RefPicMarkRep) + "\n";
    str += structName + ".FieldOutput=" + ToString(_struct.FieldOutput) + "\n";

    str += structName + ".IntraPredBlockSize=" + ToString(_struct.IntraPredBlockSize) + "\n";
    str += structName + ".InterPredBlockSize=" + ToString(_struct.InterPredBlockSize) + "\n";
    str += structName + ".MVPrecision=" + ToString(_struct.MVPrecision) + "\n";
    str += structName + ".MaxDecFrameBuffering=" + ToString(_struct.MaxDecFrameBuffering) + "\n";

    str += structName + ".AUDelimiter=" + ToString(_struct.AUDelimiter) + "\n";
    str += structName + ".EndOfStream=" + ToString(_struct.EndOfStream) + "\n";
    str += structName + ".PicTimingSEI=" + ToString(_struct.PicTimingSEI) + "\n";
    str += structName + ".VuiNalHrdParameters=" + ToString(_struct.VuiNalHrdParameters) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCodingOption2 &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(IntRefType);
    DUMP_FIELD(IntRefCycleSize);
    DUMP_FIELD(IntRefQPDelta);

    DUMP_FIELD(MaxFrameSize);
    DUMP_FIELD(MaxSliceSize);

    DUMP_FIELD(BitrateLimit);
    DUMP_FIELD(MBBRC);
    DUMP_FIELD(ExtBRC);
    DUMP_FIELD(LookAheadDepth);
    DUMP_FIELD(Trellis);
    DUMP_FIELD(RepeatPPS);
    DUMP_FIELD(BRefType);
    DUMP_FIELD(AdaptiveI);
    DUMP_FIELD(AdaptiveB);
    DUMP_FIELD(LookAheadDS);
    DUMP_FIELD(NumMbPerSlice);
    DUMP_FIELD(SkipFrame);
    DUMP_FIELD(MinQPI);
    DUMP_FIELD(MaxQPI);
    DUMP_FIELD(MinQPP);
    DUMP_FIELD(MaxQPP);
    DUMP_FIELD(MinQPB);
    DUMP_FIELD(MaxQPB);
    DUMP_FIELD(FixedFrameRate);
    DUMP_FIELD(DisableDeblockingIdc);
    DUMP_FIELD(DisableVUI);
    DUMP_FIELD(BufferingPeriodSEI);
    DUMP_FIELD(EnableMAD);
    DUMP_FIELD(UseRawRef);

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCodingOption3 &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(NumSliceI);
    DUMP_FIELD(NumSliceP);
    DUMP_FIELD(NumSliceB);

    DUMP_FIELD(WinBRCMaxAvgKbps);
    DUMP_FIELD(WinBRCSize);

    DUMP_FIELD(QVBRQuality);
    DUMP_FIELD(EnableMBQP);
    DUMP_FIELD(IntRefCycleDist);
    DUMP_FIELD(DirectBiasAdjustment);          /* tri-state option */
    DUMP_FIELD(GlobalMotionBiasAdjustment);    /* tri-state option */
    DUMP_FIELD(MVCostScalingFactor);
    DUMP_FIELD(MBDisableSkipMap);              /* tri-state option */
    DUMP_FIELD(WeightedPred);
    DUMP_FIELD(WeightedBiPred);
    DUMP_FIELD(AspectRatioInfoPresent);
    DUMP_FIELD(OverscanInfoPresent);
    DUMP_FIELD(OverscanAppropriate);
    DUMP_FIELD(TimingInfoPresent);
    DUMP_FIELD(BitstreamRestriction);
    DUMP_FIELD(LowDelayHrd);                    /* tri-state option */
    DUMP_FIELD(MotionVectorsOverPicBoundaries); /* tri-state option */
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    DUMP_FIELD(Log2MaxMvLengthHorizontal);      /* 0..16 */
    DUMP_FIELD(Log2MaxMvLengthVertical);        /* 0..16 */
#else
    DUMP_FIELD_RESERVED(reserved1);
#endif

    DUMP_FIELD(ScenarioInfo);
    DUMP_FIELD(ContentInfo);
    DUMP_FIELD(PRefType);
    DUMP_FIELD(FadeDetection);
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    DUMP_FIELD(DeblockingAlphaTcOffset);
    DUMP_FIELD(DeblockingBetaOffset);
#else
    DUMP_FIELD_RESERVED(reserved2);
#endif
    DUMP_FIELD(GPB);
    DUMP_FIELD(MaxFrameSizeI);
    DUMP_FIELD(MaxFrameSizeP);
    DUMP_FIELD(EnableQPOffset);
    DUMP_FIELD_RESERVED(QPOffset);
    DUMP_FIELD_RESERVED(NumRefActiveP);
    DUMP_FIELD_RESERVED(NumRefActiveBL0);
    DUMP_FIELD_RESERVED(NumRefActiveBL1);
    DUMP_FIELD(BRCPanicMode);
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    DUMP_FIELD(ConstrainedIntraPredFlag);
#endif
#if (MFX_VERSION >= 1026)
    DUMP_FIELD(TransformSkip);
#endif
#if (MFX_VERSION >= 1027)
    DUMP_FIELD(TargetChromaFormatPlus1);   /* Minus 1 specifies target encoding chroma format (see ColorFormat enum). May differ from input one. */
    DUMP_FIELD(TargetBitDepthLuma);        /* Target encoding bit depth for luma samples. May differ from input one. */
    DUMP_FIELD(TargetBitDepthChroma);      /* Target encoding bit depth for chroma samples. May differ from input one. */
#else
    DUMP_FIELD_RESERVED(reserved4);
#endif
    DUMP_FIELD(EnableMBForceIntra);
    DUMP_FIELD(AdaptiveMaxFrameSize);
    DUMP_FIELD(RepartitionCheckEnable);
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    DUMP_FIELD(QuantScaleType);
    DUMP_FIELD(IntraVLCFormat);
    DUMP_FIELD(ScanType);
#endif
#if ((MFX_VERSION < MFX_VERSION_NEXT) && (MFX_VERSION >= 1025))
    DUMP_FIELD_RESERVED(reserved5);
#endif
#if (MFX_VERSION >= 1025)
    DUMP_FIELD(EncodedUnitsInfo);
    DUMP_FIELD(EnableNalUnitType);
#endif
#if (MFX_VERSION >= 1026)
    DUMP_FIELD(ExtBrcAdaptiveLTR)
#endif
    DUMP_FIELD_RESERVED(reserved);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtEncoderResetOption &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(StartNewSequence);
    DUMP_FIELD_RESERVED(reserved);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVPPDoNotUse &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    // TODO dump list of algs
    DUMP_FIELD(NumAlg);
    DUMP_FIELD(AlgList);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVppAuxData &extVppAuxData)
{
    std::string str;
    str += dump(structName + ".Header", extVppAuxData.Header) + "\n";
    str += structName + ".SpatialComplexity=" + ToString(extVppAuxData.SpatialComplexity) + "\n";
    str += structName + ".TemporalComplexity=" + ToString(extVppAuxData.TemporalComplexity) + "\n";
    str += structName + ".PicStruct=" + ToString(extVppAuxData.PicStruct) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(extVppAuxData.reserved) + "\n";
    str += structName + ".SceneChangeRate=" + ToString(extVppAuxData.SceneChangeRate) + "\n";
    str += structName + ".RepeatedFrame=" + ToString(extVppAuxData.RepeatedFrame);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxFrameAllocRequest &frameAllocRequest)
{
    std::string str;
    str += structName + ".AllocId=" + ToString(frameAllocRequest.AllocId) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(frameAllocRequest.reserved) + "\n";
    str += structName + ".reserved3[]=" + DUMP_RESERVED_ARRAY(frameAllocRequest.reserved3) + "\n";
    str += dump(structName+".Info", frameAllocRequest.Info) + "\n";
    str += structName + ".Type=" + ToString(frameAllocRequest.Type) + "\n";
    str += structName + ".NumFrameMin=" + ToString(frameAllocRequest.NumFrameMin) + "\n";
    str += structName + ".NumFrameSuggested=" + ToString(frameAllocRequest.NumFrameSuggested) + "\n";
    str += structName + ".reserved2=" + ToString(frameAllocRequest.reserved2);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxFrameAllocResponse &frameAllocResponse)
{
    std::string str;
    str += structName + ".AllocId=" + ToString(frameAllocResponse.AllocId) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(frameAllocResponse.reserved) + "\n";
    if (frameAllocResponse.mids)
        str += structName + ".mids=" + ToString(frameAllocResponse.mids) + "\n";
    else
        str += structName + ".mids=" + "NULL" + "\n";
    str += structName + ".NumFrameActual=" + ToString(frameAllocResponse.NumFrameActual) + "\n";
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    str += structName + ".MemType=" + ToString(frameAllocResponse.MemType) + "\n";
#else
    str += structName + ".reserved2=" + ToString(frameAllocResponse.reserved2) + "\n";
#endif
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxFrameData &frameData)
{
    std::string str;
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(frameData.reserved) + "\n";
    str += structName + ".PitchHigh=" + ToString(frameData.PitchHigh) + "\n";
    str += structName + ".TimeStamp=" + ToString(frameData.TimeStamp) + "\n";
    str += structName + ".FrameOrder=" + ToString(frameData.FrameOrder) + "\n";
    str += structName + ".Locked=" + ToString(frameData.Locked) + "\n";
    str += structName + ".Pitch=" + ToString(frameData.Pitch) + "\n";
    str += structName + ".PitchLow=" + ToString(frameData.PitchLow) + "\n";
    if (Log::GetLogLevel() == LOG_LEVEL_FULL) {
        str += structName + ".Y=" + ToHexFormatString(frameData.Y) + "\n";
        str += structName + ".R=" + ToHexFormatString(frameData.R) + "\n";
        str += structName + ".UV=" + ToHexFormatString(frameData.UV) + "\n";
        str += structName + ".VU=" + ToHexFormatString(frameData.VU) + "\n";
        str += structName + ".CbCr=" + ToHexFormatString(frameData.CbCr) + "\n";
        str += structName + ".CrCb=" + ToHexFormatString(frameData.CrCb) + "\n";
        str += structName + ".Cb=" + ToHexFormatString(frameData.Cb) + "\n";
        str += structName + ".U=" + ToHexFormatString(frameData.U) + "\n";
        str += structName + ".G=" + ToHexFormatString(frameData.G) + "\n";
#if (MFX_VERSION >= 1027)
        str += structName + ".Y410=" + ToHexFormatString(frameData.Y410) + "\n";
#endif
        str += structName + ".Cr=" + ToHexFormatString(frameData.Cr) + "\n";
        str += structName + ".V=" + ToHexFormatString(frameData.V) + "\n";
        str += structName + ".B=" + ToHexFormatString(frameData.B) + "\n";
        str += structName + ".A=" + ToHexFormatString(frameData.A) + "\n";
#if (MFX_VERSION >= 1025)
        str += structName + ".A2RGB10=" + ToString(frameData.A2RGB10) + "\n";
#endif
    }
    str += structName + ".MemId=" + ToString(frameData.MemId) + "\n";
    str += structName + ".Corrupted=" + ToString(frameData.Corrupted) + "\n";
    str += structName + ".DataFlag=" + ToString(frameData.DataFlag) + "\n";
    str += dump_mfxExtParams(structName, frameData);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxFrameId &frame)
{
    std::string str;
    str += structName + ".TemporalId=" + ToString(frame.TemporalId) + "\n";
    str += structName + ".PriorityId=" + ToString(frame.PriorityId) + "\n";
    str += structName + ".DependencyId=" + ToString(frame.DependencyId) + "\n";
    str += structName + ".QualityId=" + ToString(frame.QualityId) + "\n";
    str += structName + ".ViewId=" + ToString(frame.ViewId);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxFrameInfo &info)
{
    std::string str;
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(info.reserved) + "\n";
    str += structName + ".reserved4=" + ToString(info.reserved4) + "\n";
    str += structName + ".BitDepthLuma=" + ToString(info.BitDepthLuma) + "\n";
    str += structName + ".BitDepthChroma=" + ToString(info.BitDepthChroma) + "\n";
    str += structName + ".Shift=" + ToString(info.Shift) + "\n";
    str += dump(structName + ".mfxFrameId", info.FrameId) + "\n";
    str += structName + ".FourCC=" + GetFourCC(info.FourCC) + "\n";
    str += structName + ".Width=" + ToString(info.Width) + "\n";
    str += structName + ".Height=" + ToString(info.Height) + "\n";
    str += structName + ".CropX=" + ToString(info.CropX) + "\n";
    str += structName + ".CropY=" + ToString(info.CropY) + "\n";
    str += structName + ".CropW=" + ToString(info.CropW) + "\n";
    str += structName + ".CropH=" + ToString(info.CropH) + "\n";
    str += structName + ".BufferSize=" + ToString(info.BufferSize) + "\n";
    str += structName + ".reserved5=" + ToString(info.reserved5) + "\n";
    str += structName + ".FrameRateExtN=" + ToString(info.FrameRateExtN) + "\n";
    str += structName + ".FrameRateExtD=" + ToString(info.FrameRateExtD) + "\n";
    str += structName + ".reserved3=" + ToString(info.reserved3) + "\n";
    str += structName + ".AspectRatioW=" + ToString(info.AspectRatioW) + "\n";
    str += structName + ".AspectRatioH=" + ToString(info.AspectRatioH) + "\n";
    str += structName + ".PicStruct=" + ToString(info.PicStruct) + "\n";
    str += structName + ".ChromaFormat=" + ToString(info.ChromaFormat) + "\n";
    str += structName + ".reserved2=" + ToString(info.reserved2);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxFrameSurface1 &frameSurface1)
{
    std::string str;
    str += dump(structName + ".Data", frameSurface1.Data) + "\n";
    str += dump(structName + ".Info", frameSurface1.Info) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(frameSurface1.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxHandleType &handleType)
{
    return std::string("mfxHandleType " + structName + "=" + ToString(handleType));
}

std::string DumpContext::dump(const std::string structName, const mfxInfoMFX &mfx)
{
    std::string str;
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(mfx.reserved) + "\n";
    str += structName + ".LowPower=" + ToString(mfx.LowPower) + "\n";
    str += structName + ".BRCParamMultiplier=" + ToString(mfx.BRCParamMultiplier) + "\n";
    str += dump(structName + ".FrameInfo", mfx.FrameInfo) + "\n";
    str += structName + ".CodecId=" + GetCodecIdString(mfx.CodecId) + "\n";
    str += structName + ".CodecProfile=" + ToString(mfx.CodecProfile) + "\n";
    str += structName + ".CodecLevel=" + ToString(mfx.CodecLevel) + "\n";
    str += structName + ".NumThread=" + ToString(mfx.NumThread) + "\n";
    str += structName + ".TargetUsage=" + ToString(mfx.TargetUsage) + "\n";
    str += structName + ".GopPicSize=" + ToString(mfx.GopPicSize) + "\n";
    str += structName + ".GopRefDist=" + ToString(mfx.GopRefDist) + "\n";
    str += structName + ".GopOptFlag=" + ToString(mfx.GopOptFlag) + "\n";
    str += structName + ".IdrInterval=" + ToString(mfx.IdrInterval) + "\n";
    str += structName + ".RateControlMethod=" + ToString(mfx.RateControlMethod) + "\n";
    str += structName + ".InitialDelayInKB=" + ToString(mfx.InitialDelayInKB) + "\n";
    str += structName + ".QPI=" + ToString(mfx.QPI) + "\n";
    str += structName + ".Accuracy=" + ToString(mfx.Accuracy) + "\n";
    str += structName + ".BufferSizeInKB=" + ToString(mfx.BufferSizeInKB) + "\n";
    str += structName + ".TargetKbps=" + ToString(mfx.TargetKbps) + "\n";
    str += structName + ".QPP=" + ToString(mfx.QPP) + "\n";
    str += structName + ".ICQQuality=" + ToString(mfx.ICQQuality) + "\n";
    str += structName + ".MaxKbps=" + ToString(mfx.MaxKbps) + "\n";
    str += structName + ".QPB=" + ToString(mfx.QPB) + "\n";
    str += structName + ".Convergence=" + ToString(mfx.Convergence) + "\n";
    str += structName + ".NumSlice=" + ToString(mfx.NumSlice) + "\n";
    str += structName + ".NumRefFrame=" + ToString(mfx.NumRefFrame) + "\n";
    str += structName + ".EncodedOrder=" + ToString(mfx.EncodedOrder) + "\n";
    str += structName + ".DecodedOrder=" + ToString(mfx.DecodedOrder) + "\n";
    str += structName + ".ExtendedPicStruct=" + ToString(mfx.ExtendedPicStruct) + "\n";
    str += structName + ".TimeStampCalc=" + ToString(mfx.TimeStampCalc) + "\n";
    str += structName + ".SliceGroupsPresent=" + ToString(mfx.SliceGroupsPresent) + "\n";
    str += structName + ".MaxDecFrameBuffering=" + ToString(mfx.MaxDecFrameBuffering) + "\n";
    str += structName + ".EnableReallocRequest=" + ToString(mfx.EnableReallocRequest) + "\n";
#if (MFX_VERSION >= 1034)
    str += structName + ".IgnoreLevelConstrain=" + ToString(mfx.IgnoreLevelConstrain) + "\n";
#endif
    str += structName + ".reserved2[]=" + DUMP_RESERVED_ARRAY(mfx.reserved2) + "\n";
    str += structName + ".JPEGChromaFormat=" + ToString(mfx.JPEGChromaFormat) + "\n";
    str += structName + ".Rotation=" + ToString(mfx.Rotation) + "\n";
    str += structName + ".JPEGColorFormat=" + ToString(mfx.JPEGColorFormat) + "\n";
    str += structName + ".InterleavedDec=" + ToString(mfx.InterleavedDec) + "\n";
    str += structName + ".reserved3[]=" + DUMP_RESERVED_ARRAY(mfx.reserved3) + "\n";
    str += structName + ".Interleaved=" + ToString(mfx.Interleaved) + "\n";
    str += structName + ".Quality=" + ToString(mfx.Quality) + "\n";
    str += structName + ".RestartInterval=" + ToString(mfx.RestartInterval) + "\n";
    str += structName + ".reserved5[]=" + DUMP_RESERVED_ARRAY(mfx.reserved5) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxInfoVPP &vpp)
{
    std::string str;
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(vpp.reserved) + "\n";
    str += dump(structName + ".In", vpp.In) + "\n" +
    str += dump(structName + ".Out", vpp.Out);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxPayload &payload)
{
    std::string str;
    str += structName + ".CtrlFlags=" + ToString(payload.CtrlFlags) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(payload.reserved) + "\n";
    str += structName + ".Data=" + ToString(payload.Data) + "\n";
    str += structName + ".NumBit=" + ToString(payload.NumBit) + "\n";
    str += structName + ".Type=" + ToString(payload.Type) + "\n";
    str += structName + ".BufSize=" + ToString(payload.BufSize);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxSkipMode &skipMode)
{
    return std::string("mfxSkipMode " + structName + "=" + ToString(skipMode));
}

std::string DumpContext::dump(const std::string structName, const mfxVideoParam& videoParam)
{
    std::string str;
    str += structName + ".AllocId=" + ToString(videoParam.AllocId) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(videoParam.reserved) + "\n";
    str += structName + ".AsyncDepth=" + ToString(videoParam.AsyncDepth) + "\n";
    if(context == DUMPCONTEXT_MFX)
        str += dump(structName + ".mfx", videoParam.mfx) + "\n";
    else if(context == DUMPCONTEXT_VPP)
        str += dump(structName + ".vpp", videoParam.vpp) + "\n";
    else if(context == DUMPCONTEXT_ALL){
        str += dump(structName + ".mfx", videoParam.mfx) + "\n";
        str += dump(structName + ".vpp", videoParam.vpp) + "\n";
    }
    str += structName + ".Protected=" + ToString(videoParam.Protected) + "\n";
    str += structName + ".IOPattern=" + GetIOPattern(videoParam.IOPattern) + "\n";
    str += dump_mfxExtParams(structName, videoParam) + "\n";
    str += structName + ".reserved2=" + ToString(videoParam.reserved2);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxVPPStat &vppStat)
{
    std::string str;
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(vppStat.reserved) + "\n";
    str += structName + ".NumFrame=" + ToString(vppStat.NumFrame) + "\n";
    str += structName + ".NumCachedFrame=" + ToString(vppStat.NumCachedFrame);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtDecodedFrameInfo &ExtDecodedFrameInfo)
{
    std::string str;
    str += dump(structName + ".Header", ExtDecodedFrameInfo.Header) + "\n";
    str += structName + ".FrameType=" + ToString(ExtDecodedFrameInfo.FrameType) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtDecodedFrameInfo.reserved) + "\n";
    return str;
}

#if (MFX_VERSION >= 1025)
std::string DumpContext::dump(const std::string structName, const mfxExtDecodeErrorReport &ExtDecodeErrorReport)
{
    std::string str;
    str += dump(structName + ".Header", ExtDecodeErrorReport.Header) + "\n";
    str += structName + ".ErrorTypes=" + toString(ExtDecodeErrorReport.ErrorTypes, DUMP_HEX) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtDecodeErrorReport.reserved) + "\n";
    return str;
}
#endif

std::string DumpContext::dump(const std::string structName, const mfxExtTimeCode &ExtTimeCode)
{
    std::string str;
    str += dump(structName + ".Header", ExtTimeCode.Header) + "\n";
    str += structName + ".DropFrameFlag=" + ToString(ExtTimeCode.DropFrameFlag) + "\n";
    str += structName + ".TimeCodeHours=" + ToString(ExtTimeCode.TimeCodeHours) + "\n";
    str += structName + ".TimeCodeMinutes=" + ToString(ExtTimeCode.TimeCodeMinutes) + "\n";
    str += structName + ".TimeCodeSeconds=" + ToString(ExtTimeCode.TimeCodeSeconds) + "\n";
    str += structName + ".TimeCodePictures=" + ToString(ExtTimeCode.TimeCodePictures) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtTimeCode.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtEncoderCapability &ExtEncoderCapability)
{
    std::string str;
    str += dump(structName + ".Header", ExtEncoderCapability.Header) + "\n";
    str += structName + ".MBPerSec=" + ToString(ExtEncoderCapability.MBPerSec) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtEncoderCapability.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtDirtyRect &ExtDirtyRect)
{
    std::string str;
    str += dump(structName + ".Header", ExtDirtyRect.Header) + "\n";
    str += structName + ".NumRect=" + ToString(ExtDirtyRect.NumRect) + "\n";
    str += structName + ".reserved1[]=" + DUMP_RESERVED_ARRAY(ExtDirtyRect.reserved1) + "\n";
    str += structName + ".Rect=" + ToString(ExtDirtyRect.Rect) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtMoveRect &ExtMoveRect)
{
    std::string str;
    str += dump(structName + ".Header", ExtMoveRect.Header) + "\n";
    str += structName + ".NumRect=" + ToString(ExtMoveRect.NumRect) + "\n";
    str += structName + ".reserved1[]=" + DUMP_RESERVED_ARRAY(ExtMoveRect.reserved1) + "\n";
    str += structName + ".Rect=" + ToString(ExtMoveRect.Rect) + "\n";
    return str;
}

//camera
std::string DumpContext::dump(const std::string structName, const mfxExtCamGammaCorrection &CamGammaCorrection)
{
    std::string str;
    str += dump(structName + ".Header", CamGammaCorrection.Header) + "\n";
    str += structName + ".Mode=" + ToString(CamGammaCorrection.Mode) + "\n";
    str += structName + ".reserved1=" + ToString(CamGammaCorrection.reserved1) + "\n";
    str += structName + ".GammaValue=" + ToString(CamGammaCorrection.GammaValue) + "\n";
    str += structName + ".reserved2[]=" + DUMP_RESERVED_ARRAY(CamGammaCorrection.reserved2) + "\n";
    str += structName + ".NumPoints=" + ToString(CamGammaCorrection.NumPoints) + "\n";
    str += structName + ".GammaPoint=" + ToString(CamGammaCorrection.GammaPoint) + "\n";
    str += structName + ".GammaCorrected=" + ToString(CamGammaCorrection.GammaCorrected) + "\n";
    str += structName + ".reserved3[]=" + DUMP_RESERVED_ARRAY(CamGammaCorrection.reserved3) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCamWhiteBalance &CamWhiteBalance)
{
    std::string str;
    str += dump(structName + ".Header", CamWhiteBalance.Header) + "\n";
    str += structName + ".Mode=" + ToString(CamWhiteBalance.Mode) + "\n";
    str += structName + ".R=" + ToString(CamWhiteBalance.R) + "\n";
    str += structName + ".G0=" + ToString(CamWhiteBalance.G0) + "\n";
    str += structName + ".B=" + ToString(CamWhiteBalance.B) + "\n";
    str += structName + ".G1=" + ToString(CamWhiteBalance.G1) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(CamWhiteBalance.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCamHotPixelRemoval &CamHotPixelRemoval)
{
    std::string str;
    str += dump(structName + ".Header", CamHotPixelRemoval.Header) + "\n";
    str += structName + ".PixelThresholdDifference=" + ToString(CamHotPixelRemoval.PixelThresholdDifference) + "\n";
    str += structName + ".PixelCountThreshold=" + ToString(CamHotPixelRemoval.PixelCountThreshold) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCamBlackLevelCorrection &CamBlackLevelCorrection)
{
    std::string str;
    str += dump(structName + ".Header", CamBlackLevelCorrection.Header) + "\n";
    str += structName + ".R=" + ToString(CamBlackLevelCorrection.R) + "\n";
    str += structName + ".G0=" + ToString(CamBlackLevelCorrection.G0) + "\n";
    str += structName + ".B=" + ToString(CamBlackLevelCorrection.B) + "\n";
    str += structName + ".G1=" + ToString(CamBlackLevelCorrection.G1) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(CamBlackLevelCorrection.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxCamVignetteCorrectionParam &VignetteCorrectionParams)
{
    std::string str;
    //need to clarify SKL HW alignment with VPG
    str += structName + ".R=" + ToString(VignetteCorrectionParams.R.integer)+"."+ToString(VignetteCorrectionParams.R.mantissa) + "\n";
    str += structName + ".G0=" + ToString(VignetteCorrectionParams.G0.integer)+"."+ToString(VignetteCorrectionParams.G0.mantissa) + "\n";
    str += structName + ".B=" + ToString(VignetteCorrectionParams.G1.integer)+"."+ToString(VignetteCorrectionParams.G1.mantissa) + "\n";
    str += structName + ".G1=" + ToString(VignetteCorrectionParams.B.integer)+"."+ToString(VignetteCorrectionParams.B.mantissa) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCamVignetteCorrection &CamVignetteCorrection)
{
    std::string str;
    str += dump(structName + ".Header", CamVignetteCorrection.Header) + "\n";
    str += structName + ".Width=" + ToString(CamVignetteCorrection.Width) + "\n";
    str += structName + ".Height=" + ToString(CamVignetteCorrection.Height) + "\n";
    str += structName + ".Pitch=" + ToString(CamVignetteCorrection.Pitch) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(CamVignetteCorrection.reserved) + "\n";
    if (CamVignetteCorrection.CorrectionMap)
    {
        str += dump(structName + ".CorrectionMap", *(CamVignetteCorrection.CorrectionMap)) + "\n";
    }
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCamBayerDenoise &CamBayerDenoise)
{
    std::string str;
    str += dump(structName + ".Header", CamBayerDenoise.Header) + "\n";
    str += structName + ".Threshold=" + ToString(CamBayerDenoise.Threshold) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(CamBayerDenoise.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCamColorCorrection3x3 &CamColorCorrection3x3)
{
    std::string str;
    str += dump(structName + ".Header", CamColorCorrection3x3.Header) + "\n";
    str += structName + ".CCM=" + ToString(CamColorCorrection3x3.CCM) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(CamColorCorrection3x3.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCamPadding &CamPadding)
{
    std::string str;
    str += dump(structName + ".Header", CamPadding.Header) + "\n";
    str += structName + ".Top=" + ToString(CamPadding.Top) + "\n";
    str += structName + ".Bottom=" + ToString(CamPadding.Bottom) + "\n";
    str += structName + ".Left=" + ToString(CamPadding.Left) + "\n";
    str += structName + ".Right=" + ToString(CamPadding.Right) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(CamPadding.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCamPipeControl &CamPipeControl)
{
    std::string str;
    str += dump(structName + ".Header", CamPipeControl.Header) + "\n";
    str += structName + ".RawFormat=" + ToString(CamPipeControl.RawFormat) + "\n";
    str += structName + ".reserved1=" + ToString(CamPipeControl.reserved1) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(CamPipeControl.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxCamFwdGammaSegment &CamFwrGammaSegment)
{
    std::string str;
    str += structName + ".Pixel=" + ToString(CamFwrGammaSegment.Pixel) + "\n";
    str += structName + ".Red=" + ToString(CamFwrGammaSegment.Red) + "\n";
    str += structName + ".Green=" + ToString(CamFwrGammaSegment.Green) + "\n";
    str += structName + ".Blue=" + ToString(CamFwrGammaSegment.Blue) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCamFwdGamma &CamFwrGamma)
{
    std::string str;
    str += dump(structName + ".Header", CamFwrGamma.Header) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(CamFwrGamma.reserved) + "\n";
    str += structName + ".NumSegments=" + ToString(CamFwrGamma.NumSegments) + "\n";
    if (CamFwrGamma.Segment)
    {
        for (int i = 0; i <  CamFwrGamma.NumSegments; i++)
        {
            str += dump((structName + ".Segment[" + ToString(i) + "]"), CamFwrGamma.Segment[i]) + "\n";
        }
    }
    return str;
}


std::string DumpContext::dump(const std::string structName, const mfxExtCamLensGeomDistCorrection &CamLensGeomDistCorrection)
{
    std::string str;
    str += dump(structName + ".Header", CamLensGeomDistCorrection.Header) + "\n";
    for (int i = 0; i < 3; i++)
        str += structName + ".a[" + ToString(i) + "]=" + ToString(CamLensGeomDistCorrection.a[i]) + "\n";
    for (int i = 0; i < 3; i++)
        str += structName + ".b[" + ToString(i) + "]=" + ToString(CamLensGeomDistCorrection.b[i]) + "\n";
    for (int i = 0; i < 3; i++)
        str += structName + ".c[" + ToString(i) + "]=" + ToString(CamLensGeomDistCorrection.c[i]) + "\n";
    for (int i = 0; i < 3; i++)
        str += structName + ".d[" + ToString(i) + "]=" + ToString(CamLensGeomDistCorrection.d[i]) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(CamLensGeomDistCorrection.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCamTotalColorControl &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(R);
    DUMP_FIELD(G);
    DUMP_FIELD(B);

    DUMP_FIELD(C);
    DUMP_FIELD(M);
    DUMP_FIELD(Y);

    DUMP_FIELD_RESERVED(reserved);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCamCscYuvRgb &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    str += structName + ".PreOffset=" + ToString(_struct.PreOffset) + "\n";
    str += structName + ".Matrix=" + ToString(_struct.Matrix) + "\n";
    str += structName + ".PreOffset=" + ToString(_struct.PostOffset) + "\n";

    DUMP_FIELD_RESERVED(reserved);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtAVCRefListCtrl &ExtAVCRefListCtrl)
{
    std::string str;
    str += dump(structName + ".Header", ExtAVCRefListCtrl.Header) + "\n";
    str += structName + ".NumRefIdxL0Active=" + ToString(ExtAVCRefListCtrl.NumRefIdxL0Active) + "\n";
    str += structName + ".NumRefIdxL1Active=" + ToString(ExtAVCRefListCtrl.NumRefIdxL1Active) + "\n";
    str += structName + ".PreferredRefList=" + ToString(ExtAVCRefListCtrl.PreferredRefList) + "\n";
    str += structName + ".RejectedRefList=" + ToString(ExtAVCRefListCtrl.RejectedRefList) + "\n";
    str += structName + ".LongTermRefList=" + ToString(ExtAVCRefListCtrl.LongTermRefList) + "\n";
    str += structName + ".ApplyLongTermIdx=" + ToString(ExtAVCRefListCtrl.ApplyLongTermIdx) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtAVCRefListCtrl.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtAvcTemporalLayers &ExtAvcTemporalLayers)
{
    std::string str;
    str += dump(structName + ".Header", ExtAvcTemporalLayers.Header) + "\n";
    str += structName + ".reserved1[]=" + DUMP_RESERVED_ARRAY(ExtAvcTemporalLayers.reserved1) + "\n";
    str += structName + ".reserved2=" + ToString(ExtAvcTemporalLayers.reserved2) + "\n";
    str += structName + ".BaseLayerPID=" + ToString(ExtAvcTemporalLayers.BaseLayerPID) + "\n";
    str += structName + ".Layer=" + ToString(ExtAvcTemporalLayers.Layer) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtAVCEncodedFrameInfo &ExtAVCEncodedFrameInfo)
{
    std::string str;
    str += dump(structName + ".Header", ExtAVCEncodedFrameInfo.Header) + "\n";
    str += structName + ".FrameOrder=" + ToString(ExtAVCEncodedFrameInfo.FrameOrder) + "\n";
    str += structName + ".PicStruct=" + ToString(ExtAVCEncodedFrameInfo.PicStruct) + "\n";
    str += structName + ".LongTermIdx=" + ToString(ExtAVCEncodedFrameInfo.LongTermIdx) + "\n";
    str += structName + ".MAD=" + ToString(ExtAVCEncodedFrameInfo.MAD) + "\n";
    str += structName + ".BRCPanicMode=" + ToString(ExtAVCEncodedFrameInfo.BRCPanicMode) + "\n";
    str += structName + ".QP=" + ToString(ExtAVCEncodedFrameInfo.QP) + "\n";
    str += structName + ".SecondFieldOffset=" + ToString(ExtAVCEncodedFrameInfo.SecondFieldOffset) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtAVCEncodedFrameInfo.reserved) + "\n";
    str += structName + ".UsedRefListL0=" + ToHexFormatString(ExtAVCEncodedFrameInfo.UsedRefListL0) + "\n";
    str += structName + ".UsedRefListL1=" + ToHexFormatString(ExtAVCEncodedFrameInfo.UsedRefListL1) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtAVCRefLists &ExtAVCRefLists)
{
    std::string str;
    str += dump(structName + ".Header", ExtAVCRefLists.Header) + "\n";
    str += structName + ".NumRefIdxL0Active=" + ToString(ExtAVCRefLists.NumRefIdxL0Active) + "\n";
    str += structName + ".NumRefIdxL1Active=" + ToString(ExtAVCRefLists.NumRefIdxL1Active) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtAVCRefLists.reserved) + "\n";
    str += structName + ".RefPicList0=" + ToString(ExtAVCRefLists.RefPicList0) + "\n";
    str += structName + ".RefPicList1=" + ToString(ExtAVCRefLists.RefPicList1) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVPPMirroring &ExtVPPMirroring)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPMirroring.Header) + "\n";
    str += structName + ".Type=" + ToString(ExtVPPMirroring.Type) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtVPPMirroring.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtMVOverPicBoundaries &ExtMVOverPicBoundaries)
{
    std::string str;
    str += dump(structName + ".Header", ExtMVOverPicBoundaries.Header) + "\n";
    str += structName + ".StickTop=" + ToString(ExtMVOverPicBoundaries.StickTop) + "\n";
    str += structName + ".StickBottom=" + ToString(ExtMVOverPicBoundaries.StickBottom) + "\n";
    str += structName + ".StickLeft=" + ToString(ExtMVOverPicBoundaries.StickLeft) + "\n";
    str += structName + ".StickRight=" + ToString(ExtMVOverPicBoundaries.StickRight) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtMVOverPicBoundaries.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVPPColorFill &ExtVPPColorFill)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPColorFill.Header) + "\n";
    str += structName + ".Enable=" + ToString(ExtVPPColorFill.Enable) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtVPPColorFill.reserved) + "\n";
    return str;
}

//mfxjpeg
std::string DumpContext::dump(const std::string structName, const mfxExtJPEGQuantTables &ExtJPEGQuantTables)
{
    std::string str;
    str += dump(structName + ".Header", ExtJPEGQuantTables.Header) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtJPEGQuantTables.reserved) + "\n";
    str += structName + ".NumTable=" + ToString(ExtJPEGQuantTables.NumTable) + "\n";
    for (int i = 0; i < 4; i++)
    {
        str += structName + ".Qm[" + ToString(i) + "][]=" + DUMP_RESERVED_ARRAY(ExtJPEGQuantTables.Qm[i]) + "\n";
    }
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtJPEGHuffmanTables &ExtJPEGHuffmanTables)
{
    std::string str;
    str += dump(structName + ".Header", ExtJPEGHuffmanTables.Header) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtJPEGHuffmanTables.reserved) + "\n";
    str += structName + ".NumDCTable=" + ToString(ExtJPEGHuffmanTables.NumDCTable) + "\n";
    str += structName + ".NumACTable=" + ToString(ExtJPEGHuffmanTables.NumACTable) + "\n";
    for (int i = 0; i < 4; i++)
    {
        str += structName + ".DCTables[" + ToString(i) + "].Bits[]=" + DUMP_RESERVED_ARRAY(ExtJPEGHuffmanTables.DCTables[i].Bits) + "\n";
        str += structName + ".DCTables[" + ToString(i) + "].Values[]=" + DUMP_RESERVED_ARRAY(ExtJPEGHuffmanTables.DCTables[i].Values) + "\n";
    }
    for (int i = 0; i < 4; i++)
    {
        str += structName + ".ACTables[" + ToString(i) + "].Bits[]=" + DUMP_RESERVED_ARRAY(ExtJPEGHuffmanTables.ACTables[i].Bits) + "\n";
        str += structName + ".ACTables[" + ToString(i) + "].Values[]=" + DUMP_RESERVED_ARRAY(ExtJPEGHuffmanTables.ACTables[i].Values) + "\n";
    }
    return str;
}

//mfxmvc
std::string DumpContext::dump(const std::string structName, const mfxMVCViewDependency &MVCViewDependency)
{
    std::string str;
    str += structName + ".ViewId=" + ToString(MVCViewDependency.ViewId) + "\n";
    str += structName + ".NumAnchorRefsL0=" + ToString(MVCViewDependency.NumAnchorRefsL0) + "\n";
    str += structName + ".NumAnchorRefsL1=" + ToString(MVCViewDependency.NumAnchorRefsL1) + "\n";
    str += structName + ".AnchorRefL0[]=" + DUMP_RESERVED_ARRAY(MVCViewDependency.AnchorRefL0) + "\n";
    str += structName + ".AnchorRefL1[]=" + DUMP_RESERVED_ARRAY(MVCViewDependency.AnchorRefL1) + "\n";
    str += structName + ".NumNonAnchorRefsL0=" + ToString(MVCViewDependency.NumNonAnchorRefsL0) + "\n";
    str += structName + ".NumNonAnchorRefsL1=" + ToString(MVCViewDependency.NumNonAnchorRefsL1) + "\n";
    str += structName + ".NonAnchorRefL0[]=" + DUMP_RESERVED_ARRAY(MVCViewDependency.NonAnchorRefL0) + "\n";
    str += structName + ".NonAnchorRefL1[]=" + DUMP_RESERVED_ARRAY(MVCViewDependency.NonAnchorRefL1) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxMVCOperationPoint &MVCOperationPoint)
{
    std::string str;
    str += structName + ".TemporalId=" + ToString(MVCOperationPoint.TemporalId) + "\n";
    str += structName + ".LevelIdc=" + ToString(MVCOperationPoint.LevelIdc) + "\n";
    str += structName + ".NumViews=" + ToString(MVCOperationPoint.NumViews) + "\n";
    str += structName + ".NumTargetViews=" + ToString(MVCOperationPoint.NumTargetViews) + "\n";
    str += structName + ".TargetViewId=" + ToHexFormatString(MVCOperationPoint.TargetViewId) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtMVCSeqDesc &ExtMVCSeqDesc)
{
    std::string str;
    str += dump(structName + ".Header", ExtMVCSeqDesc.Header) + "\n";
    str += structName + ".NumView=" + ToString(ExtMVCSeqDesc.NumView) + "\n";
    if (ExtMVCSeqDesc.View)
    {
        str += dump(structName + ".View", *(ExtMVCSeqDesc.View)) + "\n";
    }
    str += structName + ".NumViewId=" + ToString(ExtMVCSeqDesc.NumViewId) + "\n";
    str += structName + ".NumViewIdAlloc=" + ToString(ExtMVCSeqDesc.NumViewIdAlloc) + "\n";
    str += structName + ".ViewId=" + ToString(ExtMVCSeqDesc.ViewId) + "\n";
    str += structName + ".NumOP=" + ToString(ExtMVCSeqDesc.NumOP) + "\n";
    str += structName + ".NumOPAlloc=" + ToString(ExtMVCSeqDesc.NumOPAlloc) + "\n";
    if (ExtMVCSeqDesc.OP)
    {
        str += dump(structName + ".OP", *(ExtMVCSeqDesc.OP)) + "\n";
    }
    str += structName + ".NumRefsTotal=" + ToString(ExtMVCSeqDesc.NumRefsTotal) + "\n";
    str += structName + ".Reserved[]=" + DUMP_RESERVED_ARRAY(ExtMVCSeqDesc.Reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtMVCTargetViews &ExtMVCTargetViews)
{
    std::string str;
    str += dump(structName + ".Header", ExtMVCTargetViews.Header) + "\n";
    str += structName + ".TemporalId=" + ToString(ExtMVCTargetViews.TemporalId) + "\n";
    str += structName + ".NumView=" + ToString(ExtMVCTargetViews.NumView) + "\n";
    str += structName + ".ViewId[]=" + DUMP_RESERVED_ARRAY(ExtMVCTargetViews.ViewId) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtOpaqueSurfaceAlloc &ExtOpaqueSurfaceAlloc)
{
    std::string str;
    str += dump(structName + ".Header", ExtOpaqueSurfaceAlloc.Header) + "\n";
    str += structName + ".reserved1[]=" + DUMP_RESERVED_ARRAY(ExtOpaqueSurfaceAlloc.reserved1) + "\n";
    str += structName + ".In.NumSurface=" + ToString(ExtOpaqueSurfaceAlloc.In.NumSurface) + "\n";
    str += structName + ".In.Surfaces=" + ToHexFormatString(ExtOpaqueSurfaceAlloc.In.Surfaces) + "\n";
    str += structName + ".In.Type=" + ToString(ExtOpaqueSurfaceAlloc.In.Type) + "\n";
    str += structName + ".In.reserved2[]=" + DUMP_RESERVED_ARRAY(ExtOpaqueSurfaceAlloc.In.reserved2) + "\n";
    str += structName + ".Out.NumSurface=" + ToString(ExtOpaqueSurfaceAlloc.Out.NumSurface) + "\n";
    str += structName + ".Out.Surfaces=" + ToHexFormatString(ExtOpaqueSurfaceAlloc.Out.Surfaces) + "\n";
    str += structName + ".Out.Type=" + ToString(ExtOpaqueSurfaceAlloc.Out.Type) + "\n";
    str += structName + ".Out.reserved2[]=" + DUMP_RESERVED_ARRAY(ExtOpaqueSurfaceAlloc.Out.reserved2) + "\n";
    return str;
}

//mfxvpp
std::string DumpContext::dump(const std::string structName, const mfxExtVPPDenoise &ExtVPPDenoise)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPDenoise.Header) + "\n";
    str += structName + ".DenoiseFactor=" + ToString(ExtVPPDenoise.DenoiseFactor) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVPPDetail &ExtVPPDetail)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPDetail.Header) + "\n";
    str += structName + ".DetailFactor=" + ToString(ExtVPPDetail.DetailFactor) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVPPProcAmp &ExtVPPProcAmp)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPProcAmp.Header) + "\n";
    str += structName + ".Brightness=" + ToString(ExtVPPProcAmp.Brightness) + "\n";
    str += structName + ".Contrast=" + ToString(ExtVPPProcAmp.Contrast) + "\n";
    str += structName + ".Hue=" + ToString(ExtVPPProcAmp.Hue) + "\n";
    str += structName + ".Saturation=" + ToString(ExtVPPProcAmp.Saturation) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCodingOptionSPSPPS &ExtCodingOptionSPSPPS)
{
    std::string str;
    str += dump(structName + ".Header", ExtCodingOptionSPSPPS.Header) + "\n";
    str += structName + ".SPSBuffer=" + ToHexFormatString(ExtCodingOptionSPSPPS.SPSBuffer) + "\n";
    str += structName + ".PPSBuffer=" + ToHexFormatString(ExtCodingOptionSPSPPS.PPSBuffer) + "\n";
    str += structName + ".SPSBufSize=" + ToString(ExtCodingOptionSPSPPS.SPSBufSize) + "\n";
    str += structName + ".PPSBufSize=" + ToString(ExtCodingOptionSPSPPS.PPSBufSize) + "\n";
    str += structName + ".SPSId=" + ToString(ExtCodingOptionSPSPPS.SPSId) + "\n";
    str += structName + ".PPSId=" + ToString(ExtCodingOptionSPSPPS.PPSId) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtCodingOptionVPS &ExtCodingOptionVPS)
{
    std::string str;
    str += dump(structName + ".Header", ExtCodingOptionVPS.Header) + "\n";
    str += structName + ".VPSBuffer=" + ToHexFormatString(ExtCodingOptionVPS.VPSBuffer) + "\n";
    str += structName + ".reserved1=" + ToString(ExtCodingOptionVPS.reserved1) + "\n";
    str += structName + ".VPSBufSize=" + ToString(ExtCodingOptionVPS.VPSBufSize) + "\n";
    str += structName + ".VPSId=" + ToString(ExtCodingOptionVPS.VPSId) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtCodingOptionVPS.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVideoSignalInfo &ExtVideoSignalInfo)
{
    std::string str;
    str += dump(structName + ".Header", ExtVideoSignalInfo.Header) + "\n";
    str += structName + ".VideoFormat=" + ToString(ExtVideoSignalInfo.VideoFormat) + "\n";
    str += structName + ".VideoFullRange=" + ToString(ExtVideoSignalInfo.VideoFullRange) + "\n";
    str += structName + ".ColourDescriptionPresent=" + ToString(ExtVideoSignalInfo.ColourDescriptionPresent) + "\n";
    str += structName + ".ColourPrimaries=" + ToString(ExtVideoSignalInfo.ColourPrimaries) + "\n";
    str += structName + ".TransferCharacteristics=" + ToString(ExtVideoSignalInfo.TransferCharacteristics) + "\n";
    str += structName + ".MatrixCoefficients=" + ToString(ExtVideoSignalInfo.MatrixCoefficients) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVPPDoUse &ExtVPPDoUse)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPDoUse.Header) + "\n";
    str += structName + ".NumAlg=" + ToString(ExtVPPDoUse.NumAlg) + "\n";
    str += structName + ".AlgList=" + ToHexFormatString(ExtVPPDoUse.AlgList) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxVPPCompInputStream &VPPCompInputStream)
{
    std::string str;
    str += structName + ".DstX=" + ToString(VPPCompInputStream.DstX) + "\n";
    str += structName + ".DstY=" + ToString(VPPCompInputStream.DstY) + "\n";
    str += structName + ".DstW=" + ToString(VPPCompInputStream.DstW) + "\n";
    str += structName + ".DstH=" + ToString(VPPCompInputStream.DstH) + "\n";
    str += structName + ".LumaKeyEnable=" + ToString(VPPCompInputStream.LumaKeyEnable) + "\n";
    str += structName + ".LumaKeyMin=" + ToString(VPPCompInputStream.LumaKeyMin) + "\n";
    str += structName + ".LumaKeyMax=" + ToString(VPPCompInputStream.LumaKeyMax) + "\n";
    str += structName + ".GlobalAlphaEnable=" + ToString(VPPCompInputStream.GlobalAlphaEnable) + "\n";
    str += structName + ".GlobalAlphae=" + ToString(VPPCompInputStream.GlobalAlpha) + "\n";
    str += structName + ".PixelAlphaEnable=" + ToString(VPPCompInputStream.PixelAlphaEnable) + "\n";
    str += structName + ".TileId=" + ToString(VPPCompInputStream.TileId) + "\n";
    str += structName + ".reserved2[]=" + DUMP_RESERVED_ARRAY(VPPCompInputStream.reserved2) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVPPComposite &ExtVPPComposite)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPComposite.Header) + "\n";
    str += structName + ".Y=" + ToString(ExtVPPComposite.Y) + "\n";
    str += structName + ".R=" + ToString(ExtVPPComposite.R) + "\n";
    str += structName + ".U=" + ToString(ExtVPPComposite.U) + "\n";
    str += structName + ".G=" + ToString(ExtVPPComposite.G) + "\n";
    str += structName + ".V=" + ToString(ExtVPPComposite.V) + "\n";
    str += structName + ".B=" + ToString(ExtVPPComposite.B) + "\n";
    str += structName + ".NumTiles=" + ToString(ExtVPPComposite.NumTiles) + "\n";
    str += structName + ".reserved1[]=" + DUMP_RESERVED_ARRAY(ExtVPPComposite.reserved1) + "\n";
    str += structName + ".NumInputStream=" + ToString(ExtVPPComposite.NumInputStream) + "\n";
    str += structName + ".InputStream=" + ToHexFormatString(ExtVPPComposite.InputStream) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVPPVideoSignalInfo &ExtVPPVideoSignalInfo)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPVideoSignalInfo.Header) + "\n";
    str += structName + ".reserved1[]=" + DUMP_RESERVED_ARRAY(ExtVPPVideoSignalInfo.reserved1) + "\n";
    str += structName + ".In.TransferMatrix=" + ToString(ExtVPPVideoSignalInfo.In.TransferMatrix) + "\n";
    str += structName + ".In.NominalRange=" + ToString(ExtVPPVideoSignalInfo.In.NominalRange) + "\n";
    str += structName + ".In.reserved2[]=" + DUMP_RESERVED_ARRAY(ExtVPPVideoSignalInfo.In.reserved2) + "\n";
    str += structName + ".Out.TransferMatrix=" + ToString(ExtVPPVideoSignalInfo.Out.TransferMatrix) + "\n";
    str += structName + ".Out.NominalRange=" + ToString(ExtVPPVideoSignalInfo.Out.NominalRange) + "\n";
    str += structName + ".Out.reserved2[]=" + DUMP_RESERVED_ARRAY(ExtVPPVideoSignalInfo.Out.reserved2) + "\n";
    str += structName + ".TransferMatrix=" + ToString(ExtVPPVideoSignalInfo.TransferMatrix) + "\n";
    str += structName + ".NominalRange=" + ToString(ExtVPPVideoSignalInfo.NominalRange) + "\n";
    str += structName + ".reserved3[]=" + DUMP_RESERVED_ARRAY(ExtVPPVideoSignalInfo.reserved3) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVPPDeinterlacing &ExtVPPDeinterlacing)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPDeinterlacing.Header) + "\n";
    str += structName + ".Mode=" + ToString(ExtVPPDeinterlacing.Mode) + "\n";
    str += structName + ".TelecinePattern=" + ToString(ExtVPPDeinterlacing.TelecinePattern) + "\n";
    str += structName + ".TelecineLocation=" + ToString(ExtVPPDeinterlacing.TelecineLocation) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtVPPDeinterlacing.reserved) + "\n";
    return str;
}

//mfxSEI
std::string DumpContext::dump(const std::string structName, const mfxExtPictureTimingSEI &ExtPictureTimingSEI)
{
    std::string str;
    str += dump(structName + ".Header", ExtPictureTimingSEI.Header) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtPictureTimingSEI.reserved) + "\n";
    str += structName + ".TimeStamp[0].ClockTimestampFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[0].ClockTimestampFlag) + "\n";
    str += structName + ".TimeStamp[0].CtType=" + ToString(ExtPictureTimingSEI.TimeStamp[0].CtType) + "\n";
    str += structName + ".TimeStamp[0].NuitFieldBasedFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[0].NuitFieldBasedFlag) + "\n";
    str += structName + ".TimeStamp[0].CountingType=" + ToString(ExtPictureTimingSEI.TimeStamp[0].CountingType) + "\n";
    str += structName + ".TimeStamp[0].FullTimestampFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[0].FullTimestampFlag) + "\n";
    str += structName + ".TimeStamp[0].DiscontinuityFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[0].DiscontinuityFlag) + "\n";
    str += structName + ".TimeStamp[0].CntDroppedFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[0].CntDroppedFlag) + "\n";
    str += structName + ".TimeStamp[0].NFrames=" + ToString(ExtPictureTimingSEI.TimeStamp[0].NFrames) + "\n";
    str += structName + ".TimeStamp[0].SecondsFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[0].SecondsFlag) + "\n";
    str += structName + ".TimeStamp[0].MinutesFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[0].MinutesFlag) + "\n";
    str += structName + ".TimeStamp[0].HoursFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[0].HoursFlag) + "\n";
    str += structName + ".TimeStamp[0].SecondsValue=" + ToString(ExtPictureTimingSEI.TimeStamp[0].SecondsValue) + "\n";
    str += structName + ".TimeStamp[0].MinutesValue=" + ToString(ExtPictureTimingSEI.TimeStamp[0].MinutesValue) + "\n";
    str += structName + ".TimeStamp[0].HoursValue=" + ToString(ExtPictureTimingSEI.TimeStamp[0].HoursValue) + "\n";
    str += structName + ".TimeStamp[0].TimeOffset=" + ToString(ExtPictureTimingSEI.TimeStamp[0].TimeOffset) + "\n";
    str += structName + ".TimeStamp[1].ClockTimestampFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[1].ClockTimestampFlag) + "\n";
    str += structName + ".TimeStamp[1].CtType=" + ToString(ExtPictureTimingSEI.TimeStamp[1].CtType) + "\n";
    str += structName + ".TimeStamp[1].NuitFieldBasedFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[1].NuitFieldBasedFlag) + "\n";
    str += structName + ".TimeStamp[1].CountingType=" + ToString(ExtPictureTimingSEI.TimeStamp[1].CountingType) + "\n";
    str += structName + ".TimeStamp[1].FullTimestampFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[1].FullTimestampFlag) + "\n";
    str += structName + ".TimeStamp[1].DiscontinuityFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[1].DiscontinuityFlag) + "\n";
    str += structName + ".TimeStamp[1].CntDroppedFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[1].CntDroppedFlag) + "\n";
    str += structName + ".TimeStamp[1].NFrames=" + ToString(ExtPictureTimingSEI.TimeStamp[1].NFrames) + "\n";
    str += structName + ".TimeStamp[1].SecondsFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[1].SecondsFlag) + "\n";
    str += structName + ".TimeStamp[1].MinutesFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[1].MinutesFlag) + "\n";
    str += structName + ".TimeStamp[1].HoursFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[1].HoursFlag) + "\n";
    str += structName + ".TimeStamp[1].SecondsValue=" + ToString(ExtPictureTimingSEI.TimeStamp[1].SecondsValue) + "\n";
    str += structName + ".TimeStamp[1].MinutesValue=" + ToString(ExtPictureTimingSEI.TimeStamp[1].MinutesValue) + "\n";
    str += structName + ".TimeStamp[1].HoursValue=" + ToString(ExtPictureTimingSEI.TimeStamp[1].HoursValue) + "\n";
    str += structName + ".TimeStamp[1].TimeOffset=" + ToString(ExtPictureTimingSEI.TimeStamp[1].TimeOffset) + "\n";
    str += structName + ".TimeStamp[2].ClockTimestampFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[2].ClockTimestampFlag) + "\n";
    str += structName + ".TimeStamp[2].CtType=" + ToString(ExtPictureTimingSEI.TimeStamp[2].CtType) + "\n";
    str += structName + ".TimeStamp[2].NuitFieldBasedFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[2].NuitFieldBasedFlag) + "\n";
    str += structName + ".TimeStamp[2].CountingType=" + ToString(ExtPictureTimingSEI.TimeStamp[2].CountingType) + "\n";
    str += structName + ".TimeStamp[2].FullTimestampFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[2].FullTimestampFlag) + "\n";
    str += structName + ".TimeStamp[2].DiscontinuityFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[2].DiscontinuityFlag) + "\n";
    str += structName + ".TimeStamp[2].CntDroppedFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[2].CntDroppedFlag) + "\n";
    str += structName + ".TimeStamp[2].NFrames=" + ToString(ExtPictureTimingSEI.TimeStamp[2].NFrames) + "\n";
    str += structName + ".TimeStamp[2].SecondsFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[2].SecondsFlag) + "\n";
    str += structName + ".TimeStamp[2].MinutesFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[2].MinutesFlag) + "\n";
    str += structName + ".TimeStamp[2].HoursFlag=" + ToString(ExtPictureTimingSEI.TimeStamp[2].HoursFlag) + "\n";
    str += structName + ".TimeStamp[2].SecondsValue=" + ToString(ExtPictureTimingSEI.TimeStamp[2].SecondsValue) + "\n";
    str += structName + ".TimeStamp[2].MinutesValue=" + ToString(ExtPictureTimingSEI.TimeStamp[2].MinutesValue) + "\n";
    str += structName + ".TimeStamp[2].HoursValue=" + ToString(ExtPictureTimingSEI.TimeStamp[2].HoursValue) + "\n";
    str += structName + ".TimeStamp[2].TimeOffset=" + ToString(ExtPictureTimingSEI.TimeStamp[2].TimeOffset) + "\n";
    return str;
}

//mfxHEVC
std::string DumpContext::dump(const std::string structName, const mfxExtHEVCTiles &ExtHEVCTiles)
{
    std::string str;
    str += dump(structName + ".Header", ExtHEVCTiles.Header) + "\n";
    str += structName + ".NumTileRows=" + ToString(ExtHEVCTiles.NumTileRows) + "\n";
    str += structName + ".NumTileColumns=" + ToString(ExtHEVCTiles.NumTileColumns) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtHEVCTiles.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtHEVCParam &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";

    DUMP_FIELD(PicWidthInLumaSamples);
    DUMP_FIELD(PicHeightInLumaSamples);
    DUMP_FIELD(GeneralConstraintFlags);
#if (MFX_VERSION >= 1026)
    DUMP_FIELD(SampleAdaptiveOffset);
    DUMP_FIELD(LCUSize);
#endif

    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(_struct.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtHEVCRegion &ExtHEVCRegion)
{
    std::string str;
    str += dump(structName + ".Header", ExtHEVCRegion.Header) + "\n";
    str += structName + ".RegionId=" + ToString(ExtHEVCRegion.RegionId) + "\n";
    str += structName + ".RegionType=" + ToString(ExtHEVCRegion.RegionType) + "\n";
    str += structName + ".RegionEncoding=" + ToString(ExtHEVCRegion.RegionEncoding) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtHEVCRegion.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtPredWeightTable &ExtPredWeightTable)
{
    std::string str;
    str += dump(structName + ".Header", ExtPredWeightTable.Header) + "\n";
    str += structName + ".LumaLog2WeightDenom=" + ToString(ExtPredWeightTable.LumaLog2WeightDenom) + "\n";
    str += structName + ".ChromaLog2WeightDenom=" + ToString(ExtPredWeightTable.ChromaLog2WeightDenom) + "\n";
    for (int i = 0; i < 2; i++)
        str += structName + ".LumaWeightFlag[" + ToString(i) + "][]=" + DUMP_RESERVED_ARRAY(ExtPredWeightTable.LumaWeightFlag[i]) + "\n";
    for (int i = 0; i < 2; i++)
        str += structName + ".ChromaWeightFlag[" + ToString(i) + "][]=" + DUMP_RESERVED_ARRAY(ExtPredWeightTable.ChromaWeightFlag[i]) + "\n";
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 32; j++)
            for (int k = 0; k < 3; k++)
                str += structName + ".ChromaWeightFlag[" + ToString(i) + "][" + ToString(j) + "][" + ToString(k) + "][]=" + DUMP_RESERVED_ARRAY(ExtPredWeightTable.Weights[i][j][k]) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtPredWeightTable.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVPPFrameRateConversion &ExtVPPFrameRateConversion)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPFrameRateConversion.Header) + "\n";
    str += structName + ".Algorithm=" + ToString(ExtVPPFrameRateConversion.Algorithm) + "\n";
    str += structName + ".reserved=" + ToString(ExtVPPFrameRateConversion.reserved) + "\n";
    str += structName + ".reserved2[]=" + DUMP_RESERVED_ARRAY(ExtVPPFrameRateConversion.reserved2) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtVPPImageStab &ExtVPPImageStab)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPImageStab.Header) + "\n";
    str += structName + ".Mode=" + ToString(ExtVPPImageStab.Mode) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtVPPImageStab.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtEncoderROI &ExtEncoderROI)
{
    std::string str;
    str += dump(structName + ".Header", ExtEncoderROI.Header) + "\n";
    str += structName + ".NumROI=" + ToString(ExtEncoderROI.NumROI) + "\n";
    str += structName + ".ROIMode=" + ToString(ExtEncoderROI.ROIMode) + "\n";
    for (int i = 0; i < ExtEncoderROI.NumROI; i++) {
        str += structName + ".ROI[" + ToString(i) + "].Left=" + ToString(ExtEncoderROI.ROI[i].Left) + "\n";
        str += structName + ".ROI[" + ToString(i) + "].Top=" + ToString(ExtEncoderROI.ROI[i].Top) + "\n";
        str += structName + ".ROI[" + ToString(i) + "].Right=" + ToString(ExtEncoderROI.ROI[i].Right) + "\n";
        str += structName + ".ROI[" + ToString(i) + "].Bottom=" + ToString(ExtEncoderROI.ROI[i].Bottom) + "\n";
        if (ExtEncoderROI.ROIMode == MFX_ROI_MODE_QP_DELTA) {
            str += structName + ".ROI[" + ToString(i) + "].DeltaQP=" + ToString(ExtEncoderROI.ROI[i].DeltaQP) + "\n";
        } else {
            str += structName + ".ROI[" + ToString(i) + "].Priority=" + ToString(ExtEncoderROI.ROI[i].Priority) + "\n";
        }
        str += structName + ".ROI[" + ToString(i) + "].reserved2[]=" + DUMP_RESERVED_ARRAY(ExtEncoderROI.ROI[i].reserved2) + "\n";
    }
    str += structName + ".reserved1[]=" + DUMP_RESERVED_ARRAY(ExtEncoderROI.reserved1) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtVPPRotation &ExtVPPRotation)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPRotation.Header) + "\n";
    str += structName + "Angle.=" + ToString(ExtVPPRotation.Angle) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtVPPRotation.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtEncodedSlicesInfo &ExtEncodedSlicesInfo)
{
    std::string str;
    str += dump(structName + ".Header", ExtEncodedSlicesInfo.Header) + "\n";
    str += structName + "SliceSizeOverflow.=" + ToString(ExtEncodedSlicesInfo.SliceSizeOverflow) + "\n";
    str += structName + "NumSliceNonCopliant.=" + ToString(ExtEncodedSlicesInfo.NumSliceNonCopliant) + "\n";
    str += structName + "NumEncodedSlice.=" + ToString(ExtEncodedSlicesInfo.NumEncodedSlice) + "\n";
    str += structName + "NumSliceSizeAlloc.=" + ToString(ExtEncodedSlicesInfo.NumSliceSizeAlloc) + "\n";
    str += structName + "SliceSize.=" + ToHexFormatString(ExtEncodedSlicesInfo.SliceSize) + "\n";
    str += structName + "reserved1.=" + ToString(ExtEncodedSlicesInfo.reserved1) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtEncodedSlicesInfo.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtVPPScaling &ExtVPPScaling)
{
    std::string str;
    str += dump(structName + ".Header", ExtVPPScaling.Header) + "\n";
    str += structName + "ScalingMode.=" + ToString(ExtVPPScaling.ScalingMode) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtVPPScaling.reserved) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtDecVideoProcessing &ExtDecVideoProcessing)
{
    std::string str;
    /* IN structures */
    str += dump(structName + ".Header", ExtDecVideoProcessing.Header) + "\n";
    str += structName + "In.CropX=" + ToString(ExtDecVideoProcessing.In.CropX) + "\n";
    str += structName + "In.CropY=" + ToString(ExtDecVideoProcessing.In.CropY) + "\n";
    str += structName + "In.CropW=" + ToString(ExtDecVideoProcessing.In.CropW) + "\n";
    str += structName + "In.CropH=" + ToString(ExtDecVideoProcessing.In.CropH) + "\n";
    str += structName + "In.reserved[]=" + DUMP_RESERVED_ARRAY(ExtDecVideoProcessing.In.reserved) + "\n";
    /* Out structures */
    str += structName + "Out.FourCC=" + ToString(ExtDecVideoProcessing.Out.FourCC) + "\n";
    str += structName + "Out.ChromaFormat=" + ToString(ExtDecVideoProcessing.Out.ChromaFormat) + "\n";
    str += structName + "Out.reserved1=" + ToString(ExtDecVideoProcessing.Out.reserved1) + "\n";
    str += structName + "Out.Width=" + ToString(ExtDecVideoProcessing.Out.Width) + "\n";
    str += structName + "Out.Height=" + ToString(ExtDecVideoProcessing.Out.Height) + "\n";
    str += structName + "OutCropX.=" + ToString(ExtDecVideoProcessing.Out.CropX) + "\n";
    str += structName + "OutCropY.=" + ToString(ExtDecVideoProcessing.Out.CropY) + "\n";
    str += structName + "OutCropW.=" + ToString(ExtDecVideoProcessing.Out.CropW) + "\n";
    str += structName + "OutCropH.=" + ToString(ExtDecVideoProcessing.Out.CropH) + "\n";
    str += structName + "Out.reserved[]=" + DUMP_RESERVED_ARRAY(ExtDecVideoProcessing.Out.reserved) + "\n";

    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtDecVideoProcessing.reserved) + "\n";

    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtMBQP &ExtMBQP)
{
    std::string str;
    str += dump(structName + ".Header", ExtMBQP.Header) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtMBQP.reserved) + "\n";
    str += structName + ".Mode=" + ToString(ExtMBQP.Mode) + "\n";
    str += structName + ".BlockSize=" + ToString(ExtMBQP.BlockSize) + "\n";
    str += structName + ".NumQPAlloc=" + ToString(ExtMBQP.NumQPAlloc) + "\n";
    str += structName + ".QP=" + ToString(ExtMBQP.QP) + "\n";
    str += structName + ".DeltaQP=" + ToString(ExtMBQP.DeltaQP) + "\n";
    str += structName + "reserverd2=" + ToString(ExtMBQP.reserved2) + "\n";
    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtEncoderIPCMArea &ExtEncoderIPCMArea)
{
    std::string str;
    str += dump(structName + ".Header", ExtEncoderIPCMArea.Header) + "\n";
    str += structName + ".reserve1[]=" + DUMP_RESERVED_ARRAY(ExtEncoderIPCMArea.reserve1) + "\n";
    str += structName + ".NumArea=" + ToString(ExtEncoderIPCMArea.NumArea) + "\n";
    // dump Area
    if (ExtEncoderIPCMArea.Areas == nullptr)
    {
        str += structName + ".Areas = nullptr \n";
        return str;
    }
 
    for (mfxU16 i = 0; i < ExtEncoderIPCMArea.NumArea; i++) {
        str += structName + ".Areas[" + ToString(i) + "].Left=" + ToString(ExtEncoderIPCMArea.Areas[i].Left) + "\n";
        str += structName + ".Areas[" + ToString(i) + "].Top=" + ToString(ExtEncoderIPCMArea.Areas[i].Top) + "\n";
        str += structName + ".Areas[" + ToString(i) + "].Right=" + ToString(ExtEncoderIPCMArea.Areas[i].Right) + "\n";
        str += structName + ".Areas[" + ToString(i) + "].Bottom=" + ToString(ExtEncoderIPCMArea.Areas[i].Bottom) + "\n";
        str += structName + ".Areas[" + ToString(i) + "].reserved2=" +  DUMP_RESERVED_ARRAY(ExtEncoderIPCMArea.Areas[i].reserved2) + "\n";
    }
    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtInsertHeaders& ExtInsertHeaders)
{
    std::string str;
    str += dump(structName + ".Header", ExtInsertHeaders.Header) + "\n";
    str += structName + ".SPS="  + ToString(ExtInsertHeaders.SPS) + "\n";
    str += structName + ".PPS=" + ToString(ExtInsertHeaders.PPS) + "\n";
    return str;
}

#if (MFX_VERSION >= 1025)
std::string DumpContext::dump(const std::string structName, const  mfxExtMasteringDisplayColourVolume &_struct) {
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    str += structName + ".InsertPayloadToggle=" + ToString(_struct.InsertPayloadToggle) + "\n";
    for (int i = 0; i < 3; i++) {
        str += structName + ".DisplayPrimariesX[" + ToString(i) + "]=" + ToString(_struct.DisplayPrimariesX[i]) + "\n";
        str += structName + ".DisplayPrimariesY[" + ToString(i) + "]=" + ToString(_struct.DisplayPrimariesY[i]) + "\n";
    }
    str += structName + ".WhitePointX=" + ToString(_struct.WhitePointX) + "\n";
    str += structName + ".WhitePointY=" + ToString(_struct.WhitePointY) + "\n";
    str += structName + ".MaxDisplayMasteringLuminance=" + ToString(_struct.MaxDisplayMasteringLuminance) + "\n";
    str += structName + ".MinDisplayMasteringLuminance=" + ToString(_struct.MinDisplayMasteringLuminance) + "\n";

    DUMP_FIELD_RESERVED(reserved);
    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtContentLightLevelInfo &_struct) {
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    str += structName + ".InsertPayloadToggle=" + ToString(_struct.InsertPayloadToggle) + "\n";
    str += structName + ".MaxContentLightLevel=" + ToString(_struct.MaxContentLightLevel) + "\n";
    str += structName + ".MaxPicAverageLightLevel=" + ToString(_struct.MaxPicAverageLightLevel) + "\n";

    DUMP_FIELD_RESERVED(reserved);
    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtMultiFrameParam &_struct) {
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    str += structName + ".MFMode=" + ToString(_struct.MFMode) + "\n";
    str += structName + ".MaxNumFrames=" + ToString(_struct.MaxNumFrames) + "\n";

    DUMP_FIELD_RESERVED(reserved);
    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtMultiFrameControl &_struct) {
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    str += structName + ".Timeout=" + ToString(_struct.Timeout) + "\n";
    str += structName + ".Flush=" + ToString(_struct.Flush) + "\n";

    DUMP_FIELD_RESERVED(reserved);
    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtEncodedUnitsInfo &_struct) {
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    str += structName + ".NumUnitsAlloc=" + ToString(_struct.NumUnitsAlloc) + "\n";
    str += structName + ".NumUnitsEncoded=" + ToString(_struct.NumUnitsEncoded) + "\n";

    if (_struct.UnitInfo != NULL)
    {
        int count = min(_struct.NumUnitsEncoded, _struct.NumUnitsAlloc);
        for (int i = 0; i < count; i++)
        {
            str += structName + ".UnitInfo[" + ToString(i) + "].Type=" + ToString(_struct.UnitInfo[i].Type) + "\n";
            str += structName + ".UnitInfo[" + ToString(i) + "].Offset=" + ToString(_struct.UnitInfo[i].Offset) + "\n";
            str += structName + ".UnitInfo[" + ToString(i) + "].Size=" + ToString(_struct.UnitInfo[i].Size) + "\n";
        }
    }
    else
    {
        str += structName + ".UnitInfo=NULL\n";
    }

    DUMP_FIELD_RESERVED(reserved);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtColorConversion &ExtColorConversion) {
    std::string str;
    str += dump(structName + ".Header", ExtColorConversion.Header) + "\n";
    str += structName + ".ChromaSiting=" + ToString(ExtColorConversion.ChromaSiting) + "\n";
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(ExtColorConversion.reserved) + "\n";
    return str;
}

#endif

#if (MFX_VERSION >= 1026)
std::string DumpContext::dump(const std::string structName, const  mfxExtVppMctf &_struct) {
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    str += structName + ".FilterStrength=" + ToString(_struct.FilterStrength)  + "\n";
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    str += structName + ".Overlap=" + ToString(_struct.Overlap)  + "\n";
    str += structName + ".BitsPerPixelx100k=" + ToString(_struct.BitsPerPixelx100k)  + "\n";
    str += structName + ".Deblocking=" + ToString(_struct.Deblocking)  + "\n";
    str += structName + ".TemporalMode=" + ToString(_struct.TemporalMode)  + "\n";
    str += structName + ".MVPrecision=" + ToString(_struct.MVPrecision)  + "\n";
#endif
    DUMP_FIELD_RESERVED(reserved);
    return str;
}

#endif

#if (MFX_VERSION >= 1026)

std::string DumpContext::dump(const std::string structName, const  mfxVP9SegmentParam &_struct)
{
    std::string str;
    DUMP_FIELD(FeatureEnabled);
    DUMP_FIELD(QIndexDelta);
    DUMP_FIELD(LoopFilterLevelDelta);
    DUMP_FIELD(ReferenceFrame);
    return str;
}


std::string DumpContext::dump(const std::string structName, const  mfxExtVP9Segmentation &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(NumSegments);

    for (mfxU16 i = 0; i < 8 && i < _struct.NumSegments; i++)
    {
        str += dump(structName + ".Segment[" + ToString(i) + "]", _struct.Segment[i]) + "\n";
    }

    DUMP_FIELD(SegmentIdBlockSize);
    DUMP_FIELD(NumSegmentIdAlloc);

    if (_struct.SegmentId)
    {
        str += dump_array_with_cast<mfxU8, mfxU16>(_struct.SegmentId, _struct.NumSegmentIdAlloc);
    }
    else
    {
        DUMP_FIELD(SegmentId);
    }

    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxVP9TemporalLayer &_struct)
{
    std::string str;
    DUMP_FIELD(FrameRateScale);
    DUMP_FIELD(TargetKbps);
    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtVP9TemporalLayers &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";

    for (mfxU16 i = 0; i < 8; i++)
    {
        str += dump(structName + ".Layer[" + ToString(i) + "]", _struct.Layer[i]) + "\n";
    }

    return str;
}

std::string DumpContext::dump(const std::string structName, const  mfxExtVP9Param &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";

    DUMP_FIELD(FrameWidth);
    DUMP_FIELD(FrameHeight);
    DUMP_FIELD(WriteIVFHeaders);
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    str += structName + ".LoopFilterRefDelta[4]=" + DUMP_RESERVED_ARRAY(_struct.LoopFilterRefDelta) + "\n";
    str += structName + ".LoopFilterModeDelta[2]=" + DUMP_RESERVED_ARRAY(_struct.LoopFilterModeDelta) + "\n";
#endif
    DUMP_FIELD(QIndexDeltaLumaDC);
    DUMP_FIELD(QIndexDeltaChromaAC);
    DUMP_FIELD(QIndexDeltaChromaDC);
#if (MFX_VERSION >= 1029)
    DUMP_FIELD(NumTileRows);
    DUMP_FIELD(NumTileColumns);
#endif
    return str;
}

#endif

#if (MFX_VERSION >= 1034)
std::string DumpContext::dump(const std::string structName, const mfxExtAV1FilmGrainParam &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(FilmGrainFlags);
    DUMP_FIELD(GrainSeed);
    DUMP_FIELD(RefIdx);
    DUMP_FIELD(NumYPoints);
    DUMP_FIELD(NumCbPoints);
    DUMP_FIELD(NumCrPoints);

    for (mfxU16 i = 0; i < 14 && i < _struct.NumYPoints; i++)
    {
        str += dump(structName + ".PointY[" + ToString(i) + "].Value", _struct.PointY[i].Value) + "\n";
        str += dump(structName + ".PointY[" + ToString(i) + "].Scaling", _struct.PointY[i].Scaling) + "\n";
    }

    for (mfxU16 i = 0; i < 10 && i < _struct.NumCbPoints; i++)
    {
        str += dump(structName + ".PointCb[" + ToString(i) + "].Value", _struct.PointCb[i].Value) + "\n";
        str += dump(structName + ".PointCb[" + ToString(i) + "].Scaling", _struct.PointCb[i].Scaling) + "\n";
    }

    for (mfxU16 i = 0; i < 10 && i < _struct.NumCrPoints; i++)
    {
        str += dump(structName + ".PointCr[" + ToString(i) + "].Value", _struct.PointCr[i].Value) + "\n";
        str += dump(structName + ".PointCr[" + ToString(i) + "].Scaling", _struct.PointCr[i].Scaling) + "\n";
    }

    DUMP_FIELD(GrainScalingMinus8);
    DUMP_FIELD(ArCoeffLag);

    for (mfxU16 i = 0; i < 24; i++)
    {
        str += dump(structName + ".ArCoeffsYPlus128[" + ToString(i) + "]", _struct.ArCoeffsYPlus128[i]) + "\n";
    }

    for (mfxU16 i = 0; i < 25; i++)
    {
        str += dump(structName + ".ArCoeffsCbPlus128[" + ToString(i) + "]", _struct.ArCoeffsCbPlus128[i]) + "\n";
        str += dump(structName + ".ArCoeffsCrPlus128[" + ToString(i) + "]", _struct.ArCoeffsCrPlus128[i]) + "\n";
    }

    DUMP_FIELD(ArCoeffShiftMinus6);
    DUMP_FIELD(GrainScaleShift);
    DUMP_FIELD(CbMult);
    DUMP_FIELD(CbLumaMult);
    DUMP_FIELD(CbOffset);
    DUMP_FIELD(CrMult);
    DUMP_FIELD(CrLumaMult);
    DUMP_FIELD(CrOffset);

    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(_struct.reserved) + "\n";

    return str;
}
#endif

#if (MFX_VERSION >= MFX_VERSION_NEXT)
std::string DumpContext::dump(const std::string structName, const mfxExtAVCScalingMatrix &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(Type);

    for (mfxU8 i = 0; i < 12; ++i)
    {
        str += dump(structName + ".ScalingListPresent[" + ToString(i) + "]", _struct.ScalingListPresent[i]) + "\n";
    }

    for (mfxU8 i = 0; i < 6; ++i)
    {
        for(mfxU8 y = 0; y < 4; ++y)
            for (mfxU8 x = 0; x < 4; ++x)
            {
                str += dump(structName + ".ScalingList4x4[" + ToString(i) + "][" + ToString(y) + "][" + ToString(x) + "]", _struct.ScalingList4x4[i][y * 4+x]) + "\n";
            }
    }

    for (mfxU8 i = 0; i < 6; ++i)
    {
        for (mfxU8 y = 0; y < 8; ++y)
            for (mfxU8 x = 0; x < 8; ++x)
            {
                str += dump(structName + ".ScalingList8x8[" + ToString(i) + "][" + ToString(y) + "][" + ToString(x) + "]", _struct.ScalingList8x8[i][y * 4 + x]) + "\n";
            }
    }

    return str;
}
#endif

#if (MFX_VERSION >= MFX_VERSION_NEXT)
std::string DumpContext::dump(const std::string structName, const mfxExtPartialBitstreamParam &_struct)
{
    std::string str;
    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(BlockSize);
    DUMP_FIELD(Granularity);

    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(_struct.reserved) + "\n";

    return str;
}
#endif
