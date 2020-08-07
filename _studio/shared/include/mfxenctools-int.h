/******************************************************************************* *\

Copyright (C) 2019-2020 Intel Corporation.  All rights reserved.

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

File Name: mfxenctools-int.h

*******************************************************************************/
#ifndef __MFXENCTOOLS_INT_H__
#define __MFXENCTOOLS_INT_H__

#include "mfx_config.h"
#include "mfxvideo++.h"
#include "mfxbrc.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Extended Buffer Ids */
enum {
    MFX_EXTBUFF_ENCTOOLS_CONFIG = MFX_MAKEFOURCC('E', 'T', 'C', 'F'),
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef union {
    struct {
        mfxU8    Minor;
        mfxU8    Major;
    };
    mfxU16    Version;
} mfxStructVersion;
MFX_PACK_END()

#define MFX_STRUCT_VERSION(MAJOR, MINOR) (256*(MAJOR) + (MINOR))


MFX_PACK_BEGIN_USUAL_STRUCT()
    typedef struct
{
    mfxExtBuffer      Header;
    mfxStructVersion  Version;
    mfxU16            AdaptiveI;
    mfxU16            AdaptiveB;
    mfxU16            AdaptiveRefP;
    mfxU16            AdaptiveRefB;
    mfxU16            SceneChange;
    mfxU16            AdaptiveLTR;
    mfxU16            AdaptivePyramidQuantP;
    mfxU16            AdaptivePyramidQuantB;
    mfxU16            AdaptiveQuantMatrices;
    mfxU16            BRCBufferHints;
    mfxU16            BRC;
    mfxU16            reserved[20];
} mfxExtEncToolsConfig;
MFX_PACK_END()

#define MFX_ENCTOOLS_CONFIG_VERSION MFX_STRUCT_VERSION(1, 0)



/* Extended Buffer Ids */
enum {
    MFX_EXTBUFF_ENCTOOLS = MFX_MAKEFOURCC('E', 'E', 'T', 'L'),
    MFX_EXTBUFF_ENCTOOLS_DEVICE = MFX_MAKEFOURCC('E', 'T', 'E', 'D'),
    MFX_EXTBUFF_ENCTOOLS_ALLOCATOR = MFX_MAKEFOURCC('E', 'T', 'E', 'A'),
    MFX_EXTBUFF_ENCTOOLS_FRAME_TO_ANALYZE = MFX_MAKEFOURCC('E', 'F', 'T', 'A'),
    MFX_EXTBUFF_ENCTOOLS_HINT_SCENE_CHANGE = MFX_MAKEFOURCC('E', 'H', 'S', 'C'),
    MFX_EXTBUFF_ENCTOOLS_HINT_GOP = MFX_MAKEFOURCC('E', 'H', 'G', 'O'),
    MFX_EXTBUFF_ENCTOOLS_HINT_AREF = MFX_MAKEFOURCC('E', 'H', 'A', 'R'),
    MFX_EXTBUFF_ENCTOOLS_BRC_BUFFER_HINT = MFX_MAKEFOURCC('E', 'B', 'B', 'H'),
    MFX_EXTBUFF_ENCTOOLS_BRC_FRAME_PARAM = MFX_MAKEFOURCC('E', 'B', 'F', 'P'),
    MFX_EXTBUFF_ENCTOOLS_BRC_QUANT_CONTROL = MFX_MAKEFOURCC('E', 'B', 'Q', 'C'),
    MFX_EXTBUFF_ENCTOOLS_BRC_HRD_POS = MFX_MAKEFOURCC('E', 'B', 'H', 'P'),
    MFX_EXTBUFF_ENCTOOLS_BRC_ENCODE_RESULT = MFX_MAKEFOURCC('E', 'B', 'E', 'R'),
    MFX_EXTBUFF_ENCTOOLS_BRC_STATUS = MFX_MAKEFOURCC('E', 'B', 'S', 'T'),
    MFX_EXTBUFF_ENCTOOLS_HINT_MATRIX = MFX_MAKEFOURCC('E', 'H', 'Q', 'M')
};

#define MFX_ENCTOOLS_CONFIG_VERSION MFX_STRUCT_VERSION(1, 0)

enum
{
    MFX_BRC_NO_HRD = 0,
    MFX_BRC_HRD_WEAK,  /* IF HRD CALCULATION IS REQUIRED, BUT NOT WRITTEN TO THE STREAM */
    MFX_BRC_HRD_STRONG,
};

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct
{
    mfxStructVersion Version;
    mfxU16  reserved[3];

    /* info about codec */
    struct  /* coding info*/
    {
        mfxU32  CodecId;
        mfxU16  CodecProfile;
        mfxU16  CodecLevel;
        mfxU16  reserved2[64];
    };
    struct      /* input frames info */
    {
        /* info about input frames */
        mfxFrameInfo FrameInfo;
        mfxU16  IOPattern;
        mfxU16 MaxDelayInFrames;
        mfxU16  reserved3[64];
    };
    struct
    {
        /* Gop limitations */
        mfxU16  MaxGopSize;
        mfxU16  MaxGopRefDist;
        mfxU16  MaxIDRDist;
        mfxU16  BRefType;
        mfxU16  reserved4[64];
    };
    mfxU16 ScenarioInfo;

    struct  /* Rate control info */
    {
        mfxU16  RateControlMethod;          /* CBR, VBR, CRF, CQP */
        mfxU32  TargetKbps;                 /* ignored for CRF, CQP */
        mfxU32  MaxKbps;                    /* ignored for CRF, CQP */
        mfxU16  QPLevel[3];                 /* for CRF, CQP */

        mfxU16  HRDConformance;             /* for CBR & VBR */
        mfxU32  BufferSizeInKB;             /* if HRDConformance is ON */
        mfxU32  InitialDelayInKB;           /* if HRDConformance is ON */

        mfxU32  ConvergencePeriod;          /* if HRDConformance is OFF, 0 - the period is whole stream */
        mfxU32  Accuracy;                   /* if HRDConformance is OFF */

        mfxU32  WinBRCMaxAvgKbps;           /* sliding window restriction */
        mfxU16  WinBRCSize;                 /* sliding window restriction */

        mfxU32  MaxFrameSizeInBytes[3];     /* MaxFrameSize limitation */

        mfxU16  MinQPLevel[3];              /* QP range  limitations */
        mfxU16  MaxQPLevel[3];              /* QP range  limitations */

        mfxU32  PanicMode;
        mfxU16  reserved5[64];
    };
    mfxU16 NumExtParam;
    mfxExtBuffer** ExtParam;
} mfxEncToolsCtrl;
MFX_PACK_END()

#define MFX_ENCTOOLS_CTRL_VERSION MFX_STRUCT_VERSION(1, 0)

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxExtBuffer      Header;
    mfxStructVersion  Version;
    mfxU16            reserved[3];
    mfxHDL            DeviceHdl;
    mfxU32            HdlType;
    mfxU32            reserved2[3];
} mfxEncToolsCtrlExtDevice;
MFX_PACK_END()

#define MFX_ENCTOOLS_CTRL_EXTDEVICE_VERSION MFX_STRUCT_VERSION(1, 0)

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxExtBuffer       Header;
    mfxStructVersion   Version;
    mfxU16             reserved[3];
    mfxFrameAllocator *pAllocator;
    mfxU32             reserved2[4];
} mfxEncToolsCtrlExtAllocator;
MFX_PACK_END()

#define MFX_ENCTOOLS_CTRL_EXTALLOCATOR_VERSION MFX_STRUCT_VERSION(1, 0)

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxStructVersion  Version;
    mfxU16            reserved;
    mfxU32            DisplayOrder;
    mfxExtBuffer    **ExtParam;
    mfxU16            NumExtParam;
    mfxU16            reserved2;
    mfxU32            reserved3[3];
} mfxEncToolsTaskParam;
MFX_PACK_END()

#define MFX_ENCTOOLS_TASKPARAM_VERSION MFX_STRUCT_VERSION(1, 0)

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct
{
    mfxExtBuffer      Header;
    mfxStructVersion  Version;
    mfxU16            reserved[3];
    mfxFrameSurface1 *Surface;        /* Input surface */
    mfxU32            reserved2[2];
} mfxEncToolsFrameToAnalyze;
MFX_PACK_END()

#define MFX_ENCTOOLS_FRAMETOANALYZE_VERSION MFX_STRUCT_VERSION(1, 0)

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer      Header;
    mfxStructVersion  Version;
    mfxU16            reserved[3];
    mfxU16            SceneChangeFlag;
    mfxU16            RepeatedFrameFlag;
    mfxU32            TemporalComplexity;
    mfxU32            reserved2[2];
} mfxEncToolsHintPreEncodeSceneChange;
MFX_PACK_END()

#define MFX_ENCTOOLS_HINTPREENCODE_SCENECHANGE_VERSION MFX_STRUCT_VERSION(1, 0)

enum
{
    MFX_QUANT_MATRIX_DEFAULT = 0,
    MFX_QUANT_MATRIX_FLAT,
    MFX_QUANT_MATRIX_HIGH_FREQUENCY_STRONG
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer     Header;
    mfxStructVersion Version;
    mfxU16           reserved[2];
    mfxU16           MatrixType; /* enum */
} mfxEncToolsHintQuantMatrix;
MFX_PACK_END()

#define MFX_ENCTOOLS_HINT_QUANTMATRIX_VERSION MFX_STRUCT_VERSION(1, 0)

#define MFX_QP_UNDEFINED 0x1111

enum
{
    MFX_QP_MODULATION_NOT_DEFINED = 0,
    MFX_QP_MODULATION_LOW,
    MFX_QP_MODULATION_MEDIUM,
    MFX_QP_MODULATION_HIGH
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer      Header;
    mfxStructVersion  Version;
    mfxU16            reserved[2];
    mfxU16            FrameType;
    mfxI16            QPDelta;
    mfxU16            QPModulation; /* enum */
    mfxU16            MiniGopSize;  /* Adaptive GOP decision for the frame */
    mfxU16            reserved2[5];
} mfxEncToolsHintPreEncodeGOP;
MFX_PACK_END()

#define MFX_ENCTOOLS_HINTPREENCODE_GOP_VERSION MFX_STRUCT_VERSION(1, 0)

enum
{
    MFX_REF_FRAME_NORMAL = 0,
    MFX_REF_FRAME_TYPE_LTR,
    MFX_REF_FRAME_TYPE_KEY
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {                               /* only for progressive. Need to think about interlace for future support */
    mfxExtBuffer      Header;
    mfxStructVersion  Version;
    mfxU16            reserved[2];
    mfxU16            CurrFrameType;           /* enum */
    mfxU32            PreferredRefList[16];
    mfxU32            RejectedRefList[16];
    mfxU16            PreferredRefListSize;
    mfxU16            RejectedRefListSize;
    mfxU16            reserved2[7];
} mfxEncToolsHintPreEncodeARefFrames;         /* Output structure */
MFX_PACK_END()

#define MFX_ENCTOOLS_HINTPREENCODE_AREFFRAMES_VERSION MFX_STRUCT_VERSION(1, 0)

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer      Header;
    mfxStructVersion  Version;
    mfxU16            reserved[3];
    mfxU32            OptimalFrameSizeInBytes;
/*  mfxU32            OptimalBufferFullness; */ /* Target buffer fullness in bits, currently can be calculated from OptimalFrameSizeInBytes */
    mfxU32            reserved2[7];
} mfxEncToolsBRCBufferHint;
MFX_PACK_END()

#define MFX_ENCTOOLS_BRCBUFFER_HINT_VERSION MFX_STRUCT_VERSION(1, 0)

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer      Header;
    mfxStructVersion  Version;
    mfxU16            reserved[3];
    mfxU16            FrameType;       /* See FrameType enumerator */
    mfxU16            PyramidLayer;    /* B-pyramid or P-pyramid layer frame belongs to */
    mfxU32            EncodeOrder;     /* Frame number in a sequence of reordered frames starting from encoder Init() */
    mfxU32            reserved1[2];
} mfxEncToolsBRCFrameParams;
MFX_PACK_END()

#define MFX_ENCTOOLS_BRCFRAMEPARAMS_VERSION MFX_STRUCT_VERSION(1, 0)

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer      Header;
    mfxStructVersion  Version;
    mfxU16            reserved[3];
    mfxU32            QpY;             /* Frame-level Luma QP. Mandatory */
    mfxU32            MaxFrameSize;    /* Max frame size in bytes (used for rePak). Optional */
    mfxU8             DeltaQP[8];      /* deltaQP[i] is added to QP value while ith-rePakOptional */
    mfxU16            NumDeltaQP;      /* Max number of rePaks to provide MaxFrameSize (from 0 to 8) */
    mfxU16            reserved2[3];
} mfxEncToolsBRCQuantControl;
MFX_PACK_END()

#define MFX_ENCTOOLS_BRCQUANTCONTROL_VERSION MFX_STRUCT_VERSION(1, 0)

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer      Header;
    mfxStructVersion  Version;
    mfxU16            reserved[3];
    mfxU32            InitialCpbRemovalDelay;
    mfxU32            InitialCpbRemovalDelayOffset;
    mfxU32            reserved2[2];
} mfxEncToolsBRCHRDPos;
MFX_PACK_END()

#define MFX_ENCTOOLS_BRCHRDPOS_VERSION MFX_STRUCT_VERSION(1, 0)

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer      Header;
    mfxStructVersion  Version;
    mfxU16            reserved[3];
    mfxU16            NumRecodesDone;  /* Number of recodings performed for this frame. Optional */
    mfxU16            QpY;             /* Luma QP the frame was encoded with */
    mfxU32            CodedFrameSize;  /* Size of frame in bytes after encoding */
    mfxU32            reserved2[2];
} mfxEncToolsBRCEncodeResult;
MFX_PACK_END()

#define MFX_ENCTOOLS_BRCENCODERESULT_VERSION MFX_STRUCT_VERSION(1, 0)

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer       Header;
    mfxStructVersion   Version;
    mfxU16             reserved[3];
    mfxBRCFrameStatus  FrameStatus;
} mfxEncToolsBRCStatus;
MFX_PACK_END()

#define MFX_ENCTOOLS_BRCSTATUS_VERSION MFX_STRUCT_VERSION(1, 0)

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxExtBuffer      Header;
    mfxStructVersion  Version;           /* what about to return version of EncTools containing commit_id – return through GetVersion? */
    mfxU16            reserved[3];       /* to align with Version */
    mfxU32            reserved2[14];
    mfxHDL            Context;
    mfxStatus(MFX_CDECL *Init)   (mfxHDL pthis, mfxExtEncToolsConfig*  config, mfxEncToolsCtrl* ctrl);
    mfxStatus(MFX_CDECL *GetSupportedConfig)     (mfxHDL pthis, mfxExtEncToolsConfig*  config, mfxEncToolsCtrl* ctrl);
    mfxStatus(MFX_CDECL *GetActiveConfig)  (mfxHDL pthis, mfxExtEncToolsConfig* pEncToolConfig);
    mfxStatus(MFX_CDECL *GetDelayInFrames)(mfxHDL pthis, mfxExtEncToolsConfig* pEncToolConfig, mfxEncToolsCtrl* ctrl, mfxU32 *numFrames);
    mfxStatus(MFX_CDECL *Reset)  (mfxHDL pthis, mfxExtEncToolsConfig* pEncToolConfig, mfxEncToolsCtrl* ctrl); /* asynchronous reset */
    mfxStatus(MFX_CDECL *Close)  (mfxHDL pthis);
    mfxStatus(MFX_CDECL *Submit) (mfxHDL pthis, mfxEncToolsTaskParam* sparam); /* Encoder knows how many buffers should be provided by app/encode and it waits for all of them */
    mfxStatus(MFX_CDECL *Query)  (mfxHDL pthis, mfxEncToolsTaskParam* param, mfxU32 timeout);
    mfxStatus(MFX_CDECL *Discard) (mfxHDL pthis, mfxU32 DisplayOrder);
    mfxHDL            reserved3[8];
} mfxEncTools;
MFX_PACK_END()

#define MFX_ENCTOOLS_VERSION MFX_STRUCT_VERSION(1, 0)

mfxEncTools*  MFX_CDECL MFXVideoENCODE_CreateEncTools();
void  MFX_CDECL MFXVideoENCODE_DestroyEncTools(mfxEncTools *et);



#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif

