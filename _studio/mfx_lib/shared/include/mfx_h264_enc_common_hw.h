// Copyright (c) 2018-2020 Intel Corporation
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

#ifndef __MFX_H264_ENC_COMMON_HW_H__
#define __MFX_H264_ENC_COMMON_HW_H__

#if defined(AS_H264LA_PLUGIN) && defined(MFX_ENABLE_ENCTOOLS)
#undef MFX_ENABLE_ENCTOOLS
#endif

#include "mfx_common.h"
#include "mfxla.h"

#if defined (MFX_ENABLE_H264_VIDEO_ENCODE_HW)

#include <vector>
#include <assert.h>
#include <memory>
#include "mfx_ext_buffers.h"
#include "mfxfei.h"
#ifdef MFX_ENABLE_ENCTOOLS
#include "mfxenctools-int.h"
#else
#include "mfxbrc.h"
#endif
#include "mfx_h264_encode_struct_vaapi.h"

#if defined(MFX_VA_LINUX)
#include <va/va.h>
#include <va/va_enc_h264.h>
#endif
#include "mfxmvc.h"

#include "umc_defs.h"

#define ENABLE_APQ_LQ


#define D3DFMT_NV12 (D3DFORMAT)(MFX_MAKEFOURCC('N', 'V', '1', '2'))
#define D3DDDIFMT_NV12 (D3DDDIFORMAT)(MFX_MAKEFOURCC('N', 'V', '1', '2'))
#define D3DDDIFMT_YU12 (D3DDDIFORMAT)(MFX_MAKEFOURCC('Y', 'U', '1', '2'))

// this guid is used to identify that device creation is performed during Query or QueryIOSurf call
static const GUID MSDK_Private_Guid_Encode_AVC_Query =
{ 0x32560c63, 0xe3dc, 0x43c9, { 0xa8, 0x16, 0xda, 0x73, 0x36, 0x45, 0x89, 0xe9 } };
// this guid is used to identify device creation for MVC BD/AVCHD dependent view
static const GUID MSDK_Private_Guid_Encode_MVC_Dependent_View =
{ 0x68bebcda, 0xefff, 0x4858, { 0x8d, 0x65, 0x92, 0x28, 0xab, 0xc5, 0x8c, 0x4e } };
// this guid is used to identify that device creation is performed during for low power encoder
static const GUID MSDK_Private_Guid_Encode_AVC_LowPower_Query =
{ 0x6815aa23, 0xc93e, 0x4a71, { 0xae, 0x66, 0xb, 0x60, 0x5d, 0x3b, 0xc4, 0xd7 } };

namespace MfxHwH264Encode
{
    class  DdiTask;
    class  InputBitstream;
    class  OutputBitstream;
    struct mfxExtSpsHeader;
    struct mfxExtPpsHeader;

    const mfxU16 CROP_UNIT_X[] = { 1, 2, 2, 1 };
    const mfxU16 CROP_UNIT_Y[] = { 1, 2, 1, 1 };

    static const mfxU16 MFX_PICSTRUCT_PART1 = MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_BFF;
    static const mfxU16 MFX_PICSTRUCT_PART2 = MFX_PICSTRUCT_FIELD_REPEATED | MFX_PICSTRUCT_FRAME_DOUBLING | MFX_PICSTRUCT_FRAME_TRIPLING;

    static const mfxU16 MFX_PICSTRUCT_FRAME_FLAGS = MFX_PICSTRUCT_FRAME_DOUBLING | MFX_PICSTRUCT_FRAME_TRIPLING;
    static const mfxU16 MFX_PICSTRUCT_FIELD_FLAGS = MFX_PICSTRUCT_FIELD_REPEATED;

    static const mfxU8 SEI_TYPE_BUFFERING_PERIOD               = 0;
    static const mfxU8 SEI_TYPE_PIC_TIMING                     = 1;
    static const mfxU8 SEI_TYPE_FILLER_PAYLOAD                 = 3;
    static const mfxU8 SEI_TYPE_RECOVERY_POINT                 = 6;
    static const mfxU8 SEI_TYPE_DEC_REF_PIC_MARKING_REPETITION = 7;
    static const mfxU8 SEI_TYPE_SCALABILITY_INFO               = 24;
// MVC BD {
    static const mfxU8 SEI_TYPE_MVC_SCALABLE_NESTING           = 37;
// MVC BD }

    static const mfxU16 MFX_RATECONTROL_WIDI_VBR = 100;

    // internally used buffers
    static const mfxU32 MFX_EXTBUFF_SPS_HEADER     = MFX_MAKEFOURCC(0xff, 'S', 'P', 'S');
    static const mfxU32 MFX_EXTBUFF_PPS_HEADER     = MFX_MAKEFOURCC(0xff, 'P', 'P', 'S');

    static const mfxU16 MFX_FRAMETYPE_IPB     = MFX_FRAMETYPE_I | MFX_FRAMETYPE_P | MFX_FRAMETYPE_B;
    static const mfxU16 MFX_FRAMETYPE_PB      = MFX_FRAMETYPE_P | MFX_FRAMETYPE_B;
    static const mfxU16 MFX_FRAMETYPE_PREF    = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
    static const mfxU16 MFX_FRAMETYPE_IREF    = MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF;
    static const mfxU16 MFX_FRAMETYPE_IREFIDR = MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR;
    static const mfxU16 MFX_FRAMETYPE_xIPB    = MFX_FRAMETYPE_xI | MFX_FRAMETYPE_xP | MFX_FRAMETYPE_xB;
    static const mfxU16 MFX_FRAMETYPE_xPREF   = MFX_FRAMETYPE_xP | MFX_FRAMETYPE_xREF;
    static const mfxU16 MFX_FRAMETYPE_xIREF   = MFX_FRAMETYPE_xI | MFX_FRAMETYPE_xREF;
    static const mfxU16 MFX_FRAMETYPE_KEYPIC  = 0x0020;

    static const mfxU16 MFX_IOPATTERN_IN_MASK_SYS_OR_D3D =
        MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_IN_VIDEO_MEMORY;

    static const mfxU16 MFX_IOPATTERN_IN_MASK =
        MFX_IOPATTERN_IN_MASK_SYS_OR_D3D | MFX_IOPATTERN_IN_OPAQUE_MEMORY;

    // masks for VideoParam.mfx.CodecProfile
    static const mfxU16 MASK_PROFILE_IDC          = (0xff);
    static const mfxU16 MASK_CONSTRAINT_SET0_FLAG = (0x100 << 0);
    static const mfxU16 MASK_CONSTRAINT_SET1_FLAG = (0x100 << 1);
    static const mfxU16 MASK_CONSTRAINT_SET2_FLAG = (0x100 << 2);
    static const mfxU16 MASK_CONSTRAINT_SET3_FLAG = (0x100 << 3);
    static const mfxU16 MASK_CONSTRAINT_SET4_FLAG = (0x100 << 4);
    static const mfxU16 MASK_CONSTRAINT_SET5_FLAG = (0x100 << 5);
    static const mfxU16 MASK_CONSTRAINT_SET6_FLAG = (0x100 << 6);
    static const mfxU16 MASK_CONSTRAINT_SET7_FLAG = (0x100 << 7);
    static const mfxU16 MASK_CONSTRAINT_SET0123_FLAG =  MASK_CONSTRAINT_SET0_FLAG | MASK_CONSTRAINT_SET1_FLAG | MASK_CONSTRAINT_SET2_FLAG | MASK_CONSTRAINT_SET3_FLAG;

    template<class T> inline T* SecondHalfOf(std::vector<T>& v) { return &v[v.size() / 2]; }

    template<class T> inline void Zero(T & obj)                { memset(reinterpret_cast<void*>(&obj), 0, sizeof(obj)); }
    template<class T> inline void Zero(std::vector<T> & vec)   { if (vec.size() > 0) memset(&vec[0], 0, sizeof(T) * vec.size()); }
    template<class T> inline void Zero(T * first, size_t cnt)  { memset(first, 0, sizeof(T) * cnt); }

    template<class T> inline bool Equal(T const & lhs, T const & rhs) { return memcmp(&lhs, &rhs, sizeof(T)) == 0; }

    template<class T, class U> inline void Copy(T & dst, U const & src)
    {
        static_assert(sizeof(T) == sizeof(U), "copy_objects_of_different_size");
        MFX_INTERNAL_CPY(&dst, &src, sizeof(dst));
    }

    template<class T> inline T & RemoveConst(T const & t) { return const_cast<T &>(t); }

    template<class T> inline T * RemoveConst(T const * t) { return const_cast<T *>(t); }

    template<class T, size_t N> inline T * Begin(T(& t)[N]) { return t; }

    template<class T, size_t N> inline T * End(T(& t)[N]) { return t + N; }

    template<class T, size_t N> inline size_t SizeOf(T(&)[N]) { return N; }

    template<class T> inline T const * Begin(std::vector<T> const & t) { return &*t.begin(); }

    template<class T> inline T const * End(std::vector<T> const & t) { return &*t.begin() + t.size(); }

    template<class T> inline T * Begin(std::vector<T> & t) { return &*t.begin(); }

    template<class T> inline T * End(std::vector<T> & t) { return &*t.begin() + t.size(); }

    template<class T> struct ExtBufTypeToId {};

#define BIND_EXTBUF_TYPE_TO_ID(TYPE, ID) template<> struct ExtBufTypeToId<TYPE> { enum { id = ID }; }
    BIND_EXTBUF_TYPE_TO_ID (mfxExtCodingOption,         MFX_EXTBUFF_CODING_OPTION            );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtCodingOptionSPSPPS,   MFX_EXTBUFF_CODING_OPTION_SPSPPS     );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtCodingOptionDDI,      MFX_EXTBUFF_DDI                      );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtVideoSignalInfo,      MFX_EXTBUFF_VIDEO_SIGNAL_INFO        );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtOpaqueSurfaceAlloc,   MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
    BIND_EXTBUF_TYPE_TO_ID (mfxExtMVCSeqDesc,           MFX_EXTBUFF_MVC_SEQ_DESC             );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtMVCTargetViews,       MFX_EXTBUFF_MVC_TARGET_VIEWS         );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtPictureTimingSEI,     MFX_EXTBUFF_PICTURE_TIMING_SEI       );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtSpsHeader,            MFX_EXTBUFF_SPS_HEADER               );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtPpsHeader,            MFX_EXTBUFF_PPS_HEADER               );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtAVCRefListCtrl,       MFX_EXTBUFF_AVC_REFLIST_CTRL         );
#if defined MFX_ENABLE_H264_ROUNDING_OFFSET
    BIND_EXTBUF_TYPE_TO_ID (mfxExtAVCRoundingOffset,    MFX_EXTBUFF_AVC_ROUNDING_OFFSET      );
#endif
    BIND_EXTBUF_TYPE_TO_ID (mfxExtAvcTemporalLayers,    MFX_EXTBUFF_AVC_TEMPORAL_LAYERS      );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtVppAuxData,           MFX_EXTBUFF_VPP_AUXDATA              );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtCodingOption2,        MFX_EXTBUFF_CODING_OPTION2           );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtAVCEncodedFrameInfo,  MFX_EXTBUFF_ENCODED_FRAME_INFO       );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtEncoderResetOption,   MFX_EXTBUFF_ENCODER_RESET_OPTION     );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtEncoderCapability,    MFX_EXTBUFF_ENCODER_CAPABILITY       );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtEncoderROI,           MFX_EXTBUFF_ENCODER_ROI              );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtLAFrameStatistics,    MFX_EXTBUFF_LOOKAHEAD_STAT           );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiParam,             MFX_EXTBUFF_FEI_PARAM                );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtAVCRefLists,          MFX_EXTBUFF_AVC_REFLISTS             );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtCodingOption3,        MFX_EXTBUFF_CODING_OPTION3           );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtMBQP,                 MFX_EXTBUFF_MBQP                     );
#if MFX_VERSION >= 1023
    BIND_EXTBUF_TYPE_TO_ID (mfxExtMBForceIntra,         MFX_EXTBUFF_MB_FORCE_INTRA           );
#endif
    BIND_EXTBUF_TYPE_TO_ID (mfxExtChromaLocInfo,        MFX_EXTBUFF_CHROMA_LOC_INFO          );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtMBDisableSkipMap,     MFX_EXTBUFF_MB_DISABLE_SKIP_MAP      );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtPredWeightTable,      MFX_EXTBUFF_PRED_WEIGHT_TABLE        );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtDirtyRect,            MFX_EXTBUFF_DIRTY_RECTANGLES         );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtMoveRect,             MFX_EXTBUFF_MOVING_RECTANGLES        );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiCodingOption,      MFX_EXTBUFF_FEI_CODING_OPTION        );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiEncMV,             MFX_EXTBUFF_FEI_ENC_MV               );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiEncMBStat,         MFX_EXTBUFF_FEI_ENC_MB_STAT          );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiPakMBCtrl,         MFX_EXTBUFF_FEI_PAK_CTRL             );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiEncFrameCtrl,      MFX_EXTBUFF_FEI_ENC_CTRL             );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiEncMVPredictors,   MFX_EXTBUFF_FEI_ENC_MV_PRED          );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiEncMBCtrl,         MFX_EXTBUFF_FEI_ENC_MB               );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiEncQP,             MFX_EXTBUFF_FEI_ENC_QP               );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiSPS,               MFX_EXTBUFF_FEI_SPS                  );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiPPS,               MFX_EXTBUFF_FEI_PPS                  );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiSliceHeader,       MFX_EXTBUFF_FEI_SLICE                );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiPreEncMV,          MFX_EXTBUFF_FEI_PREENC_MV            );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiPreEncMBStat,      MFX_EXTBUFF_FEI_PREENC_MB            );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiPreEncCtrl,        MFX_EXTBUFF_FEI_PREENC_CTRL          );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiPreEncMVPredictors,MFX_EXTBUFF_FEI_PREENC_MV_PRED       );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiRepackCtrl,        MFX_EXTBUFF_FEI_REPACK_CTRL          );
#if (MFX_VERSION >= 1025)
    BIND_EXTBUF_TYPE_TO_ID (mfxExtFeiRepackStat,        MFX_EXTBUFF_FEI_REPACK_STAT          );
#endif
#if defined (__MFXBRC_H__)
    BIND_EXTBUF_TYPE_TO_ID (mfxExtBRC,        MFX_EXTBUFF_BRC          );
#endif

#ifdef MFX_ENABLE_MFE
    BIND_EXTBUF_TYPE_TO_ID (mfxExtMultiFrameControl,     MFX_EXTBUFF_MULTI_FRAME_CONTROL     );
    BIND_EXTBUF_TYPE_TO_ID (mfxExtMultiFrameParam,       MFX_EXTBUFF_MULTI_FRAME_PARAM       );
#endif
#if defined (MFX_ENABLE_ENCTOOLS)
    BIND_EXTBUF_TYPE_TO_ID(mfxEncTools, MFX_EXTBUFF_ENCTOOLS);
    BIND_EXTBUF_TYPE_TO_ID(mfxEncToolsBRCFrameParams, MFX_EXTBUFF_ENCTOOLS_BRC_FRAME_PARAM);
    BIND_EXTBUF_TYPE_TO_ID(mfxEncToolsBRCQuantControl, MFX_EXTBUFF_ENCTOOLS_BRC_QUANT_CONTROL);
    BIND_EXTBUF_TYPE_TO_ID(mfxEncToolsBRCHRDPos, MFX_EXTBUFF_ENCTOOLS_BRC_HRD_POS);
    BIND_EXTBUF_TYPE_TO_ID(mfxEncToolsBRCEncodeResult, MFX_EXTBUFF_ENCTOOLS_BRC_ENCODE_RESULT);
    BIND_EXTBUF_TYPE_TO_ID(mfxEncToolsBRCStatus, MFX_EXTBUFF_ENCTOOLS_BRC_STATUS);
    BIND_EXTBUF_TYPE_TO_ID(mfxExtEncToolsConfig, MFX_EXTBUFF_ENCTOOLS_CONFIG);
    BIND_EXTBUF_TYPE_TO_ID(mfxEncToolsCtrlExtDevice, MFX_EXTBUFF_ENCTOOLS_DEVICE);
    BIND_EXTBUF_TYPE_TO_ID(mfxEncToolsCtrlExtAllocator, MFX_EXTBUFF_ENCTOOLS_ALLOCATOR);
    BIND_EXTBUF_TYPE_TO_ID(mfxEncToolsFrameToAnalyze, MFX_EXTBUFF_ENCTOOLS_FRAME_TO_ANALYZE);
    BIND_EXTBUF_TYPE_TO_ID(mfxEncToolsHintPreEncodeSceneChange, MFX_EXTBUFF_ENCTOOLS_HINT_SCENE_CHANGE);
    BIND_EXTBUF_TYPE_TO_ID(mfxEncToolsHintPreEncodeGOP, MFX_EXTBUFF_ENCTOOLS_HINT_GOP);
    BIND_EXTBUF_TYPE_TO_ID(mfxEncToolsHintPreEncodeARefFrames, MFX_EXTBUFF_ENCTOOLS_HINT_AREF);
    BIND_EXTBUF_TYPE_TO_ID(mfxEncToolsBRCBufferHint, MFX_EXTBUFF_ENCTOOLS_BRC_BUFFER_HINT);

#endif

#undef BIND_EXTBUF_TYPE_TO_ID

    template <class T> inline void InitExtBufHeader(T & extBuf)
    {
        Zero(extBuf);
        extBuf.Header.BufferId = ExtBufTypeToId<T>::id;
        extBuf.Header.BufferSz = sizeof(T);
    }

    template <> inline void InitExtBufHeader<mfxExtVideoSignalInfo>(mfxExtVideoSignalInfo & extBuf)
    {
        Zero(extBuf);
        extBuf.Header.BufferId = ExtBufTypeToId<mfxExtVideoSignalInfo>::id;
        extBuf.Header.BufferSz = sizeof(mfxExtVideoSignalInfo);

        // set default values
        extBuf.VideoFormat              = 5; // unspecified video format
        extBuf.VideoFullRange           = 0;
        extBuf.ColourDescriptionPresent = 0;
        extBuf.ColourPrimaries          = 2; // unspecified
        extBuf.TransferCharacteristics  = 2; // unspecified
        extBuf.MatrixCoefficients       = 2; // unspecified
    }

    template <> inline void InitExtBufHeader<mfxExtAVCRefListCtrl>(mfxExtAVCRefListCtrl & extBuf)
    {
        Zero(extBuf);
        extBuf.Header.BufferId = ExtBufTypeToId<mfxExtAVCRefListCtrl>::id;
        extBuf.Header.BufferSz = sizeof(mfxExtAVCRefListCtrl);

        for (mfxU32 i = 0; i < 32; i++)
            extBuf.PreferredRefList[i].FrameOrder = mfxU32(MFX_FRAMEORDER_UNKNOWN);
        for (mfxU32 i = 0; i < 16; i++)
            extBuf.RejectedRefList[i].FrameOrder = mfxU32(MFX_FRAMEORDER_UNKNOWN);
        for (mfxU32 i = 0; i < 16; i++)
            extBuf.LongTermRefList[i].FrameOrder = mfxU32(MFX_FRAMEORDER_UNKNOWN);
    }
#if defined(MFX_ENABLE_ENCTOOLS)
    template <> inline void InitExtBufHeader<mfxExtEncToolsConfig>(mfxExtEncToolsConfig & extBuf)
    {
        Zero(extBuf);
        extBuf.Header.BufferId = ExtBufTypeToId<mfxExtEncToolsConfig>::id;
        extBuf.Header.BufferSz = sizeof(mfxExtEncToolsConfig);

        extBuf.AdaptiveI =
            extBuf.AdaptiveB =
            extBuf.AdaptiveRefP =
            extBuf.AdaptiveRefB =
            extBuf.SceneChange =
            extBuf.AdaptiveLTR =
            extBuf.AdaptivePyramidQuantP =
            extBuf.AdaptivePyramidQuantB =
            extBuf.AdaptiveQuantMatrices =
            extBuf.BRCBufferHints =
            extBuf.BRC = MFX_CODINGOPTION_OFF;
    }
#endif
    template <class T> struct GetPointedType {};
    template <class T> struct GetPointedType<T *> { typedef T Type; };
    template <class T> struct GetPointedType<T const *> { typedef T Type; };

    struct HrdParameters
    {
        mfxU8   cpbCntMinus1;
        mfxU8   bitRateScale;
        mfxU8   cpbSizeScale;
        mfxU32  bitRateValueMinus1[32];
        mfxU32  cpbSizeValueMinus1[32];
        mfxU8   cbrFlag[32];
        mfxU8   initialCpbRemovalDelayLengthMinus1;
        mfxU8   cpbRemovalDelayLengthMinus1;
        mfxU8   dpbOutputDelayLengthMinus1;
        mfxU8   timeOffsetLength;
    };

    struct VuiParameters
    {
        struct
        {
            mfxU16  aspectRatioInfoPresent         : 1;
            mfxU16  overscanInfoPresent            : 1;
            mfxU16  overscanAppropriate            : 1;
            mfxU16  videoSignalTypePresent         : 1;
            mfxU16  videoFullRange                 : 1;
            mfxU16  colourDescriptionPresent       : 1;
            mfxU16  chromaLocInfoPresent           : 1;
            mfxU16  timingInfoPresent              : 1;
            mfxU16  fixedFrameRate                 : 1;
            mfxU16  nalHrdParametersPresent        : 1;
            mfxU16  vclHrdParametersPresent        : 1;
            mfxU16  lowDelayHrd                    : 1;
            mfxU16  picStructPresent               : 1;
            mfxU16  bitstreamRestriction           : 1;
            mfxU16  motionVectorsOverPicBoundaries : 1;
            mfxU16  reserved                       : 1;
        } flags;

        mfxU8   aspectRatioIdc;
        mfxU16  sarWidth;
        mfxU16  sarHeight;
        mfxU8   videoFormat;
        mfxU8   colourPrimaries;
        mfxU8   transferCharacteristics;
        mfxU8   matrixCoefficients;
        mfxU8   chromaSampleLocTypeTopField;
        mfxU8   chromaSampleLocTypeBottomField;
        mfxU32  numUnitsInTick;
        mfxU32  timeScale;

        HrdParameters nalHrdParameters;
        HrdParameters vclHrdParameters;

        mfxU8   maxBytesPerPicDenom;
        mfxU8   maxBitsPerMbDenom;
        mfxU8   log2MaxMvLengthHorizontal;
        mfxU8   log2MaxMvLengthVertical;
        mfxU8   numReorderFrames;
        mfxU8   maxDecFrameBuffering;
    };

    struct mfxExtSpsHeader
    {
        mfxExtBuffer Header;

        mfxU8   nalRefIdc;
        mfxU8   nalUnitType;
        mfxU8   profileIdc;

        struct
        {
            mfxU8   set0 : 1;
            mfxU8   set1 : 1;
            mfxU8   set2 : 1;
            mfxU8   set3 : 1;
            mfxU8   set4 : 1;
            mfxU8   set5 : 1;
            mfxU8   set6 : 2;
            mfxU8   set7 : 2;
        } constraints;

        mfxU8   levelIdc;
        mfxU8   seqParameterSetId;
        mfxU8   chromaFormatIdc;
        mfxU8   separateColourPlaneFlag;
        mfxU8   bitDepthLumaMinus8;
        mfxU8   bitDepthChromaMinus8;
        mfxU8   qpprimeYZeroTransformBypassFlag;
        mfxU8   seqScalingMatrixPresentFlag;
        mfxU8   seqScalingListPresentFlag[12];
        mfxU8   scalingList4x4[6][16];
        mfxU8   scalingList8x8[6][64];

        mfxU8   log2MaxFrameNumMinus4;
        mfxU8   picOrderCntType;
        mfxU8   log2MaxPicOrderCntLsbMinus4;
        mfxU8   deltaPicOrderAlwaysZeroFlag;
        mfxI32  offsetForNonRefPic;
        mfxI32  offsetForTopToBottomField;
        mfxU8   numRefFramesInPicOrderCntCycle;
        mfxI32  offsetForRefFrame[256];

        mfxU8   maxNumRefFrames;
        mfxU8   gapsInFrameNumValueAllowedFlag;
        mfxU16  picWidthInMbsMinus1;
        mfxU16  picHeightInMapUnitsMinus1;
        mfxU8   frameMbsOnlyFlag;
        mfxU8   mbAdaptiveFrameFieldFlag;
        mfxU8   direct8x8InferenceFlag;
        mfxU8   frameCroppingFlag;
        mfxU32  frameCropLeftOffset;
        mfxU32  frameCropRightOffset;
        mfxU32  frameCropTopOffset;
        mfxU32  frameCropBottomOffset;
        mfxU8   vuiParametersPresentFlag;

        VuiParameters vui;
    };

    bool operator ==(mfxExtSpsHeader const & lhs, mfxExtSpsHeader const & rhs);

    template <> inline bool Equal<mfxExtSpsHeader>(mfxExtSpsHeader const & lhs, mfxExtSpsHeader const & rhs)
    {
        return lhs == rhs;
    }

    struct mfxExtSpsSvcHeader
    {
        mfxExtBuffer Header;

        mfxU8   interLayerDeblockingFilterControlPresentFlag;
        mfxU8   extendedSpatialScalabilityIdc;
        mfxU8   chromaPhaseXPlus1Flag;
        mfxU8   chromaPhaseYPlus1;
        mfxU8   seqRefLayerChromaPhaseXPlus1Flag;
        mfxU8   seqRefLayerChromaPhaseYPlus1;
        mfxI16  seqScaledRefLayerLeftOffset;
        mfxI16  seqScaledRefLayerTopOffset;
        mfxI16  seqScaledRefLayerRightOffset;
        mfxI16  seqScaledRefLayerBottomOffset;
        mfxU8   seqTcoeffLevelPredictionFlag;
        mfxU8   adaptiveTcoeffLevelPredictionFlag;
        mfxU8   sliceHeaderRestrictionFlag;
    };


    union SliceGroupInfo
    {
        struct
        {
            mfxU32 runLengthMinus1[8];
        } t0;

        struct
        {
            mfxU32 topLeft[7];
            mfxU32 bottomRight[7];
        } t2;

        struct
        {
            mfxU8  sliceGroupChangeDirectionFlag;
            mfxU32 sliceGroupChangeRate;
        } t3;

        struct
        {
            mfxU32 picSizeInMapUnitsMinus1;
        } t6;
    };

    template <> inline void InitExtBufHeader<mfxExtSpsHeader>(mfxExtSpsHeader & extBuf)
    {
        Zero(extBuf);
        extBuf.Header.BufferId = ExtBufTypeToId<mfxExtSpsHeader>::id;
        extBuf.Header.BufferSz = sizeof (mfxExtSpsHeader);

        // set default non-zero values
        extBuf.chromaFormatIdc                   = 1;
        extBuf.vuiParametersPresentFlag          = 1;
        extBuf.vui.flags.nalHrdParametersPresent = 1;
        extBuf.vui.flags.timingInfoPresent       = 1;
        extBuf.vui.flags.fixedFrameRate          = 1;
    }

    struct mfxExtPpsHeader
    {
        mfxExtBuffer Header;

        mfxU8   nalRefIdc;
        mfxU8   picParameterSetId;
        mfxU8   seqParameterSetId;
        mfxU8   entropyCodingModeFlag;
        mfxU8   bottomFieldPicOrderInframePresentFlag;
        mfxU8   numSliceGroupsMinus1;
        mfxU8   sliceGroupMapType;

        SliceGroupInfo sliceGroupInfo;

        mfxU8   numRefIdxL0DefaultActiveMinus1;
        mfxU8   numRefIdxL1DefaultActiveMinus1;
        mfxU8   weightedPredFlag;
        mfxU8   weightedBipredIdc;
        mfxI8   picInitQpMinus26;
        mfxI8   picInitQsMinus26;
        mfxI8   chromaQpIndexOffset;
        mfxU8   deblockingFilterControlPresentFlag;
        mfxU8   constrainedIntraPredFlag;
        mfxU8   redundantPicCntPresentFlag;
        mfxU8   transform8x8ModeFlag;
        mfxU8   picScalingMatrixPresentFlag;
        mfxI8   secondChromaQpIndexOffset;
        bool    moreRbspData;

        mfxU8   scalingList4x4[6][16];
        mfxU8   scalingList8x8[6][64];
        bool    picScalingListPresentFlag[12];
    };

    struct mfxRoiDesc{
        mfxU32  Left;
        mfxU32  Top;
        mfxU32  Right;
        mfxU32  Bottom;

        mfxI16  ROIValue;
    };

    struct mfxRectDesc{
        mfxU32  Left;
        mfxU32  Top;
        mfxU32  Right;
        mfxU32  Bottom;
    };

    struct mfxMovingRectDesc{
        mfxU32  DestLeft;
        mfxU32  DestTop;
        mfxU32  DestRight;
        mfxU32  DestBottom;

        mfxU32  SourceLeft;
        mfxU32  SourceTop;
    };

    class MfxVideoParam : public mfxVideoParam
    {
    public:
        MfxVideoParam();
        MfxVideoParam(MfxVideoParam const &);
        MfxVideoParam(mfxVideoParam const &);

        MfxVideoParam & operator = (MfxVideoParam const &);
        MfxVideoParam & operator = (mfxVideoParam const &);

        void SyncVideoToCalculableParam();

        void SyncCalculableToVideoParam();

        void AlignCalcWithBRCParamMultiplier();

        void ApplyDefaultsToMvcSeqDesc();


    protected:
        void Construct(mfxVideoParam const & par);

        void ConstructMvcSeqDesc(mfxExtMVCSeqDesc const & desc);

    private:
#if defined(MFX_ENABLE_ENCTOOLS)
        mfxExtBuffer *              m_extParam[40];
#else
        mfxExtBuffer *              m_extParam[32];
#endif
        // external, documented
        mfxExtCodingOption          m_extOpt;
        mfxExtCodingOption2         m_extOpt2;
        mfxExtCodingOption3         m_extOpt3;
        mfxExtCodingOptionSPSPPS    m_extOptSpsPps;
        mfxExtVideoSignalInfo       m_extVideoSignal;
        mfxExtOpaqueSurfaceAlloc    m_extOpaque;
        mfxExtMVCSeqDesc            m_extMvcSeqDescr;
        mfxExtPictureTimingSEI      m_extPicTiming;
        mfxExtAvcTemporalLayers     m_extTempLayers;
        mfxExtEncoderResetOption    m_extEncResetOpt;
        mfxExtEncoderROI            m_extEncRoi;
        mfxExtFeiParam              m_extFeiParam;
        mfxExtChromaLocInfo         m_extChromaLoc;
        mfxExtPredWeightTable       m_extPwt;
        mfxExtDirtyRect             m_extDirtyRect;
        mfxExtMoveRect              m_extMoveRect;

        mfxExtCodingOptionDDI       m_extOptDdi;
        mfxExtSpsHeader             m_extSps;
        mfxExtPpsHeader             m_extPps;

        mfxExtFeiCodingOption       m_extFeiOpt;
        mfxExtFeiSliceHeader        m_extFeiSlice[2];
        mfxExtFeiSPS                m_extFeiSPS;
        mfxExtFeiPPS                m_extFeiPPS;

#if defined(__MFXBRC_H__)
        mfxExtBRC                   m_extBRC;
#endif
#if defined(MFX_ENABLE_ENCTOOLS)
        mfxEncTools                     m_encTools;
        mfxExtEncToolsConfig            m_encToolsConfig;
        mfxEncToolsCtrlExtDevice        m_extDevice;
        mfxEncToolsCtrlExtAllocator     m_extAllocator;
#endif
#if defined (MFX_ENABLE_MFE)
        mfxExtMultiFrameParam    m_MfeParam;
        mfxExtMultiFrameControl  m_MfeControl;
#endif
        std::vector<mfxMVCViewDependency> m_storageView;
        std::vector<mfxMVCOperationPoint> m_storageOp;
        std::vector<mfxU16>               m_storageViewId;

    public:
        struct CalculableParam
        {
            mfxU32 bufferSizeInKB;
            mfxU32 initialDelayInKB;
            mfxU32 targetKbps;
            mfxU32 maxKbps;
            mfxU32 WinBRCMaxAvgKbps;

            mfxU32 numTemporalLayer;
            mfxU32 tid[8];
            mfxU32 scale[8];

// MVC BD {
            struct
            {
                mfxU32 bufferSizeInKB;
                mfxU32 initialDelayInKB;
                mfxU32 targetKbps;
                mfxU32 maxKbps;
                mfxU16 codecLevel;
            } mvcPerViewPar;
// MVC BD }
            mfxU32 numDependencyLayer;
            mfxU32 did[8];
            mfxU32 numLayersTotal;
            mfxU32 numLayerOffset[8];
            mfxU32 dqId1Exists;
            mfxU32 tempScalabilityMode;
            mfxU32 cqpHrdMode; // special CQP mode in which HRD information is written into bitstreams: 0 - disabled, 1 -CBR, 2 - VBR
            struct
            {
                mfxU32 bufferSizeInKB;
                mfxU32 initialDelayInKB;
                mfxU32 targetKbps;
                mfxU32 maxKbps;
            } decorativeHrdParam; // decorative HRD parameters for cqpHrdMode

            mfxU16 widthLa;
            mfxU16 heightLa;
            mfxU32 TCBRCTargetFrameSize;
            mfxU32 PPyrInterval;
        } calcParam;
    };

    bool   isSWBRC (MfxVideoParam const & par);
    bool   isAdaptiveQP(MfxVideoParam const & par);
    mfxU16 GetMaxNumSlices(MfxVideoParam const & par);

    mfxU16 GetNumSurfInput(MfxVideoParam const & video);

    mfxU8 GetNumReorderFrames(MfxVideoParam const & video);

    mfxU32 CalcNumSurfRaw(MfxVideoParam const & video);

    mfxU32 CalcNumSurfRecon(MfxVideoParam const & video);

    mfxU32 CalcNumSurfBitstream(MfxVideoParam const & video);

    mfxU32 CalcNumTasks(MfxVideoParam const & video);

    mfxI64 CalcDTSFromPTS(
        mfxFrameInfo const & info,
        mfxU16               dpbOutputDelay,
        mfxU64               timeStamp);

#define SCALE_FROM_DRIVER 4 // driver hardcodes scale 4. Need to use this value in MSDK. Otherwise BRC and MSDK HRD calculations will diverge.

    mfxU32 GetMaxBitrateValue(
        mfxU32 kbps,
        mfxU32 scale = SCALE_FROM_DRIVER);

    mfxU8 GetCabacInitIdc(mfxU32 targetUsage);

    bool IsLookAheadSupported(
        MfxVideoParam const & video,
        eMFXHWType            platform);
    bool IsMctfSupported(
        MfxVideoParam const & video);

    mfxU32 GetPPyrSize(
        MfxVideoParam const & video,
        mfxU32 miniGopSize,
        bool   bEncToolsLA);

    bool IsExtBrcSceneChangeSupported(
        MfxVideoParam const & video);

    bool IsCmNeededForSCD(
        MfxVideoParam const & video);

    bool IsAdaptiveLtrOn(
        MfxVideoParam const & video);

    mfxU8 DetermineQueryMode(mfxVideoParam * in);

    mfxStatus SetLowPowerDefault(
        MfxVideoParam& par,
        const eMFXHWType& platform);

    mfxStatus QueryHwCaps(
        VideoCORE *       core,
        MFX_ENCODE_CAPS & hwCaps,
        mfxVideoParam *   par);

    mfxStatus QueryMbProcRate(
        VideoCORE* core,
        mfxVideoParam const & out,
        mfxU32 (&mbPerSec)[16],
        const mfxVideoParam * in);

    mfxStatus QueryGuid(
        VideoCORE* core,
        GUID guid);

    mfxStatus ReadSpsPpsHeaders(MfxVideoParam & par);

    mfxU16 GetFrameWidth(MfxVideoParam & par);

    mfxU16 GetFrameHeight(MfxVideoParam & par);

    mfxStatus CorrectCropping(
        MfxVideoParam & par);

    bool IsRunTimeOnlyExtBuffer(
        mfxU32 id);

    bool IsRunTimeExtBufferIdSupported(
        MfxVideoParam const & video, mfxU32 id);

    bool IsRuntimeOutputExtBufferIdSupported(
        MfxVideoParam const & video, mfxU32 id);

    bool IsRunTimeExtBufferPairAllowed(
        MfxVideoParam const & video, mfxU32 id);

    bool IsRunTimeExtBufferPairRequired(
        MfxVideoParam const & video, mfxU32 id);

    bool IsVideoParamExtBufferIdSupported(
        mfxU32 id);

    mfxStatus CheckExtBufferId(
        mfxVideoParam const & par);


    mfxStatus CheckWidthAndHeight(
        MfxVideoParam const & par);

    mfxStatus CopySpsPpsToVideoParam(
        MfxVideoParam & par);

    mfxStatus CheckForAllowedH264SpecViolations(
        MfxVideoParam const & par);

    mfxStatus CheckAndFixRectQueryLike(
        MfxVideoParam const & par,
        mfxRectDesc *         rect);

    mfxStatus CheckAndFixOpenRectQueryLike(
        MfxVideoParam const & par,
        mfxRectDesc *         rect);

    mfxStatus CheckAndFixRoiQueryLike(
        MfxVideoParam const & par,
        mfxRoiDesc *          roi,
        mfxU16                roiMode);

    mfxStatus CheckAndFixMovingRectQueryLike(
        MfxVideoParam const & par,
        mfxMovingRectDesc *   rect);


    mfxStatus CheckVideoParam(
        MfxVideoParam &         par,
        MFX_ENCODE_CAPS const & hwCaps,
        bool                    setExtAlloc,
        eMFXHWType              platform = MFX_HW_UNKNOWN,
        eMFXVAType              vaType = MFX_HW_NO,
        eMFXGTConfig            config = MFX_GT_UNKNOWN,
        bool                    bInit = false);

    mfxStatus CheckVideoParamFEI(
        MfxVideoParam &     par);

    mfxStatus CheckVideoParamQueryLike(
        MfxVideoParam &         par,
        MFX_ENCODE_CAPS const & hwCaps,
        eMFXHWType              platform = MFX_HW_UNKNOWN,
        eMFXVAType              vaType = MFX_HW_NO,
        eMFXGTConfig            config = MFX_GT_UNKNOWN);

    mfxStatus CheckVideoParamMvcQueryLike(MfxVideoParam &     par);

    mfxStatus CheckMVCSeqDescQueryLike(mfxExtMVCSeqDesc * mvcSeqDesc);

    mfxStatus CheckAndFixMVCSeqDesc(mfxExtMVCSeqDesc * mvcSeqDesc, bool isViewOutput);

    void SetDefaults(
        MfxVideoParam &         par,
        MFX_ENCODE_CAPS const & hwCaps,
        bool                    setExtAlloc,
        eMFXHWType              platform = MFX_HW_UNKNOWN,
        eMFXVAType              vaType = MFX_HW_NO,
        eMFXGTConfig            config = MFX_GT_UNKNOWN);

    void InheritDefaultValues(
        MfxVideoParam const & parInit,
        MfxVideoParam &       parReset,
        MFX_ENCODE_CAPS const & hwCaps,
        mfxVideoParam const * parResetIn = 0);

    mfxStatus CheckPayloads(
        mfxPayload const * const * payload,
        mfxU16                     numPayload);

    mfxStatus CheckRunTimeExtBuffers(
        MfxVideoParam const & video,
        mfxEncodeCtrl *       ctrl,
        mfxFrameSurface1 *    surface,
        mfxBitstream *        bs,
        MFX_ENCODE_CAPS const &   caps,
        eMFXHWType            platform = MFX_HW_UNKNOWN);

    mfxStatus CheckFEIRunTimeExtBuffersContent(
        MfxVideoParam const & video,
        mfxEncodeCtrl *       ctrl,
        mfxFrameSurface1 *    surface,
        mfxBitstream *        bs);

    mfxStatus CheckRunTimePicStruct(
        mfxU16 runtPicStruct,
        mfxU16 initPicStruct);

    bool IsRecoveryPointSeiMessagePresent(
        mfxPayload const * const * payload,
        mfxU16                     numPayload,
        mfxU32                     payloadLayout);

    mfxStatus CopyFrameDataBothFields(
        VideoCORE *          core,
        mfxFrameData const & dst,
        mfxFrameData const & src,
        mfxFrameInfo const & info);

    mfxExtBuffer * GetExtBuffer(
        mfxExtBuffer ** extBuf,
        mfxU32          numExtBuf,
        mfxU32          id,
        mfxU32          offset = 0);
    struct mfxExtBufferProxy;
    struct mfxExtBufferRefProxy;
    template <typename T> mfxExtBufferProxy GetExtBuffer(const T & par, mfxU32 fieldId = 0);
    template <typename T> mfxExtBufferProxy GetExtBufferFEI(const T & bs, mfxU32 fieldId = 0);
    template <typename T> mfxExtBufferRefProxy GetExtBufferRef(const T & par, mfxU32 fieldId = 0);

    struct mfxExtBufferProxy
    {
    public:
        template <typename T> operator T()
        {
            mfxExtBuffer * p = MfxHwH264Encode::GetExtBuffer(
                m_extParam,
                m_numExtParam,
                ExtBufTypeToId<typename GetPointedType<T>::Type>::id,
                m_fieldId);
            return reinterpret_cast<T>(p);
        }

        template <typename T> friend mfxExtBufferProxy GetExtBuffer(const T & par, mfxU32 fieldId)
        {
            return mfxExtBufferProxy(par.ExtParam, par.NumExtParam, fieldId);
        }
        template <typename T> friend mfxExtBufferProxy GetExtBufferFEI(const T & bs, mfxU32 fieldId)
        {
            return mfxExtBufferProxy(bs->ExtParam, bs->NumExtParam, fieldId);
        }

    protected:
        mfxExtBufferProxy(mfxExtBuffer ** extParam, mfxU32 numExtParam,  mfxU32 fieldId = 0)
            : m_extParam(extParam)
            , m_numExtParam(numExtParam)
            , m_fieldId(fieldId)
        {
        }

    private:
        mfxExtBuffer ** m_extParam;
        mfxU32          m_numExtParam;
        mfxU32          m_fieldId;
    };

    // version with assert, special for extract extBuffer from wrapper MfxVideoParam
    struct mfxExtBufferRefProxy{
    public:
        template <typename T> operator T&()
        {
            mfxExtBuffer * p = MfxHwH264Encode::GetExtBuffer(
                m_extParam,
                m_numExtParam,
                ExtBufTypeToId<typename GetPointedType<T*>::Type>::id,
                m_fieldId);
            assert(p);
            return *(reinterpret_cast<T*>(p));
        }

        // Intention of GetExtBufferRef is to get ext buffers
        // from MfxVideoParam structure which always has full set
        // of extension buffers ("MfxHwH264Encode::GetExtBuffer" can't return zero).
        //
        // So, for MfxVideoParam it's better to use function GetExtBufferRef()
        // instead GetExtBuffer(), we can wrap issues from static code analyze
        // to one place (body of function GetExtBufferRef()) instead several
        // places (every calling GetExtBuffer())
        template <typename T> friend mfxExtBufferRefProxy GetExtBufferRef(const T & par, mfxU32 fieldId)
        {
            return mfxExtBufferRefProxy(par.ExtParam, par.NumExtParam, fieldId);
        }

    protected:
        mfxExtBufferRefProxy(mfxExtBuffer ** extParam, mfxU32 numExtParam,  mfxU32 fieldId = 0)
            : m_extParam(extParam)
            , m_numExtParam(numExtParam)
            , m_fieldId(fieldId)
        {
        }

    private:
        mfxExtBuffer ** m_extParam;
        mfxU32          m_numExtParam;
        mfxU32          m_fieldId;
    };

    inline bool IsFieldCodingPossible(MfxVideoParam const & par)
    {
        return (par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) == 0;
    }

    inline void ResetNumSliceIPB(mfxVideoParam &par)
    {
        mfxExtCodingOption3 * extOpt3 = GetExtBuffer(par);
        if(extOpt3 != nullptr && (extOpt3->NumSliceI == 0 || extOpt3->NumSliceP == 0 || extOpt3->NumSliceB == 0)){
            extOpt3->NumSliceI = extOpt3->NumSliceP = extOpt3->NumSliceB = par.mfx.NumSlice;
        }
    }

    inline mfxU32 GetAvgFrameSizeInBytes(const MfxVideoParam &  par, bool bMaxKbps)
    {
        return mfxU32(1000.0 / 8.0*((par.mfx.MaxKbps && bMaxKbps) ? par.mfx.MaxKbps : par.mfx.TargetKbps) * std::max<mfxU32>(par.mfx.BRCParamMultiplier,1) /
            (mfxF64(par.mfx.FrameInfo.FrameRateExtN) / par.mfx.FrameInfo.FrameRateExtD) );
    }
    inline bool IsTCBRC(const MfxVideoParam &  par, MFX_ENCODE_CAPS const & hwCaps)
    {
        mfxExtCodingOption3 &extOpt3 = GetExtBufferRef(par);
        mfxExtCodingOption  &extOpt = GetExtBufferRef(par);
        return (IsOn(extOpt3.LowDelayBRC) && (hwCaps.ddi_caps.TCBRCSupport) && IsOff(extOpt.NalHrdConformance) &&
               (par.mfx.RateControlMethod  ==  MFX_RATECONTROL_VBR  ||
                par.mfx.RateControlMethod  ==  MFX_RATECONTROL_QVBR ||
                par.mfx.RateControlMethod  ==  MFX_RATECONTROL_VCM ));
    }

    inline mfxU8 GetPayloadLayout(mfxU32 fieldPicFlag, mfxU32 secondFieldPicFlag)
    {
        return fieldPicFlag == 0
            ? 0 /* MFX_PAYLOAD_LAYOUT_ALL */
            : secondFieldPicFlag == 0
                ? 1 /* MFX_PAYLOAD_LAYOUT_EVEN */
                : 2 /* MFX_PAYLOAD_LAYOUT_ODD */;
    }

    inline mfxU8 GetPayloadLayout(const mfxFrameParam& fp)
    {
        return GetPayloadLayout(fp.AVC.FieldPicFlag, fp.AVC.SecondFieldPicFlag);
    }

    inline bool IsDriverSliceSizeControlEnabled(const MfxVideoParam & par, MFX_ENCODE_CAPS const & hwCaps)
    {
        return IsOn(par.mfx.LowPower) && hwCaps.ddi_caps.SliceLevelRateCtrl;
    }

    mfxU8 ConvertFrameTypeMfx2Ddi(mfxU32 type);

    mfxU8 ConvertMfxFrameType2SliceType(mfxU8 type);

    ENCODE_FRAME_SIZE_TOLERANCE ConvertLowDelayBRCMfx2Ddi(mfxU16 type, bool bTCBRC);

    enum class SliceDividerType
    {
        ONESLICE            = 0, // Once slice for the whole frame
        ROW2ROW             = 1, // Slices are power of 2 number of rows, all slices the same
        ROWSLICE            = 2, // Slices are any number of rows, all slices the same
        ARBITRARY_ROW_SLICE = 3, // Slices are any number of rows, slices can be different
        ARBITRARY_MB_SLICE  = 4, // Slices are any number of MBs, slices can be different
    };

    struct MfxMemId
    {
    public:
        MfxMemId()
            : mid(0)
            , external(false)
        {}

        MfxMemId(mfxMemId mid_, bool external_)
            : mid(mid_)
            , external(external_)
        {}

        mfxMemId mid;
        bool     external;
    };

    inline mfxU32 CeilLog2(mfxU32 val)
    {
        mfxU32 res = 0;

        while (val)
        {
            val >>= 1;
            res++;
        }

        return res;
    }

    inline mfxU32 ExpGolombCodeLength(mfxU32 val)
    {
        return
            CeilLog2(val + 1) * 2 - 1;
    }

    // auto-lock for frames
    struct FrameLocker
    {
        FrameLocker(VideoCORE * core, mfxFrameData & data, bool external = false)
            : m_core(core)
            , m_data(data)
            , m_memId(data.MemId)
            , m_status(Lock(external))
        {
        }

        FrameLocker(VideoCORE * core, mfxFrameData & data, mfxMemId memId, bool external = false)
            : m_core(core)
            , m_data(data)
            , m_memId(memId)
            , m_status(Lock(external))
        {
        }

        ~FrameLocker() { Unlock(); }

        mfxStatus Unlock()
        {
            mfxStatus mfxSts = MFX_ERR_NONE;

            if (m_status == LOCK_INT)
                mfxSts = m_core->UnlockFrame(m_memId, &m_data);
            else if (m_status == LOCK_EXT)
                mfxSts = m_core->UnlockExternalFrame(m_memId, &m_data);

            m_status = LOCK_NO;
            return mfxSts;
        }


        enum { LOCK_NO, LOCK_INT, LOCK_EXT };

        mfxU32 Lock(bool external)
        {
            mfxU32 status = LOCK_NO;

            if (m_data.Y == 0)
            {
                status = external
                    ? (m_core->LockExternalFrame(m_memId, &m_data) == MFX_ERR_NONE ? LOCK_EXT : LOCK_NO)
                    : (m_core->LockFrame(m_memId, &m_data) == MFX_ERR_NONE ? LOCK_INT : LOCK_NO);
            }

            return status;
        }

    private:
        FrameLocker(FrameLocker const &);
        FrameLocker & operator =(FrameLocker const &);

        VideoCORE *    m_core;
        mfxFrameData & m_data;
        mfxMemId       m_memId;
        mfxU32         m_status;
    };

    class FileHandle
    {
    public:
        FileHandle(FILE * file = 0)
            : m_file(file)
        {
        }

        ~FileHandle()
        {
            Close();
        }

        void Close()
        {
            if (m_file != 0)
            {
                fclose(m_file);
            }
        }

        FileHandle & operator =(FILE* file)
        {
            Close();
            m_file = file;
            return *this;
        }

        operator FILE *()
        {
            return m_file;
        }

    private:
        FileHandle(FileHandle const &); // no impl
        FileHandle & operator =(FileHandle const &); // no impl
        FILE* m_file;
    };

    template <class T>
    class ScopedArray
    {
    public:
        ScopedArray(T * arr = 0)
            : m_arr(arr)
        {
        }

        ~ScopedArray()
        {
            reset(0);
        }

        T * reset(T * arr)
        {
            T * tmp(m_arr);
            delete [] m_arr;
            m_arr = arr;
            return tmp;
        }

        T * cptr()
        {
            return m_arr;
        }

        T const * cptr() const
        {
            return m_arr;
        }

        T & operator [](size_t idx)
        {
            assert(m_arr);
            return m_arr[idx];
        }

        T const & operator [](size_t idx) const
        {
            assert(m_arr);
            return m_arr[idx];
        }

    private:
        ScopedArray(ScopedArray const &);
        ScopedArray & operator =(ScopedArray const &);

        T * m_arr;
    };

    struct SliceDividerState
    {
        mfxU32 m_numSlice;
        mfxU32 m_numMbInRow;
        mfxU32 m_numMbRow;
        mfxU32 m_leftSlice;
        mfxU32 m_leftMbRow;
        mfxU32 m_currSliceFirstMbRow;
        mfxU32 m_currSliceNumMbRow;
    };

    struct SliceDivider : protected SliceDividerState
    {
        SliceDivider() : m_pfNext(0) {}

        // 'False' means no more slices, object needs to be recreated
        bool Next() { return m_pfNext(*this); }

        mfxU32 GetFirstMbInSlice() const;

        mfxU32 GetNumMbInSlice() const;

        mfxU32 GetNumSlice() const;

    protected:
        bool (*m_pfNext)(SliceDividerState & state);
    };


    struct SliceDividerOneSlice : SliceDivider
    {
        SliceDividerOneSlice(
            mfxU32 numSlice,
            mfxU32 widthInMbs,
            mfxU32 heightInMbs);

        static bool Next(SliceDividerState & state);
    };


    struct SliceDividerArbitraryRowSlice : SliceDivider
    {
        SliceDividerArbitraryRowSlice(
            mfxU32 numSlice,
            mfxU32 widthInMbs,
            mfxU32 heightInMbs);

        static bool Next(SliceDividerState & state);
    };


    struct SliceDividerRow2Row : SliceDivider
    {
        SliceDividerRow2Row(
            mfxU32 numSlice,
            mfxU32 widthInMbs,
            mfxU32 heightInMbs);

        static bool Next(SliceDividerState & state);

        static void Test();
    };


    struct SliceDividerRowSlice : SliceDivider
    {
        SliceDividerRowSlice(
            mfxU32 numSlice,
            mfxU32 widthInMbs,
            mfxU32 heightInMbs);

        static bool Next(SliceDividerState & state);
    };

    struct SliceDividerTemporalScalability : SliceDivider
    {
        SliceDividerTemporalScalability(
            mfxU32 sliceSizeInMbs,
            mfxU32 widthInMbs,
            mfxU32 heightInMbs);

        static bool Next(SliceDividerState & state);
    };
    struct SliceDividerLowPower : SliceDivider
    {
        SliceDividerLowPower(
            mfxU32 numSlice,
            mfxU32 widthInMbs,
            mfxU32 heightInMbs);

        static bool Next(SliceDividerState & state);
    };
    struct SliceDividerLowPowerTemporalScalability : SliceDivider
    {
        SliceDividerLowPowerTemporalScalability(
            mfxU32 sliceSizeInMbs,
            mfxU32 widthInMbs,
            mfxU32 heightInMbs);

        static bool Next(SliceDividerState & state);
    };
    SliceDivider MakeSliceDivider(
        SliceDividerType sliceHwCaps,
        mfxU32  sliceSizeInMbs,
        mfxU32  numSlice,
        mfxU32  widthInMbs,
        mfxU32  heightInMbs,
        bool isLowPower = false);


    class AspectRatioConverter
    {
    public:
        AspectRatioConverter(mfxU16 sarw, mfxU16 sarh);
        AspectRatioConverter(mfxU8 sarIdc, mfxU16 sarw, mfxU16 sarh);
        mfxU8  GetSarIdc()    const { return m_sarIdc; }
        mfxU16 GetSarWidth()  const { return m_sarWidth; }
        mfxU16 GetSarHeight() const { return m_sarHeight; }

    private:
        mfxU8  m_sarIdc;
        mfxU16 m_sarWidth;
        mfxU16 m_sarHeight;
    };

    class EndOfBuffer : public std::exception
    {
    public:
        EndOfBuffer() : std::exception() {}
    };

    class InvalidBitstream : public std::exception
    {
    public:
        InvalidBitstream() : std::exception() {}
    };

    void ReadSpsHeader(
        InputBitstream &  reader,
        mfxExtSpsHeader & sps);

    mfxU8 ReadSpsIdOfPpsHeader(
        InputBitstream reader);

    void ReadPpsHeader(
        InputBitstream &        reader,
        mfxExtSpsHeader const & sps,
        mfxExtPpsHeader &       pps);

    mfxU32 WriteSpsHeader(
        OutputBitstream &       writer,
        mfxExtSpsHeader const & sps);

    mfxU32 WriteSpsHeader(
        OutputBitstream &        writer,
        mfxExtSpsHeader const &  sps,
        mfxExtBuffer const &     spsExt);

    mfxU32 WritePpsHeader(
        OutputBitstream &       writer,
        mfxExtPpsHeader const & pps);

    void WriteScalingList(
        OutputBitstream &       writer,
        const mfxU8* scalingList,
        mfxI32 sizeOfScalingList);

    mfxU32 WriteAud(
        OutputBitstream & writer,
        mfxU32            frameType);

    struct RefListMod;
    struct WeightTab;
    template<class T, mfxU32 N> struct FixedArray;
    typedef FixedArray<RefListMod, 32> ArrayRefListMod;
    typedef FixedArray<WeightTab, 32> ArrayWeightTab;

    void WriteRefPicListModification(
        OutputBitstream &       writer,
        ArrayRefListMod const & refListMod);

    struct DecRefPicMarkingInfo;

    void WriteDecRefPicMarking(
        OutputBitstream &            writer,
        DecRefPicMarkingInfo const & marking,
        mfxU32                       idrPicFlag);

    bool IsAvcProfile(mfxU32 profile);

    bool IsAvcBaseProfile(mfxU32 profile);

    bool IsAvcHighProfile(mfxU32 profile);

    bool IsMvcProfile(mfxU32 profile);


    inline mfxStatus Error(mfxStatus sts)
    {
        return sts;
    }

    inline mfxStatus Warning(mfxStatus sts)
    {
        //__asm { int 3 }
        return sts;
    }

    mfxU8 * PackPrefixNalUnitSvc(
        mfxU8 *         begin,
        mfxU8 *         end,
        bool            emulationControl,
        DdiTask const & task,
        mfxU32          fieldId,
        mfxU32          nalUnitType = 0xe);

    class HeaderPacker
    {
    public:
        void Init(
            MfxVideoParam const &     par,
            MFX_ENCODE_CAPS const &   hwCaps,
            bool                      emulPrev = true); // insert emualtion prevention bytes when possible (sps/pps/sei/aud)

        std::vector<ENCODE_PACKEDHEADER_DATA> const & PackSlices(
            DdiTask const & task,
            mfxU32          fieldId);



        ENCODE_PACKEDHEADER_DATA const & PackSkippedSlice(
            DdiTask const & task,
            mfxU32          fieldId);

        ENCODE_PACKEDHEADER_DATA const & PackAud(
            DdiTask const & task,
            mfxU32          fieldId);


        ENCODE_PACKEDHEADER_DATA const & GetAud() const { return m_packedAud; }

        std::vector<ENCODE_PACKEDHEADER_DATA> const & GetSps() const { return m_packedSps; }

        std::vector<ENCODE_PACKEDHEADER_DATA> const & GetPps() const { return m_packedPps; }

        std::vector<ENCODE_PACKEDHEADER_DATA> const & GetSlices() const { return m_packedSlices; }

        bool isMVC() const { return m_isMVC; }
        bool isSvcPrefixUsed() const { return m_needPrefixNalUnit; }

        void ResizeSlices(mfxU32 num);

#ifndef MFX_AVC_ENCODING_UNIT_DISABLE
        void GetHeadersInfo(std::vector<mfxEncodedUnitInfo> &HeadersMap, DdiTask const& task, mfxU32 fid);
#endif

    private:

        mfxU32 WriteSlice(
            OutputBitstream & obs,
            DdiTask const &   task,
            mfxU32            fieldId,
            mfxU32            sliceId);

        mfxU32 WriteSlice(
            OutputBitstream & obs,
            DdiTask const &   task,
            mfxU32            fieldId,
            mfxU32            firstMbInSlice,
            mfxU32            numMbInSlice);

        // for header packing
        std::vector<mfxExtSpsHeader>    m_sps;
        std::vector<mfxExtPpsHeader>    m_pps;
        MFX_ENCODE_CAPS                 m_hwCaps;
        mfxU8                           m_spsIdx[8][16];            // for lookup by did & qid
        mfxU8                           m_ppsIdx[8][16];            // for lookup by did & qid
        mfxU8                           m_refDqId[8];               // for lookup by did
        mfxU8                           m_simulcast[8];             // for lookup by did
        mfxU16                          m_cabacInitIdc;             // same for all layers and slices
        mfxU16                          m_directSpatialMvPredFlag;  // same for all layers and slices
        mfxU16                          m_numMbPerSlice;
        bool                            m_needPrefixNalUnit;
        bool                            m_emulPrev;                 // insert emualtion prevention bytes when possible (sps/pps/sei/aud)
        bool                            m_isMVC;
        bool                            m_longStartCodes;
        bool                            m_isLowPower;

        ENCODE_PACKEDHEADER_DATA                m_packedAud;
        std::vector<ENCODE_PACKEDHEADER_DATA>   m_packedSps;
        std::vector<ENCODE_PACKEDHEADER_DATA>   m_packedPps;
        std::vector<ENCODE_PACKEDHEADER_DATA>   m_packedSlices;
        std::vector<mfxU8>                      m_headerBuffer;
        std::vector<mfxU8>                      m_sliceBuffer;

        static const mfxU32 SPSPPS_BUFFER_SIZE = 1024;
        static const mfxU32 SLICE_BUFFER_SIZE  = 2048;
    };

    inline mfxU16 LaDSenumToFactor(const mfxU16& LookAheadDS)
    {
        switch (LookAheadDS)
        {
        case MFX_LOOKAHEAD_DS_UNKNOWN:
            return 0;
        case MFX_LOOKAHEAD_DS_OFF:
            return 1;
        case MFX_LOOKAHEAD_DS_2x :
            return 2;
        case MFX_LOOKAHEAD_DS_4x :
            return 4;
        default:
            assert(0);
            return LookAheadDS;
        }
    }
};

#endif // MFX_ENABLE_H264_VIDEO_..._HW
#endif // __MFX_H264_ENC_COMMON_HW_H__
