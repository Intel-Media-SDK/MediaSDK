//test_trace.cpp - trace functions only
//no blocks
#include "test_trace.h"
#include <iomanip>

_print_param print_param;

inline bool pflag(mfxU32 f) { return !!(print_param.flags & f);};
inline void set_pflag(mfxU32 f){print_param.flags |= f;};
inline void reset_pflag(){print_param.flags = 0;};

#define PUT_PARP(prefix, par) print_param.padding << "  " << #par << " = " << prefix.par  << '\n'
#define PUT_4CC(par) print_param.padding << "  " << #par << " = " << ((char*)&p.par)[0] << ((char*)&p.par)[1] << ((char*)&p.par)[2] << ((char*)&p.par)[3] << '\n'
#define PUT_ARRP(prefix, name, size) \
    print_param.padding << "  " << #name << '[' << (size) << "] = { ";\
    if(prefix.name && (size)){for(mfxU16 _i = 0; _i < (size)-1; _i++) os << prefix.name[_i] << ", ";\
    os << prefix.name[(size)-1];} os << " }\n"
#define PUT_DATAP(prefix, par, size) print_param.padding << "  " << #par << " = " << hexstr(prefix.par, (size))  << '\n'
#define PUT_PAR(par) PUT_PARP(p, par)
#define PUT_ARR(name, size) PUT_ARRP(p, name, size)
#define PUT_DATA(par, size) PUT_DATAP(p, par, size)
#define PUT_STRUCT(par) print_param.padding << "  " << #par << " = "; INC_PADDING(); os << p.par ; DEC_PADDING(); os << '\n'
#define PRINT_PTR(type)\
    std::ostream &operator << (std::ostream &os, type* &p){\
        if(p) return os << (void*)p << " &(" << *p << ')';\
        return os << (void*)p;\
    };
#define DEF_STRUCT_TRACE(name) \
    PRINT_PTR(name); \
    std::ostream &operator << (std::ostream &os, name &p)

std::ostream &operator << (std::ostream &os, mfxStatus &st){
#define PRINT_STS_LOCAL(sts) case sts: return os << #sts << '(' << sts << ')';
    switch(st){
        PRINT_STS_LOCAL(MFX_ERR_NONE);
        PRINT_STS_LOCAL(MFX_ERR_UNKNOWN);
        PRINT_STS_LOCAL(MFX_ERR_NULL_PTR);
        PRINT_STS_LOCAL(MFX_ERR_UNSUPPORTED);
        PRINT_STS_LOCAL(MFX_ERR_MEMORY_ALLOC);
        PRINT_STS_LOCAL(MFX_ERR_NOT_ENOUGH_BUFFER);
        PRINT_STS_LOCAL(MFX_ERR_INVALID_HANDLE);
        PRINT_STS_LOCAL(MFX_ERR_LOCK_MEMORY);
        PRINT_STS_LOCAL(MFX_ERR_NOT_INITIALIZED);
        PRINT_STS_LOCAL(MFX_ERR_NOT_FOUND);
        PRINT_STS_LOCAL(MFX_ERR_MORE_DATA);
        PRINT_STS_LOCAL(MFX_ERR_MORE_SURFACE);
        PRINT_STS_LOCAL(MFX_ERR_ABORTED);
        PRINT_STS_LOCAL(MFX_ERR_DEVICE_LOST);
        PRINT_STS_LOCAL(MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        PRINT_STS_LOCAL(MFX_ERR_INVALID_VIDEO_PARAM);
        PRINT_STS_LOCAL(MFX_ERR_UNDEFINED_BEHAVIOR);
        PRINT_STS_LOCAL(MFX_ERR_DEVICE_FAILED);
        PRINT_STS_LOCAL(MFX_ERR_MORE_BITSTREAM);
        PRINT_STS_LOCAL(MFX_WRN_IN_EXECUTION);
        PRINT_STS_LOCAL(MFX_WRN_DEVICE_BUSY);
        PRINT_STS_LOCAL(MFX_WRN_VIDEO_PARAM_CHANGED);
        PRINT_STS_LOCAL(MFX_WRN_PARTIAL_ACCELERATION);
        PRINT_STS_LOCAL(MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        PRINT_STS_LOCAL(MFX_WRN_VALUE_NOT_CHANGED);
        PRINT_STS_LOCAL(MFX_WRN_OUT_OF_RANGE);
        PRINT_STS_LOCAL(MFX_WRN_FILTER_SKIPPED);
        PRINT_STS_LOCAL(MFX_TASK_WORKING);
        PRINT_STS_LOCAL(MFX_TASK_BUSY);
        default: return os << st;
    };
#undef PRINT_STS_LOCAL
};
PRINT_PTR(mfxSession);
PRINT_PTR(mfxSyncPoint);
PRINT_PTR(mfxFrameSurface1*);

std::ostream &operator << (std::ostream &os, mfxU8* &p){
    return os << (void*)p;
}
std::ostream &operator << (std::ostream &os, std::string &p){
    return os << p.c_str();
}
DEF_STRUCT_TRACE(mfxVideoParam){
    /*if(!pflag(0xF))*/{
        reset_pflag();
        int io_in  = !!(p.IOPattern&0x0F);
        int io_out = !!(p.IOPattern&0xF0);
        if(io_in && io_out) set_pflag(PRINT_OPT_VPP);
        else if(io_in) set_pflag(PRINT_OPT_ENC);
        else if(io_out) set_pflag(PRINT_OPT_DEC);
    }
    os  << "{\n"
        << PUT_PAR(AsyncDepth);
    if(!pflag(PRINT_OPT_VPP)) {
        os  << PUT_STRUCT(mfx);
    } else {
        os  << PUT_STRUCT(vpp);
    }
    os  << PUT_PAR(Protected)
        << PUT_PAR(IOPattern)
        << PUT_PAR(NumExtParam)
        << PUT_PAR(ExtParam);
    if(p.NumExtParam && p.ExtParam){
        INC_PADDING();
        os << print_param.padding << "{\n";
        INC_PADDING();
        for(mfxU32 i = 0; i < p.NumExtParam; i++)
            os << print_param.padding << p.ExtParam[i] << '\n';
        DEC_PADDING();
        os << print_param.padding << "}\n";
        DEC_PADDING();
    }
    os  << print_param.padding << "}";
    return os;
};
std::ostream &operator << (std::ostream &os, mfxInfoVPP &p){
    os  << "{\n"
        << PUT_STRUCT(In)
        << PUT_STRUCT(Out)
        << print_param.padding << "}";
    return os;
}
std::ostream &operator << (std::ostream &os, mfxInfoMFX &p){
    os  << "{\n"
        << PUT_PAR(LowPower)
        << PUT_PAR(BRCParamMultiplier)
        << PUT_STRUCT(FrameInfo)
        << PUT_4CC(CodecId)
        << PUT_PAR(CodecProfile)
        << PUT_PAR(CodecLevel)
        << PUT_PAR(NumThread);

    if(!pflag(0x1F0)){
        switch(p.CodecId){
            case MFX_CODEC_AVC:     set_pflag(PRINT_OPT_AVC); break;
            case MFX_CODEC_VC1:     set_pflag(PRINT_OPT_VC1); break;
            case MFX_CODEC_VP8:     set_pflag(PRINT_OPT_VP8); break;
            case MFX_CODEC_JPEG:    set_pflag(PRINT_OPT_JPEG); break;
            case MFX_CODEC_MPEG2:   set_pflag(PRINT_OPT_MPEG2); break;
            default: break;
        }
    }

    if(pflag(PRINT_OPT_ENC) && !pflag(PRINT_OPT_JPEG)){
        os  << PUT_PAR(TargetUsage)
            << PUT_PAR(GopPicSize)
            << PUT_PAR(GopRefDist)
            << PUT_PAR(GopOptFlag)
            << PUT_PAR(IdrInterval)
            << PUT_PAR(BufferSizeInKB)
            << PUT_PAR(RateControlMethod);
        if(p.RateControlMethod == MFX_RATECONTROL_AVBR){
            os  << PUT_PAR(Accuracy)
                << PUT_PAR(TargetKbps)
                << PUT_PAR(Convergence);
        }else if(p.RateControlMethod == MFX_RATECONTROL_CQP){
            os  << PUT_PAR(QPI)
                << PUT_PAR(QPP)
                << PUT_PAR(QPB);
        }else {
            os  << PUT_PAR(InitialDelayInKB)
                << PUT_PAR(TargetKbps)
                << PUT_PAR(MaxKbps);
        }
        os  << PUT_PAR(NumSlice)
            << PUT_PAR(NumRefFrame)
            << PUT_PAR(EncodedOrder);
    } else if(!pflag(PRINT_OPT_JPEG)){
        os  << PUT_PAR(DecodedOrder)
            << PUT_PAR(ExtendedPicStruct)
            << PUT_PAR(TimeStampCalc)
            << PUT_PAR(SliceGroupsPresent);
    }else if(pflag(PRINT_OPT_DEC)){
        os  << PUT_PAR(JPEGChromaFormat)
            << PUT_PAR(Rotation);
    } else {
        os  << PUT_PAR(Interleaved)
            << PUT_PAR(Quality);
    }
    os  << print_param.padding << "}";
    return os;
};
std::ostream &operator << (std::ostream &os, mfxFrameInfo &p){
    os  << "{\n"
        << PUT_STRUCT(FrameId)
        << PUT_4CC(FourCC)
        << PUT_PAR(Width)
        << PUT_PAR(Height)
        << PUT_PAR(CropX)
        << PUT_PAR(CropY)
        << PUT_PAR(CropW)
        << PUT_PAR(CropH)
        << PUT_PAR(FrameRateExtN)
        << PUT_PAR(FrameRateExtD)
        << PUT_PAR(AspectRatioW)
        << PUT_PAR(AspectRatioH)
        << PUT_PAR(PicStruct)
        << PUT_PAR(ChromaFormat)
        << print_param.padding << "}";
    return os;
};
std::ostream &operator << (std::ostream &os, mfxFrameId &p){
    os  << "{\n"
        << PUT_PAR(TemporalId)
        << PUT_PAR(PriorityId)
        << PUT_PAR(DependencyId)
        << PUT_PAR(QualityId)
        << PUT_PAR(ViewId)
        << print_param.padding << "}";
    return os;
};
DEF_STRUCT_TRACE(mfxVersion){
    os  << "{\n"
        << PUT_PAR(Minor)
        << PUT_PAR(Major)
        << PUT_PAR(Version);
    os  << print_param.padding << '}';
    return os;
};
DEF_STRUCT_TRACE(mfxFrameAllocRequest){
    os  << "{\n"
        << PUT_STRUCT(Info)
        << PUT_PAR(Type)
        << PUT_PAR(NumFrameMin)
        << PUT_PAR(NumFrameSuggested)
        << print_param.padding << '}';
    return os;
};
DEF_STRUCT_TRACE(mfxBitstream){
    os  << "{\n"
//        << PUT_PAR(EncryptedData)
        << PUT_PAR(NumExtParam)
        << PUT_PAR(ExtParam);

    if(p.NumExtParam && p.ExtParam){
        INC_PADDING();
        os << print_param.padding << "{\n";
        INC_PADDING();
        for(mfxU32 i = 0; i < p.NumExtParam; i++)
            os << print_param.padding << p.ExtParam[i] << '\n';
        DEC_PADDING();
        os << print_param.padding << "}\n";
        DEC_PADDING();
    }

    os  << PUT_PAR(DecodeTimeStamp)
        << PUT_PAR(TimeStamp)
        << PUT_PAR(Data)
        << PUT_PAR(DataOffset)
        << PUT_PAR(DataLength)
        << PUT_PAR(MaxLength)
        << PUT_PAR(PicStruct)
        << PUT_PAR(FrameType)
        << PUT_PAR(DataFlag)
        << print_param.padding << '}';
    return os;
};
DEF_STRUCT_TRACE(mfxFrameSurface1){
    os  << "{\n"
        << PUT_STRUCT(Info)
        << PUT_STRUCT(Data)
        << print_param.padding << '}';
    return os;
};
std::ostream &operator << (std::ostream &os, mfxFrameData &p){
    os  << "{\n"
#if ( (MFX_VERSION_MAJOR >= 1) && (MFX_VERSION_MINOR > 9))
        << PUT_PAR(ExtParam)
        << PUT_PAR(NumExtParam);
        if(p.NumExtParam && p.ExtParam){
            INC_PADDING();
            os << print_param.padding << "{\n";
            INC_PADDING();
            for(mfxU32 i = 0; i < p.NumExtParam; i++)
                os << print_param.padding << p.ExtParam[i] << '\n';
            DEC_PADDING();
            os << print_param.padding << "}\n";
            DEC_PADDING();
        }
    os  << print_param.padding << "}"
#endif
        << PUT_PAR(PitchHigh)
        << PUT_PAR(TimeStamp)
        << PUT_PAR(FrameOrder)
        << PUT_PAR(Locked)
        << PUT_PAR(Pitch)
        << PUT_PAR(Y)
        << PUT_PAR(U)
        << PUT_PAR(V)
        << PUT_PAR(A)
        << PUT_PAR(MemId)
        << PUT_PAR(Corrupted)
        << PUT_PAR(DataFlag)
        << print_param.padding << '}';
    return os;
};

#define PRINT_BUF(id, type) case id: if(p.BufferSz < sizeof(type)) break; os << *((type*)&p); return os;
DEF_STRUCT_TRACE(mfxExtBuffer){
    switch(p.BufferId){
#ifdef __MFXSVC_H__
        PRINT_BUF(MFX_EXTBUFF_SVC_SEQ_DESC              , mfxExtSVCSeqDesc              );
        PRINT_BUF(MFX_EXTBUFF_SVC_RATE_CONTROL          , mfxExtSVCRateControl          );
#endif //__MFXSVC_H__
        PRINT_BUF(MFX_EXTBUFF_CODING_OPTION2            , mfxExtCodingOption2           );
        PRINT_BUF(MFX_EXTBUFF_VPP_DONOTUSE              , mfxExtVPPDoNotUse             );
        PRINT_BUF(MFX_EXTBUFF_VPP_DENOISE               , mfxExtVPPDenoise              );
        PRINT_BUF(MFX_EXTBUFF_VPP_DETAIL                , mfxExtVPPDetail               );
        PRINT_BUF(MFX_EXTBUFF_VPP_PROCAMP               , mfxExtVPPProcAmp              );
        PRINT_BUF(MFX_EXTBUFF_VPP_AUXDATA               , mfxExtVppAuxData              );
        PRINT_BUF(MFX_EXTBUFF_VPP_DOUSE                 , mfxExtVPPDoUse                );
        PRINT_BUF(MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION , mfxExtVPPFrameRateConversion  );
        PRINT_BUF(MFX_EXTBUFF_VPP_IMAGE_STABILIZATION   , mfxExtVPPImageStab            );
        PRINT_BUF(MFX_EXTBUFF_MVC_SEQ_DESC              , mfxExtMVCSeqDesc              );
        PRINT_BUF(MFX_EXTBUFF_CODING_OPTION             , mfxExtCodingOption            );
        PRINT_BUF(MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION , mfxExtOpaqueSurfaceAlloc      );
//        PRINT_BUF(MFX_EXTBUFF_PAVP_OPTION               , mfxExtPAVPOption              );
#if ((MFX_VERSION_MAJOR >= 1) && (MFX_VERSION_MINOR >= 7))
        PRINT_BUF(MFX_EXTBUFF_ENCODER_CAPABILITY        , mfxExtEncoderCapability       );
        PRINT_BUF(MFX_EXTBUFF_ENCODER_RESET_OPTION      , mfxExtEncoderResetOption      );
        PRINT_BUF(MFX_EXTBUFF_ENCODED_FRAME_INFO        , mfxExtAVCEncodedFrameInfo     );
        PRINT_BUF(MFX_EXTBUFF_AVC_TEMPORAL_LAYERS       , mfxExtAvcTemporalLayers       );
        PRINT_BUF(MFX_EXTBUFF_CODING_OPTION_SPSPPS      , mfxExtCodingOptionSPSPPS      );
        PRINT_BUF(MFX_EXTBUFF_VIDEO_SIGNAL_INFO         , mfxExtVideoSignalInfo         );
#endif
#if ((MFX_VERSION_MAJOR >= 1) && (MFX_VERSION_MINOR >= 8))
        PRINT_BUF(MFX_EXTBUFF_VPP_COMPOSITE             , mfxExtVPPComposite            );
        PRINT_BUF(MFX_EXTBUFF_VPP_DEINTERLACING         , mfxExtVPPDeinterlacing        );
        PRINT_BUF(MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO     , mfxExtVPPVideoSignalInfo      );
        PRINT_BUF(MFX_EXTBUFF_ENCODER_ROI               , mfxExtEncoderROI              );
#endif
        PRINT_BUF(MFX_EXTBUFF_AVC_REFLIST_CTRL          , mfxExtAVCRefListCtrl          );
        
#ifdef __MFXVP8_H__
        PRINT_BUF(MFX_EXTBUFF_VP8_CODING_OPTION         , mfxExtVP8CodingOption         );
#endif //__MFXVP8_H__
        default: break;
    }
    return os  << "{\n"
                << PUT_4CC(BufferId)
                << PUT_PAR(BufferSz)
                << print_param.padding << '}';
};

#ifdef __MFXSVC_H__
DEF_STRUCT_TRACE(mfxExtSVCSeqDesc){
    mfxExtSVCSeqDesc zero = {0};
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_ARR(TemporalScale, 8)
        << PUT_PAR(RefBaseDist);
    if(memcmp(p.DependencyLayer, &zero, sizeof(p.DependencyLayer))){
        for(mfxU16 i = 0; i < 8; i++){
            if(!memcmp(&p.DependencyLayer[i], &zero, sizeof(p.DependencyLayer[0]))){
                os << print_param.padding << "  DependencyLayer[" << i << "] = {0}\n";
                continue;
            }
            os  << print_param.padding << "  DependencyLayer[" << i << "] = {\n";
            INC_PADDING();
            os  << PUT_PARP(p.DependencyLayer[i], Active)
                << PUT_PARP(p.DependencyLayer[i], Width)
                << PUT_PARP(p.DependencyLayer[i], Height)
                << PUT_PARP(p.DependencyLayer[i], CropX)
                << PUT_PARP(p.DependencyLayer[i], CropY)
                << PUT_PARP(p.DependencyLayer[i], CropW)
                << PUT_PARP(p.DependencyLayer[i], CropH)
                << PUT_PARP(p.DependencyLayer[i], RefLayerDid)
                << PUT_PARP(p.DependencyLayer[i], RefLayerQid)
                << PUT_PARP(p.DependencyLayer[i], GopPicSize)
                << PUT_PARP(p.DependencyLayer[i], GopRefDist)
                << PUT_PARP(p.DependencyLayer[i], GopOptFlag)
                << PUT_PARP(p.DependencyLayer[i], IdrInterval)
                << PUT_PARP(p.DependencyLayer[i], BasemodePred)
                << PUT_PARP(p.DependencyLayer[i], MotionPred)
                << PUT_PARP(p.DependencyLayer[i], ResidualPred)
                << PUT_PARP(p.DependencyLayer[i], DisableDeblockingFilter)
                << PUT_ARRP(p.DependencyLayer[i], ScaledRefLayerOffsets, 4)
                << PUT_PARP(p.DependencyLayer[i], ScanIdxPresent)
                << PUT_PARP(p.DependencyLayer[i], TemporalNum)
                << PUT_ARRP(p.DependencyLayer[i], TemporalId, 8)
                << PUT_PARP(p.DependencyLayer[i], QualityNum);
            if(!memcmp(&p.DependencyLayer[i].QualityLayer, &zero, sizeof(p.DependencyLayer[i].QualityLayer))){
                os << print_param.padding << "  QualityLayer = {0,}\n";
                DEC_PADDING();
                os  << print_param.padding << "  }\n";
                continue;
            }
            for(mfxU32 j = 0; j < 16; j++){
                if(!memcmp(&p.DependencyLayer[i].QualityLayer[j], &zero, sizeof(p.DependencyLayer[i].QualityLayer[j]))){
                    os  << print_param.padding << "  QualityLayer[" << j << "] = {0}\n";
                    continue;
                }
                os  << print_param.padding << "  QualityLayer[" << j << "] = {\n";
                INC_PADDING();
                os  << PUT_PARP(p.DependencyLayer[i].QualityLayer[j], ScanIdxStart)
                    << PUT_PARP(p.DependencyLayer[i].QualityLayer[j], ScanIdxEnd)
                    << print_param.padding << "}\n";
                DEC_PADDING();
            }
            DEC_PADDING();
            os  << print_param.padding << "  }\n";
        }
    } else {
        os << print_param.padding << "  DependencyLayer = {0,}\n";
    }
    os  << print_param.padding << '}';
    return os;
};
DEF_STRUCT_TRACE(mfxExtSVCRateControl){
    mfxExtSVCRateControl zero = {0};
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(RateControlMethod)
        << PUT_PAR(NumLayers);
    if(memcmp(p.Layer, &zero, sizeof(p.Layer))){
        for(mfxU32 i = 0; i < 1024; i++){
            if(!memcmp(&p.Layer[i], &zero, sizeof(p.Layer[i])*(1024-i))){
                os  << print_param.padding << "  Layer[" << i << "..1023] = {0}\n";
                break;
            }
            if(!memcmp(&p.Layer[i], &zero, sizeof(p.Layer[i]))){
                os  << print_param.padding << "  Layer[" << i << "] = {0}\n";
                continue;
            }
            os  << print_param.padding << "  Layer[" << i << "] = {\n";
            INC_PADDING();
            os  << PUT_PARP(p.Layer[i], TemporalId)
                << PUT_PARP(p.Layer[i], DependencyId)
                << PUT_PARP(p.Layer[i], QualityId);
            if(p.RateControlMethod == MFX_RATECONTROL_CQP){
                os  << PUT_PARP(p.Layer[i], Cqp.QPI)
                    << PUT_PARP(p.Layer[i], Cqp.QPP)
                    << PUT_PARP(p.Layer[i], Cqp.QPB);
            }else if(p.RateControlMethod == MFX_RATECONTROL_AVBR){
                os  << PUT_PARP(p.Layer[i], Avbr.TargetKbps)
                    << PUT_PARP(p.Layer[i], Avbr.Convergence)
                    << PUT_PARP(p.Layer[i], Avbr.Accuracy);
            }else{
                os  << PUT_PARP(p.Layer[i], CbrVbr.TargetKbps)
                    << PUT_PARP(p.Layer[i], CbrVbr.InitialDelayInKB)
                    << PUT_PARP(p.Layer[i], CbrVbr.BufferSizeInKB)
                    << PUT_PARP(p.Layer[i], CbrVbr.MaxKbps);
            }
            os  << print_param.padding << "}\n";
            DEC_PADDING();
        }
    } else {
        os << print_param.padding << "  Layer = {0,}\n";
    }
    os  << print_param.padding << '}';
    return os;
};

#endif //__MFXSVC_H__

DEF_STRUCT_TRACE(mfxExtCodingOption2){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(IntRefType)
        << PUT_PAR(IntRefCycleSize)
        << PUT_PAR(IntRefQPDelta)
        << PUT_PAR(MaxFrameSize)
        << PUT_PAR(MaxSliceSize)
        << PUT_PAR(BitrateLimit)
        << PUT_PAR(MBBRC)
        << PUT_PAR(ExtBRC)
#if ((MFX_VERSION_MAJOR >= 1) && (MFX_VERSION_MINOR >= 7))
        << PUT_PAR(LookAheadDepth)
        << PUT_PAR(Trellis)
        << PUT_PAR(RepeatPPS)
        << PUT_PAR(BRefType)
        << PUT_PAR(AdaptiveI)
        << PUT_PAR(AdaptiveB)
        << PUT_PAR(LookAheadDS)
        << PUT_PAR(NumMbPerSlice)
#endif //#if ((MFX_VERSION_MAJOR >= 1) && (MFX_VERSION_MINOR >= 7))
        << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxExtVPPDoNotUse){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(NumAlg);
    for (mfxU32 i = 0; i < p.NumAlg; i++){
        os << PUT_PAR(AlgList[i]);
    }
    os  << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxExtVPPDenoise){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(DenoiseFactor);
    os  << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxExtVPPDetail){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(DetailFactor);
    os  << print_param.padding << '}';
    return os;
};


DEF_STRUCT_TRACE(mfxExtVPPProcAmp){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(Brightness)
        << PUT_PAR(Contrast)
        << PUT_PAR(Hue)
        << PUT_PAR(Saturation);
    os  << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxExtVppAuxData){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(SpatialComplexity)
        << PUT_PAR(TemporalComplexity)
        << PUT_PAR(SceneChangeRate)
        << PUT_PAR(RepeatedFrame);
    os  << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxExtVPPDoUse){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(NumAlg);
    for (mfxU32 i = 0; i < p.NumAlg; i++){
        os << PUT_PAR(AlgList[i]);
    }
    os  << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxExtVPPFrameRateConversion){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(Algorithm);
    os  << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxExtVPPImageStab){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(Mode);
    os  << print_param.padding << '}';
    return os;
};

std::ostream &operator << (std::ostream &os, mfxMVCViewDependency &p){
    os  << "{\n"
        << PUT_PAR(ViewId)
        << PUT_PAR(NumAnchorRefsL0)
        << PUT_PAR(NumAnchorRefsL1)
        << PUT_ARR(AnchorRefL0, 16)
        << PUT_ARR(AnchorRefL1, 16)
        << PUT_PAR(NumNonAnchorRefsL0)
        << PUT_PAR(NumNonAnchorRefsL1)
        << PUT_ARR(NonAnchorRefL0, 16)
        << PUT_ARR(NonAnchorRefL1, 16)
        << print_param.padding << '}';
    return os;
};

std::ostream &operator << (std::ostream &os, mfxMVCOperationPoint &p){
    os  << "{\n"
        << PUT_PAR(TemporalId)
        << PUT_PAR(LevelIdc)
        << PUT_PAR(NumViews)
        << PUT_PAR(NumTargetViews);
    if(p.TargetViewId){
        os << PUT_ARR(TargetViewId, p.NumTargetViews);
    }
    os  << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxExtMVCSeqDesc){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(NumView)
        << PUT_PAR(NumViewAlloc);
    if(p.View){
        for(mfxU16 i = 0; i < p.NumViewAlloc; i++){
            os << PUT_STRUCT(View[i]);
        }
    }
    os  << PUT_PAR(NumViewId)
        << PUT_PAR(NumViewIdAlloc);
    if(p.ViewId){
        os << PUT_ARR(ViewId, p.NumViewIdAlloc);
    }
    os  << PUT_PAR(NumOP)
        << PUT_PAR(NumOPAlloc);
    if(p.OP){
        for(mfxU16 i = 0; i < p.NumOPAlloc; i++){
            os << PUT_STRUCT(OP[i]);
        }
    }

    os  << PUT_PAR(NumRefsTotal)
        << print_param.padding << '}';
    return os;
};

std::ostream &operator << (std::ostream &os, mfxI16Pair &p){
    return os  << "{ " << p.x << ", " << p.y << " }";
}
DEF_STRUCT_TRACE(mfxExtCodingOption){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(RateDistortionOpt)
        << PUT_PAR(MECostType)
        << PUT_PAR(MESearchType)
        << PUT_PAR(MVSearchWindow)
        << PUT_PAR(EndOfSequence)
        << PUT_PAR(FramePicture)
        << PUT_PAR(CAVLC)
        << PUT_PAR(RecoveryPointSEI)
        << PUT_PAR(ViewOutput)
        << PUT_PAR(NalHrdConformance)
        << PUT_PAR(SingleSeiNalUnit)
        << PUT_PAR(VuiVclHrdParameters)
        << PUT_PAR(RefPicListReordering)
        << PUT_PAR(ResetRefList)
        << PUT_PAR(RefPicMarkRep)
        << PUT_PAR(FieldOutput)
        << PUT_PAR(IntraPredBlockSize)
        << PUT_PAR(InterPredBlockSize)
        << PUT_PAR(MVPrecision)
        << PUT_PAR(MaxDecFrameBuffering)
        << PUT_PAR(AUDelimiter)
        << PUT_PAR(EndOfStream)
        << PUT_PAR(PicTimingSEI)
        << PUT_PAR(VuiNalHrdParameters)
        << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxExtOpaqueSurfaceAlloc){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(In.Surfaces)
        << PUT_PAR(In.Type)
        << PUT_PAR(In.NumSurface)
        << PUT_PAR(Out.Surfaces)
        << PUT_PAR(Out.Type)
        << PUT_PAR(Out.NumSurface);
    os  << print_param.padding << '}';
    return os;
};

rawdata hexstr(void* d, unsigned int s){
    rawdata r = {(unsigned char*)d, s};
    return r;
};

std::ostream &operator << (std::ostream &os, rawdata p){
    char f = os.fill();
    os.fill('0');
    os << std::hex;
    for(unsigned int i = 0; i < p.size; i++)
        os << std::setw(2) << (unsigned int)(p.data[i]);
    os << std::dec;
    os.fill(f);
    return os;
}

DEF_STRUCT_TRACE(mfxPayload){
    os  << "{\n"
        << PUT_DATA(Data, p.BufSize)
        << PUT_PAR(NumBit)
        << PUT_PAR(Type)
        << PUT_PAR(BufSize);
    os  << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxEncodeCtrl){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(SkipFrame)
        << PUT_PAR(QP)
        << PUT_PAR(FrameType)
        << PUT_PAR(NumExtParam)
        << PUT_PAR(ExtParam);

    if(p.NumExtParam && p.ExtParam){
        INC_PADDING();
        os << print_param.padding << "{\n";
        INC_PADDING();
        for(mfxU32 i = 0; i < p.NumExtParam; i++)
            os << print_param.padding << p.ExtParam[i] << '\n';
        DEC_PADDING();
        os << print_param.padding << "}\n";
        DEC_PADDING();
    }
    os  << PUT_PAR(NumPayload)
        << PUT_PAR(Payload);
    
    if(p.NumPayload && p.Payload){
        INC_PADDING();
        os << print_param.padding << "{\n";
        INC_PADDING();
        for(mfxU32 i = 0; i < p.NumPayload; i++)
            os << print_param.padding << p.Payload[i] << '\n';
        DEC_PADDING();
        os << print_param.padding << "}\n";
        DEC_PADDING();
    }
    os << print_param.padding << "}";
    return os;
};

/*
DEF_STRUCT_TRACE(mfxAES128CipherCounter){
    os  << "{\n"
        << PUT_PAR(IV)
        << PUT_PAR(Count)
        << print_param.padding << '}';
    return os;
};
*/

/*
DEF_STRUCT_TRACE(mfxExtPAVPOption){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_STRUCT(CipherCounter)
        << PUT_PAR(CounterIncrement)
        << PUT_PAR(EncryptionType)
        << PUT_PAR(CounterType)
        << print_param.padding << '}';
    return os;
};
*/

/*
DEF_STRUCT_TRACE(mfxEncryptedData){
    os  << "{\n"
        << PUT_PAR(Next)
        << PUT_PAR(Data)
        << PUT_PAR(DataOffset)
        << PUT_PAR(DataLength)
        << PUT_PAR(MaxLength)
        << PUT_STRUCT(CipherCounter)
        << print_param.padding << '}';
    return os;
};
*/

#ifdef __MFXPLUGIN_H__
DEF_STRUCT_TRACE(mfxPluginParam){
    os  << "{\n"
        << PUT_STRUCT(PluginUID)
        << PUT_PAR(Type)
        << PUT_PAR(CodecId)
        << PUT_PAR(ThreadPolicy)
        << PUT_PAR(MaxThreadNum)
        << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxCoreParam){
    os  << "{\n"
        << PUT_PAR(Impl)
        << PUT_PAR(Version)
        << PUT_PAR(NumWorkingThread)
        << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxPluginUID){
    os  << hexstr(p.Data, 16);
    return os;
};
#endif

#ifdef __MFXAUDIO_H__
DEF_STRUCT_TRACE(mfxAudioInfoMFX){
    os  << "{\n"
        << PUT_PAR(Bitrate)
        << PUT_PAR(NumChannel)
        << PUT_PAR(SampleFrequency)
        << PUT_PAR(BitPerSample)
        << PUT_PAR(CodecId)
        << PUT_PAR(CodecProfile)
        << PUT_PAR(CodecLevel);

    if(pflag(PRINT_OPT_DEC)){
        if(MFX_CODEC_AAC == p.CodecId){
            os << PUT_PAR(FlagPSSupportLev)
               << PUT_PAR(Layer)
               << PUT_ARR(AACHeaderData, p.AACHeaderDataSize)
               << PUT_PAR(AACHeaderDataSize);
        }
    }else if(MFX_CODEC_AAC == p.CodecId){
        os << PUT_PAR(OutputFormat)
           << PUT_PAR(StereoMode);
    }

    os  << print_param.padding << '}';
    return os;
};

DEF_STRUCT_TRACE(mfxAudioParam){
    os  << "{\n"
        << PUT_PAR(AsyncDepth);

    if(!pflag(PRINT_OPT_VPP)) {
        os  << PUT_STRUCT(mfx);
    } 

    os  << PUT_PAR(Protected)
        << PUT_PAR(NumExtParam)
        << PUT_PAR(ExtParam);

    if(p.NumExtParam && p.ExtParam){
        INC_PADDING();
        os << print_param.padding << "{\n";
        INC_PADDING();
        for(mfxU32 i = 0; i < p.NumExtParam; i++)
            os << print_param.padding << p.ExtParam[i] << '\n';
        DEC_PADDING();
        os << print_param.padding << "}\n";
        DEC_PADDING();
    }

    os  << print_param.padding << '}';
    return os;
};


DEF_STRUCT_TRACE(mfxAudioAllocRequest){
    os  << "{\n"
        << PUT_PAR(SuggestedInputSize)
        << PUT_PAR(SuggestedOutputSize)
        << print_param.padding << '}';
    return os;
};
#endif //#ifdef __MFXAUDIO_H__

#if ((MFX_VERSION_MAJOR >= 1) && (MFX_VERSION_MINOR >= 7))
DEF_STRUCT_TRACE(mfxExtEncoderCapability){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(MBPerSec)
        << print_param.padding << '}';
    return os;
}

DEF_STRUCT_TRACE(mfxExtEncoderResetOption){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(StartNewSequence)
        << print_param.padding << '}';
    return os;
}

#define PUT_LXP_INFO(prefix, name)                                    \
    print_param.padding << "  " << #name << " = {\n";            \
    INC_PADDING();INC_PADDING();                                 \
    for(mfxU32 i = 0; i < 32 && prefix.name[i].FrameOrder; i++){ \
        os  << print_param.padding << "{\n"                      \
            << PUT_PARP(prefix.name[i], FrameOrder)              \
            << PUT_PARP(prefix.name[i], PicStruct)               \
            << PUT_PARP(prefix.name[i], LongTermIdx)             \
            << print_param.padding << "}\n";                     \
    }                                                            \
    DEC_PADDING();DEC_PADDING();                                 \
    os  << print_param.padding << "  }\n"

DEF_STRUCT_TRACE(mfxExtAVCEncodedFrameInfo){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(FrameOrder)
        << PUT_PAR(PicStruct)
        << PUT_PAR(LongTermIdx)
        << PUT_PAR(MAD)
        << PUT_PAR(BRCPanicMode)
        << PUT_PAR(QP)
        << PUT_LXP_INFO(p, UsedRefListL0)
        << PUT_LXP_INFO(p, UsedRefListL1)
        << print_param.padding << '}';
    return os;
}
#endif //#if ((MFX_VERSION_MAJOR >= 1) && (MFX_VERSION_MINOR >= 7))

#define PUT_LXP_CTRL(prefix, name)                       \
    print_param.padding << "  " << #name << " = {\n";    \
    INC_PADDING();INC_PADDING();                         \
    for(mfxU32 i = 0;                                    \
        i < (sizeof(prefix.name)/sizeof(prefix.name[0])) \
        && prefix.name[i].FrameOrder; i++){              \
        os  << print_param.padding << "{\n"              \
            << PUT_PARP(prefix.name[i], FrameOrder)      \
            << PUT_PARP(prefix.name[i], PicStruct)       \
            << PUT_PARP(prefix.name[i], ViewId)          \
            << PUT_PARP(prefix.name[i], LongTermIdx)     \
            << print_param.padding << "}\n";             \
    }                                                    \
    DEC_PADDING();DEC_PADDING();                         \
    os  << print_param.padding << "  }\n"

DEF_STRUCT_TRACE(mfxExtAVCRefListCtrl){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(NumRefIdxL0Active)
        << PUT_PAR(NumRefIdxL1Active)
        << PUT_LXP_CTRL(p, PreferredRefList)
        << PUT_LXP_CTRL(p, RejectedRefList)
        << PUT_LXP_CTRL(p, LongTermRefList)
        << PUT_PAR(ApplyLongTermIdx)
        << print_param.padding << '}';
    return os;
}

DEF_STRUCT_TRACE(mfxExtAvcTemporalLayers){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz);

    for(mfxU32 i = 0; i < 8; i++){
        os  << print_param.padding << "  Layer[" << i << "] = {\n";
        INC_PADDING();
        os  << PUT_PARP(p.Layer[i], Scale)
            << print_param.padding << "}\n";
        DEC_PADDING();
    }
    os  << print_param.padding << '}';
    return os;
}

DEF_STRUCT_TRACE(mfxExtCodingOptionSPSPPS){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(SPSBuffer)
        << PUT_PAR(PPSBuffer)
        << PUT_PAR(SPSBufSize)
        << PUT_PAR(PPSBufSize)
        << PUT_PAR(SPSId)
        << PUT_PAR(PPSId)
        << print_param.padding << '}';
    return os;
}

DEF_STRUCT_TRACE(mfxExtVideoSignalInfo){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(VideoFormat)
        << PUT_PAR(VideoFullRange)
        << PUT_PAR(ColourDescriptionPresent)
        << PUT_PAR(ColourPrimaries)
        << PUT_PAR(TransferCharacteristics)
        << PUT_PAR(MatrixCoefficients)
        << print_param.padding << '}';
    return os;
}
#if ((MFX_VERSION_MAJOR >= 1) && (MFX_VERSION_MINOR >= 8))
DEF_STRUCT_TRACE(mfxVPPCompInputStream){
    os  << "{\n"
        << PUT_PAR(DstX)
        << PUT_PAR(DstY)
        << PUT_PAR(DstW)
        << PUT_PAR(DstH)
        << print_param.padding << '}';
    return os;
}

DEF_STRUCT_TRACE(mfxExtVPPComposite){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(Y)
        << PUT_PAR(U)
        << PUT_PAR(V)
        << PUT_PAR(NumInputStream);
    for(mfxU32 i = 0; i < p.NumInputStream; i++)
        os << PUT_PAR(InputStream[i]);
    os  << print_param.padding << '}';
    return os;
}

DEF_STRUCT_TRACE(mfxExtVPPDeinterlacing){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(Mode)
        << print_param.padding << '}';
    return os;
}

DEF_STRUCT_TRACE(mfxExtVPPVideoSignalInfo){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(In.TransferMatrix)
        << PUT_PAR(In.NominalRange)
        << PUT_PAR(Out.TransferMatrix)
        << PUT_PAR(Out.NominalRange)
        << print_param.padding << '}';
    return os;
}

DEF_STRUCT_TRACE(mfxExtEncoderROI){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(NumROI);

    for(mfxU32 i = 0; i < p.NumROI; i++){
        os  << print_param.padding << "  ROI[" << i << "] = {\n";
        INC_PADDING();
        os  << PUT_PAR(ROI[i].Left)
            << PUT_PAR(ROI[i].Top)
            << PUT_PAR(ROI[i].Right)
            << PUT_PAR(ROI[i].Bottom)
            << PUT_PAR(ROI[i].Priority)
            << print_param.padding << "}\n";
        DEC_PADDING();
    }
    os  << print_param.padding << '}';
    return os;
}
#endif


#ifdef __MFXVP8_H__
DEF_STRUCT_TRACE(mfxExtVP8CodingOption){
    os  << "{\n"
        << PUT_4CC(Header.BufferId)
        << PUT_PAR(Header.BufferSz)
        << PUT_PAR(EnableMultipleSegments)
        << print_param.padding << '}';
    return os;
}
#endif //__MFXVP8_H__
