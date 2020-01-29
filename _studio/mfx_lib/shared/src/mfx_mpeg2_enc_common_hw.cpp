// Copyright (c) 2017-2020 Intel Corporation
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

#if defined(MFX_ENABLE_MPEG2_VIDEO_ENCODE)

#include "assert.h"
//#include "mfx_mpeg2_enc_ddi_hw.h"
#include "mfx_enc_common.h"
#include "mfx_mpeg2_encode_interface.h"

#include <memory>

#include "libmfx_core_interface.h"
#include "libmfx_core_factory.h"

using namespace MfxHwMpeg2Encode;

namespace MfxHwMpeg2Encode
{
    bool ConvertFrameRateMPEG2(mfxU32 FrameRateExtD, mfxU32 FrameRateExtN, mfxI32 &frame_rate_code, mfxI32 &frame_rate_extension_n, mfxI32 &frame_rate_extension_d);
}

typedef struct tagENCODE_QUERY_STATUS_DATA_tmp
{
    UINT    uBytesWritten;
} ENCODE_QUERY_STATUS_DATA_tmp;

using namespace MfxHwMpeg2Encode;

mfxStatus MfxHwMpeg2Encode::QueryHwCaps(VideoCORE* pCore, ENCODE_CAPS & hwCaps, mfxU16 codecProfile)
{
    EncodeHWCaps* pEncodeCaps = QueryCoreInterface<EncodeHWCaps>(pCore);
    if (!pEncodeCaps)
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    else
    {
        if (pEncodeCaps->GetHWCaps<ENCODE_CAPS>(DXVA2_Intel_Encode_MPEG2, &hwCaps) == MFX_ERR_NONE)
            return MFX_ERR_NONE;
    }

    std::unique_ptr<DriverEncoder> ddi;

    ddi.reset( CreatePlatformMpeg2Encoder(pCore) );
    if(ddi.get() == NULL)
        return MFX_ERR_NULL_PTR;

    mfxStatus sts = ddi->CreateAuxilliaryDevice(codecProfile);
    MFX_CHECK_STS(sts);
    ddi->QueryEncodeCaps(hwCaps);

    return pEncodeCaps->SetHWCaps<ENCODE_CAPS>(DXVA2_Intel_Encode_MPEG2, &hwCaps);
}


//-------------------------------------------------------------
//  Execute Buffers
//-------------------------------------------------------------


mfxStatus ExecuteBuffers::Init(const mfxVideoParamEx_MPEG2* par, mfxU32 funcId, bool bAllowBRC)
{
    DWORD nSlices = par->mfxVideoParams.mfx.NumSlice;
    DWORD nMBs    = (par->mfxVideoParams.mfx.FrameInfo.Width>>4)*(par->mfxVideoParams.mfx.FrameInfo.Height>>4);

    memset(&m_caps, 0, sizeof(m_caps));
    memset(&m_sps,  0, sizeof(m_sps));
    memset(&m_pps,  0, sizeof(m_pps));

    m_bOutOfRangeMV = false;
    m_bErrMBType    = false;
    m_bUseRawFrames = par->bRawFrames;
    m_fFrameRate = (double) par->mfxVideoParams.mfx.FrameInfo.FrameRateExtN/(double)par->mfxVideoParams.mfx.FrameInfo.FrameRateExtD;

    m_FrameRateExtN = par->mfxVideoParams.mfx.FrameInfo.FrameRateExtN;
    m_FrameRateExtD = par->mfxVideoParams.mfx.FrameInfo.FrameRateExtD;

    m_idxMb = (DWORD(-1));
    m_idxBs = (DWORD(-1));

    if (m_pSlice == 0)
    {
        m_nSlices = nSlices;
        m_pSlice  = new ENCODE_SET_SLICE_HEADER_MPEG2 [m_nSlices];
        memset (m_pSlice,0,sizeof(ENCODE_SET_SLICE_HEADER_MPEG2)*m_nSlices);
    }
    else
    {
        if (m_nSlices < nSlices)
        {
            return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
        }
    }

    if (m_nMBs == 0)
    {
        m_nMBs  = nMBs;
        m_pMBs  = new ENCODE_ENC_MB_DATA_MPEG2 [m_nMBs];
        memset (m_pMBs,0,sizeof(ENCODE_ENC_MB_DATA_MPEG2)*m_nMBs);
    }
    else
    {
        if (m_nMBs < nMBs)
        {
            return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
        }
    }
   // m_sps parameters

    m_bAddSPS = 1;

    m_bAddDisplayExt = par->bAddDisplayExt;
    if (m_bAddDisplayExt)
    {
        m_VideoSignalInfo = par->videoSignalInfo;
    }

    if (funcId == ENCODE_ENC_ID)
    {
        m_sps.FrameWidth   = par->mfxVideoParams.mfx.FrameInfo.Width;
        m_sps.FrameHeight  = par->mfxVideoParams.mfx.FrameInfo.Height;
    }
    else if (funcId == ENCODE_ENC_PAK_ID)
    {
        m_sps.FrameWidth = (par->mfxVideoParams.mfx.FrameInfo.CropW!=0) ?
            par->mfxVideoParams.mfx.FrameInfo.CropW:
            par->mfxVideoParams.mfx.FrameInfo.Width;

        m_sps.FrameHeight = (par->mfxVideoParams.mfx.FrameInfo.CropH!=0) ?
            par->mfxVideoParams.mfx.FrameInfo.CropH:
            par->mfxVideoParams.mfx.FrameInfo.Height;
    }
    else
        return MFX_ERR_UNSUPPORTED;

    m_sps.Profile      = (UCHAR)par->mfxVideoParams.mfx.CodecProfile;
    m_sps.Level        = (UCHAR)par->mfxVideoParams.mfx.CodecLevel;
    m_sps.ChromaFormat = (UCHAR)par->mfxVideoParams.mfx.FrameInfo.ChromaFormat;
    m_sps.TargetUsage  = (UCHAR)par->mfxVideoParams.mfx.TargetUsage;
    m_sps.progressive_sequence = par->mfxVideoParams.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE? 1:0;
    m_GOPPictureSize = par->mfxVideoParams.mfx.GopPicSize;
    m_GOPRefDist     = (UCHAR)par->mfxVideoParams.mfx.GopRefDist;
    m_GOPOptFlag     = (UCHAR)par->mfxVideoParams.mfx.GopOptFlag;

    {
        mfxU32 aw = (par->mfxVideoParams.mfx.FrameInfo.AspectRatioW != 0) ? par->mfxVideoParams.mfx.FrameInfo.AspectRatioW * m_sps.FrameWidth :m_sps.FrameWidth;
        mfxU32 ah = (par->mfxVideoParams.mfx.FrameInfo.AspectRatioH != 0) ? par->mfxVideoParams.mfx.FrameInfo.AspectRatioH * m_sps.FrameHeight:m_sps.FrameHeight;
        mfxU16 ar_code = 1;
        mfxI32 fr_code = 0, fr_codeN = 0, fr_codeD = 0;

        if(aw*3 == ah*4)
            ar_code = 2;
        else if(aw*9 == ah*16)
            ar_code = 3;
        else if(aw*100 == ah*221)
            ar_code = 4;
        else
            ar_code = 1;

        m_sps.AspectRatio = ar_code;

        if (!ConvertFrameRateMPEG2( par->mfxVideoParams.mfx.FrameInfo.FrameRateExtD,
                                    par->mfxVideoParams.mfx.FrameInfo.FrameRateExtN,
                                    fr_code, fr_codeN, fr_codeD))
        {
          return MFX_ERR_UNSUPPORTED;
        }

        m_sps.FrameRateCode = (USHORT) fr_code;
        m_sps.FrameRateExtD = (USHORT) fr_codeD;
        m_sps.FrameRateExtN = (USHORT) fr_codeN;

        mfxU32 multiplier = std::max<mfxU32>(par->mfxVideoParams.mfx.BRCParamMultiplier, 1);

        m_sps.bit_rate = (par->mfxVideoParams.mfx.RateControlMethod != MFX_RATECONTROL_CQP) ?
                                                            par->mfxVideoParams.mfx.TargetKbps * multiplier : 0;
        m_sps.vbv_buffer_size =((par->mfxVideoParams.mfx.BufferSizeInKB * multiplier) >> 1);
        m_sps.low_delay = 0;
        m_sps.ChromaFormat = (UCHAR)par->mfxVideoParams.mfx.FrameInfo.ChromaFormat;
        m_sps.progressive_sequence = par->mfxVideoParams.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE? 1:0;
        m_sps.Profile = (UCHAR)par->mfxVideoParams.mfx.CodecProfile;
        m_sps.Level = (UCHAR)par->mfxVideoParams.mfx.CodecLevel;
        m_sps.TargetUsage = (UCHAR)par->mfxVideoParams.mfx.TargetUsage;

        m_sps.RateControlMethod = bAllowBRC ? (UCHAR)par->mfxVideoParams.mfx.RateControlMethod: 0;
        m_sps.MaxBitRate        = (UINT)par->mfxVideoParams.mfx.MaxKbps * multiplier;
        m_sps.MinBitRate        = m_sps.bit_rate;
        m_sps.UserMaxFrameSize  = par->mfxVideoParams.mfx.BufferSizeInKB * 1000 * multiplier;
        m_sps.InitVBVBufferFullnessInBit = par->mfxVideoParams.mfx.InitialDelayInKB * 8000 * multiplier;
        m_sps.AVBRAccuracy = par->mfxVideoParams.mfx.Accuracy;
        m_sps.AVBRConvergence = par->mfxVideoParams.mfx.Convergence;
    }

    if (par->bMbqpMode)
    {
        mfxU32 wMB = (par->mfxVideoParams.mfx.FrameInfo.CropW + 15) / 16;
        mfxU32 hMB = (par->mfxVideoParams.mfx.FrameInfo.CropH + 15) / 16;
        m_mbqp_data = new mfxU8[wMB*hMB];
    }

    m_bDisablePanicMode = par->bDisablePanicMode;

    // m_caps parameters

    m_caps.IntraPredBlockSize = (par->bAllowFieldDCT) ?
        ENC_INTER_BLOCK_SIZE_16x16|ENC_INTER_BLOCK_SIZE_16x8:ENC_INTRA_BLOCK_16x16;
    m_caps.IntraPredCostType  = ENC_COST_TYPE_SAD;
    m_caps.InterPredBlockSize = (par->bAllowFieldDCT) ?
        ENC_INTER_BLOCK_SIZE_16x16|ENC_INTER_BLOCK_SIZE_16x8:
        ENC_INTER_BLOCK_SIZE_16x16;

    m_caps.MVPrecision           = ENC_MV_PRECISION_INTEGER|ENC_MV_PRECISION_HALFPEL;
    m_caps.MECostType            = ENC_COST_TYPE_PROPRIETARY;
    m_caps.MESearchType          = ENC_INTER_SEARCH_TYPE_PROPRIETARY;
    m_caps.MVSearchWindowX       = par->MVRangeP[0];
    m_caps.MVSearchWindowY       = par->MVRangeP[1];
    m_caps.MEInterpolationMethod = ENC_INTERPOLATION_TYPE_BILINEAR;
    //m_caps.MEFractionalSearchType= ENC_COST_TYPE_SAD;
    m_caps.MaxMVs                = 4;
    m_caps.SkipCheck             = 1;
    m_caps.DirectCheck           = 1;
    m_caps.BiDirSearch           = (par->mfxVideoParams.mfx.GopRefDist > 1)? 1:0;
    m_caps.MBAFF                 = 0;
    m_caps.FieldPrediction       = (par->bFieldCoding && par->mfxVideoParams.mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE) ? 1:0;
    m_caps.RefOppositeField      = 0;
    m_caps.ChromaInME            = 0;
    m_caps.WeightedPrediction    = 0;
    m_caps.RateDistortionOpt     = 0;
    m_caps.MVPrediction          = 1;
    m_caps.DirectVME             = 0;

    InitFramesSet(0, 0, 0, 0, 0);



    m_encrypt.Init(par, funcId);



    return MFX_ERR_NONE;

} // mfxStatus ExecuteBuffers::Init(const mfxVideoParamEx_MPEG2* par)


mfxStatus ExecuteBuffers::Close()
{
    delete [] m_pSlice;
    m_pSlice = 0;

    m_nSlices = 0;

    memset(&m_caps, 0, sizeof(m_caps));
    memset(&m_sps,  0, sizeof(m_sps));
    memset(&m_pps,  0, sizeof(m_pps));

    m_idxMb = (DWORD(-1));
    delete [] m_pMBs;
    delete [] m_mbqp_data;
    m_mbqp_data = 0;

    m_nMBs = 0;
    m_pMBs  = 0;

    if (m_bOutOfRangeMV)
    {
        //printf ("\n\n --------- ERROR: Out of range MVs are found out. ---------\n\n");
        m_bOutOfRangeMV = false;
    }
    if (m_bErrMBType)
    {
        //printf ("\n\n --------- ERROR: incorrect MB type ---------\n\n");
        m_bOutOfRangeMV = false;
    }

    return MFX_ERR_NONE;

} // mfxStatus ExecuteBuffers::Close()


mfxStatus ExecuteBuffers::InitPictureParameters(mfxFrameParamMPEG2*  pParams, mfxU32 frameNum)
{
    m_pps.CurrReconstructedPic.bPicEntry = 0;
    m_pps.RefFrameList[0].bPicEntry      = 0;
    m_pps.RefFrameList[1].bPicEntry      = 0;
    m_pps.temporal_reference             = pParams->TemporalReference;

    if (pParams->FrameType & MFX_FRAMETYPE_I)
    {
        m_pps.picture_coding_type = CODING_TYPE_I;
    }
    else if (pParams->FrameType & MFX_FRAMETYPE_P)
    {
        m_pps.picture_coding_type = CODING_TYPE_P;
    }
    else if (pParams->FrameType & MFX_FRAMETYPE_B)
    {
        m_pps.picture_coding_type = CODING_TYPE_B;
    }
    else
    {
        return MFX_ERR_UNSUPPORTED;
    }
    m_pps.FieldCodingFlag = 0;
    m_pps.FieldFrameCodingFlag = 0;
    m_pps.InterleavedFieldBFF = 0;
    m_pps.ProgressiveField = 0;
    m_pps.CurrReconstructedPic.AssociatedFlag = 0;

    if (pParams->ProgressiveFrame)
    {
        m_pps.FieldCodingFlag        = 0;
        m_pps.FieldFrameCodingFlag    = 0;
        m_pps.InterleavedFieldBFF    = (pParams->TopFieldFirst)? 0:1;
    }
    else if (pParams->FieldPicFlag)
    {
        m_pps.FieldCodingFlag        = 1;
        m_pps.FieldFrameCodingFlag    = 0;
        m_pps.InterleavedFieldBFF    = 0;
        m_pps.CurrReconstructedPic.AssociatedFlag = (pParams->BottomFieldFlag)? 1:0;
    }
    else
    {
        m_pps.FieldCodingFlag         = 0;
        m_pps.FieldFrameCodingFlag    = 1;
        m_pps.InterleavedFieldBFF     = (pParams->TopFieldFirst)? 0:1;
    }

    m_pps.NumSlice                   =  UCHAR(pParams->FrameHinMbMinus1 + 1);
    m_pps.bPicBackwardPrediction     =  pParams->BackwardPredFlag;
    m_pps.bBidirectionalAveragingMode= (pParams->BackwardPredFlag && pParams->ForwardPredFlag);
    m_pps.bUseRawPicForRef           =  m_bUseRawFrames;
    m_pps.StatusReportFeedbackNumber =  frameNum + 1;

    m_pps.alternate_scan             = pParams->AlternateScan;
    m_pps.intra_vlc_format           = pParams->IntraVLCformat;
    m_pps.q_scale_type               = pParams->QuantScaleType;
    m_pps.concealment_motion_vectors = pParams->ConcealmentMVs;
    m_pps.frame_pred_frame_dct       = (pParams->FrameDCTprediction )?1:0;


    m_pps.DisableMismatchControl     = 0;
    m_pps.intra_dc_precision         = pParams->IntraDCprecision;

    m_pps.f_code00                   = (pParams->BitStreamFcodes >> 12) & 0x0f;
    m_pps.f_code01                   = (pParams->BitStreamFcodes >> 8 ) & 0x0f;
    m_pps.f_code10                   = (pParams->BitStreamFcodes >> 4 ) & 0x0f;
    m_pps.f_code11                   = (pParams->BitStreamFcodes >> 0 ) & 0x0f;


    m_pps.bLastPicInStream           = (pParams->ExtraFlags & MFX_IFLAG_ADD_EOS)!=0 ? 1:0;
    m_pps.bNewGop                    = m_pps.picture_coding_type == CODING_TYPE_I && (!pParams->FieldPicFlag || pParams->SecondFieldFlag)? 1:0;
    m_pps.GopPicSize                 = m_GOPPictureSize;
    m_pps.GopRefDist                 = m_GOPRefDist;
    m_pps.GopOptFlag                 = m_GOPOptFlag;
    {
        mfxI32 num = 0;
        mfxI32 fps = 0, pict = 0, sec = 0, minute = 0, hour = 0;
        num = frameNum;
        fps = (int32_t)(m_fFrameRate + 0.5);
        pict = num % fps;
        num = (num - pict) / fps;
        sec = num % 60;
        num = (num - sec) / 60;
        minute = num % 60;
        num = (num - minute) / 60;
        hour = num % 24;
        m_pps.time_code =(hour<<19) | (minute<<13) | (1<<12) | (sec<<6) | pict;
    }
    m_pps.temporal_reference        =   pParams->TemporalReference;
    m_pps.vbv_delay                 =   (USHORT)pParams->VBVDelay;
    m_pps.repeat_first_field        =   pParams->RepeatFirstField;
    m_pps.composite_display_flag    =   0;
    m_pps.v_axis                    =   0;
    m_pps.field_sequence            =   0;
    m_pps.sub_carrier               =   0;
    m_pps.burst_amplitude           =   0;
    m_pps.sub_carrier_phase         =   0;

    if ((pParams->ExtraFlags & MFX_IFLAG_ADD_HEADER)!=0 && !pParams->SecondFieldFlag)
    {
        m_bAddSPS = 1;
    }
    m_pps.bPic4MVallowed = 1;

    return MFX_ERR_NONE;
}
void ExecuteBuffers::InitFramesSet(mfxMemId curr, bool bExternal, mfxMemId rec, mfxMemId ref_0,mfxMemId ref_1)
{

    m_CurrFrameMemID        = curr;
    m_bExternalCurrFrame    = bExternal;
    m_RecFrameMemID         = rec;
    m_RefFrameMemID[0]      = ref_0;
    m_RefFrameMemID[1]      = ref_1;

}

static int32_t QuantToScaleCode(int32_t quant_value, int32_t q_scale_type)
{
    if (q_scale_type == 0)
    {
        return (quant_value + 1)/2;
    }
    else
    {
        if (quant_value <= 8)
            return quant_value;
        else if (quant_value <= 24)
            return 8 + (quant_value - 8)/2;
        else if (quant_value <= 56)
            return 16 + (quant_value - 24)/4;
        else
            return 24 + (quant_value - 56)/8;
    }
}

mfxStatus ExecuteBuffers::InitSliceParameters(mfxU8 qp, mfxU16 scale_type, mfxU8 * mbqp, mfxU32 numMB)
{
    if (m_pps.NumSlice > m_nSlices)
        return MFX_ERR_UNSUPPORTED;

    mfxU8  intra = (m_pps.picture_coding_type == CODING_TYPE_I)? 1:0;
    mfxU16 numMBSlice = (mfxU16)((m_sps.FrameWidth +15)>>4);

    bool isMBQP = (m_mbqp_data != 0) && (mbqp != 0) && (mbqp[0] != 0);

    if (isMBQP)
    {
        for (mfxU32 i = 0; i < numMB; i++)
        {
            m_mbqp_data[i] = (uint8_t)QuantToScaleCode(mbqp[i], scale_type);
        }
    }

    for (int i=0; i<(int)m_pps.NumSlice; i++)
    {
        ENCODE_SET_SLICE_HEADER_MPEG2*  pDDISlice = &m_pSlice[i];
        pDDISlice->FirstMbX                       = 0;
        pDDISlice->FirstMbY                       = (mfxU16)i;
        pDDISlice->NumMbsForSlice                 = numMBSlice;
        pDDISlice->IntraSlice                     = intra;
        pDDISlice->quantiser_scale_code           = isMBQP ? mbqp[i*numMBSlice] : qp;
        //pDDISlice->quantiser_scale_code           = qp;
    }

    return MFX_ERR_NONE;

} // mfxStatus ExecuteBuffers::InitSliceParameters


//depreciated functions
namespace MfxHwMpeg2Encode
{
    bool ConvertFrameRateMPEG2(mfxU32 FrameRateExtD, mfxU32 FrameRateExtN, mfxI32 &frame_rate_code, mfxI32 &frame_rate_extension_n, mfxI32 &frame_rate_extension_d)
    {
        static const mfxF64 ratetab[8]=
        {24000.0/1001.0,24.0,25.0,30000.0/1001.0,30.0,50.0,60000.0/1001.0,60.0};

        const mfxI32 sorted_ratio[][2] =
        {
            {1,32},{1,31},{1,30},{1,29},{1,28},{1,27},{1,26},{1,25},{1,24},{1,23},{1,22},{1,21},{1,20},{1,19},{1,18},{1,17},
            {1,16},{2,31},{1,15},{2,29},{1,14},{2,27},{1,13},{2,25},{1,12},{2,23},{1,11},{3,32},{2,21},{3,31},{1,10},{3,29},
            {2,19},{3,28},{1, 9},{3,26},{2,17},{3,25},{1, 8},{4,31},{3,23},{2,15},{3,22},{4,29},{1, 7},{4,27},{3,20},{2,13},
            {3,19},{4,25},{1, 6},{4,23},{3,17},{2,11},{3,16},{4,21},{1, 5},{4,19},{3,14},{2, 9},{3,13},{4,17},{1, 4},{4,15},
            {3,11},{2, 7},{3,10},{4,13},{1, 3},{4,11},{3, 8},{2, 5},{3, 7},{4, 9},{1, 2},{4, 7},{3, 5},{2, 3},{3, 4},{4, 5},
            {1,1},{4,3},{3,2},{2,1},{3,1},{4,1}
        };

        const mfxI32 srsize = sizeof(sorted_ratio)/sizeof(sorted_ratio[0]);
        const mfxI32 rtsize = sizeof(ratetab)/sizeof(ratetab[0]);

        if (!FrameRateExtD || !FrameRateExtN)
        {
            return false;
        }

        mfxF64    new_fr = (mfxF64)FrameRateExtN/(mfxF64)FrameRateExtD;
        mfxI32    i=0, j=0, besti=0, bestj=0;
        mfxF64    ratio=0.0, bestratio = 1.5;
        mfxI32    fr1001 = (int32_t)(new_fr*1001+.5);

        frame_rate_code = 5;
        frame_rate_extension_n = 0;
        frame_rate_extension_d = 0;

        for(j=0;j<rtsize;j++)
        {
            int32_t try1001 = (int32_t)(ratetab[j]*1001+.5);
            if(fr1001 == try1001)
            {
                frame_rate_code = j+1;
                return true;
            }
        }
        if(new_fr < ratetab[0]/sorted_ratio[0][1]*0.7)
            return false;

        for(j=0;j<rtsize;j++)
        {
            ratio = ratetab[j] - new_fr; // just difference here
            if(ratio < 0.0001 && ratio > -0.0001)
            { // was checked above with bigger range
                frame_rate_code = j+1;
                frame_rate_extension_n = frame_rate_extension_d = 0;
                return true;
            }

            for(i=0;i<srsize+1;i++)
            { // +1 because we want to analyze last point as well
                if((i<srsize)? (ratetab[j]*sorted_ratio[i][0] > new_fr*sorted_ratio[i][1]) : true)
                {
                    if(i>0)
                    {
                        ratio = ratetab[j]*sorted_ratio[i-1][0] / (new_fr*sorted_ratio[i-1][1]); // up to 1
                        if(1/ratio < bestratio)
                        {
                            besti = i-1;
                            bestj = j;
                            bestratio = 1/ratio;
                        }
                    }
                    if(i<srsize)
                    {
                        ratio = ratetab[j]*sorted_ratio[i][0] / (new_fr*sorted_ratio[i][1]); // down to 1
                        if(ratio < bestratio)
                        {
                            besti = i;
                            bestj = j;
                            bestratio = ratio;
                        }
                    }
                    break;
                }
            }
        }

        if(bestratio > 1.005)
            return false;

        frame_rate_code = bestj+1;
        frame_rate_extension_n = sorted_ratio[besti][0]-1;
        frame_rate_extension_d = sorted_ratio[besti][1]-1;

        return true;

    }
}

#endif
/* EOF */
