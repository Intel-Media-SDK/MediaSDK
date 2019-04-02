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

#include "umc_defs.h"

#if defined (MFX_ENABLE_VC1_VIDEO_DECODE)

#ifndef __UMC_VC1_COMMON_DEFS_H__
#define __UMC_VC1_COMMON_DEFS_H__

#include <stdio.h>
#include "umc_vc1_common_macros_defs.h"
#include "umc_structures.h"

#define VC1_ENC_RECODING_MAX_NUM 3
#define START_CODE_NUMBER 600
//frame header size in case of SM profiles
#define  VC1FHSIZE 8
//start code size in case of Advance profile
#define  VC1SCSIZE 4

typedef enum
{
    VC1_OK   = 0,
    VC1_FAIL = UMC::UMC_ERR_FAILED,
    VC1_SKIP_FRAME = 2,
    VC1_NOT_ENOUGH_DATA = UMC::UMC_ERR_NOT_ENOUGH_DATA,
    VC1_ERR_INVALID_STREAM   = UMC::UMC_ERR_INVALID_STREAM,
    VC1_WRN_INVALID_STREAM = UMC::UMC_ERR_UNSUPPORTED
}VC1Status;
#define VC1_TO_UMC_CHECK_STS(sts) if (sts != VC1_OK) return sts;


// start code (for vc1 format)
//see C24.008-VC1-Spec-CD2r1.pdf standard Annex E,E5, p. 431
typedef enum
{
    VC1_EndOfSequence              = 0x0A,
    VC1_Slice                      = 0x0B,
    VC1_Field                      = 0x0C,
    VC1_FrameHeader                = 0x0D,
    VC1_EntryPointHeader           = 0x0E,
    VC1_SequenceHeader             = 0x0F,
    VC1_SliceLevelUserData         = 0x1B,
    VC1_FieldLevelUserData         = 0x1C,
    VC1_FrameLevelUserData         = 0x1D,
    VC1_EntryPointLevelUserData    = 0x1E,
    VC1_SequenceLevelUserData      = 0x1F
} VC1StartCode;

//frame coding mode
enum
{
    VC1_Progressive           = 0,
    VC1_FrameInterlace        = 1,
    VC1_FieldInterlace        = 2
};

enum
{
    VC1_I_FRAME = 0,
    VC1_P_FRAME = 1,
    VC1_B_FRAME = 2,
    VC1_BI_FRAME= 3,
    VC1_SKIPPED_FRAME = 0x4
};
#define VC1_IS_SKIPPED(value) ((value & VC1_SKIPPED_FRAME) == VC1_SKIPPED_FRAME)
#define VC1_IS_REFERENCE(value) ((value < VC1_B_FRAME)||VC1_IS_SKIPPED(value))
#define VC1_IS_NOT_PRED(value) ((value == VC1_I_FRAME)||(value == VC1_BI_FRAME))
#define VC1_IS_PRED(value) ((value == VC1_P_FRAME)||(value == VC1_B_FRAME))


#define IS_VC1_USER_DATA(x) (x <= VC1_SequenceLevelUserData && x >= VC1_SliceLevelUserData)
#define IS_VC1_DATA_SC(x) (x <= VC1_EntryPointHeader && x >= VC1_Slice)


enum
{
    VC1_COND_OVER_FLAG_NONE = 0, //no 8x8 block boundaries are smoothed,
                             //see standart, p163
    VC1_COND_OVER_FLAG_ALL = 2,  //all 8x8 block boundaries are smoothed,
                             //see standart, p163
    VC1_COND_OVER_FLAG_SOME = 3  //some 8x8 block boundaries are smoothed,
                             //see standart, p163
};
#define VC1_MAX_BITPANE_CHUNCKS 7
#define VC1_NUM_OF_BLOCKS 6
#define VC1_NUM_OF_LUMA   4
#define VC1_PIXEL_IN_LUMA 16
#define VC1_PIXEL_IN_CHROMA 8
#define VC1_PIXEL_IN_BLOCK  8

enum
{
    VC1_BLK_INTER8X8   = 0x1,
    VC1_BLK_INTER8X4   = 0x2,
    VC1_BLK_INTER4X8   = 0x4,
    VC1_BLK_INTER4X4   = 0x8,
    VC1_BLK_INTER      = 0xf,
    VC1_BLK_INTRA_TOP  = 0x10,
    VC1_BLK_INTRA_LEFT = 0x20,
    VC1_BLK_INTRA      = 0x30
};


#define VC1SLICEINPARAL 512
#define VC1FRAMEPARALLELPAIR 2

enum
{
    VC1_MVMODE_HPELBI_1MV    = 0,    //0000      1 MV Half-pel bilinear
    VC1_MVMODE_1MV           = 1,    //1         1 MV
    VC1_MVMODE_MIXED_MV      = 2,    //01        Mixed MV
    VC1_MVMODE_HPEL_1MV      = 3,    //001       1 MV Half-pel
    VC1_MVMODE_INTENSCOMP    = 4     //0001      Intensity Compensation
};

enum
{
    VC1_DQPROFILE_ALL4EDGES  = 0,    //00    All four Edges
    VC1_DQPROFILE_DBLEDGES   = 1,    //01    Double Edges
    VC1_DQPROFILE_SNGLEDGES  = 2,    //10    Single Edges
    VC1_DQPROFILE_ALLMBLKS   = 3     //11    All Macroblocks
};
enum
{
    VC1_ALTPQUANT_LEFT            = 1,
    VC1_ALTPQUANT_TOP             = 2,
    VC1_ALTPQUANT_RIGTHT          = 4,
    VC1_ALTPQUANT_BOTTOM          = 8,
    VC1_ALTPQUANT_LEFT_TOP        = 3,
    VC1_ALTPQUANT_TOP_RIGTHT      = 6,
    VC1_ALTPQUANT_RIGTHT_BOTTOM   = 12,
    VC1_ALTPQUANT_BOTTOM_LEFT     = 9,
    VC1_ALTPQUANT_EDGES           = 15,
    VC1_ALTPQUANT_ALL             = 16,
    VC1_ALTPQUANT_NO              = 0,
    VC1_ALTPQUANT_MB_LEVEL        = 32,
    VC1_ALTPQUANT_ANY_VALUE       = 64
};
enum //profile definitions
{
    VC1_PROFILE_SIMPLE = 0, //disables X8 Intraframe, Loop filter, DQuant, and
                            //Multires while enabling the Fast Transform
    VC1_PROFILE_MAIN   = 1, //The main profile is has all the simple profile
                            //tools plus loop filter, dquant, and multires
    VC1_PROFILE_RESERVED = 2,
    VC1_PROFILE_ADVANCED = 3,    //The complex profile has X8 Intraframe can use
                      //the normal IDCT transform or the VC1 Inverse Transform
    VC1_PROFILE_UNKNOWN = 4
};

#define VC1_IS_VALID_PROFILE(profile) ((profile == VC1_PROFILE_RESERVED)||(profile > VC1_PROFILE_ADVANCED)?false:true)
enum //bitplane modes definitions
{
    VC1_BITPLANE_RAW_MODE        = 0,//Raw           0000
    VC1_BITPLANE_NORM2_MODE      = 1,//Norm-2        10
    VC1_BITPLANE_DIFF2_MODE      = 2,//Diff-2        001
    VC1_BITPLANE_NORM6_MODE      = 3,//Norm-6        11
    VC1_BITPLANE_DIFF6_MODE      = 4,//Diff-6        0001
    VC1_BITPLANE_ROWSKIP_MODE    = 5,//Rowskip       010
    VC1_BITPLANE_COLSKIP_MODE    = 6 //Colskip       011
};

#define VC1_UNDEF_PQUANT 0
#define VC1_MVINTRA (0X7F7F)

enum //quantizer deadzone definitions
{
    VC1_QUANTIZER_UNIFORM    = 0,
    VC1_QUANTIZER_NONUNIFORM = 1
};


//for extended differantial MV range flag(inerlace P picture)
enum
{
    VC1_DMVRANGE_NONE = 0,
    VC1_DMVRANGE_HORIZONTAL_RANGE,
    VC1_DMVRANGE_VERTICAL_RANGE,
    VC1_DMVRANGE_HORIZONTAL_VERTICAL_RANGE
};


//intensity comprnsation
enum
{
    VC1_INTCOMP_TOP_FIELD    = 1,
    VC1_INTCOMP_BOTTOM_FIELD = 2,
    VC1_INTCOMP_BOTH_FIELD   = 3
};

#define VC1_IS_INT_TOP_FIELD(value)    (value & VC1_INTCOMP_TOP_FIELD)
#define VC1_IS_INT_BOTTOM_FIELD(value) (value & VC1_INTCOMP_BOTTOM_FIELD)
#define VC1_IS_INT_BOTH_FIELD(value)   (value & VC1_INTCOMP_BOTH_FIELD)

#define VC1_BRACTION_INVALID 0
#define VC1_BRACTION_BI 9

#define VC1_MAX_SLICE_NUM  512

typedef struct
{
    int32_t*             pRLTable;
    const int8_t*        pDeltaLevelLast0;
    const int8_t*        pDeltaLevelLast1;
    const int8_t*        pDeltaRunLast0;
    const int8_t*        pDeltaRunLast1;
}IppiACDecodeSet_VC1;


#define VC1_IS_BITPLANE_RAW_MODE(bitplane) ((bitplane)->m_imode == VC1_BITPLANE_RAW_MODE)
#define VC1_IS_U_PRESENT_IN_CBPCY(value) (value & 2)
#define VC1_IS_V_PRESENT_IN_CBPCY(value) (value & 1)

#define VC1_NEXT_BITS(num_bits, value) VC1NextNBits(pContext->m_bitstream.pBitstream, pContext->m_bitstream.bitOffset, num_bits, value);
#define VC1_GET_BITS(num_bits, value)  VC1GetNBits(pContext->m_bitstream.pBitstream, pContext->m_bitstream.bitOffset, num_bits, value);


typedef struct
{
    uint8_t              m_invert;
    int32_t             m_imode;
    uint8_t*             m_databits;
}VC1Bitplane;

typedef struct
{
    //common field
    uint32_t PROFILE;            //2

    //Advanced profile fields
    uint32_t LEVEL;              //3

    //common fields
    uint32_t FRMRTQ_POSTPROC;  //3
    uint32_t BITRTQ_POSTPROC;  //5

    uint32_t FRAMERATENR;  //8
    uint32_t FRAMERATEDR;  //4

    //Advanced profile fields
    uint32_t POSTPROCFLAG;      //1
    uint32_t MAX_CODED_WIDTH;   //12
    uint32_t MAX_CODED_HEIGHT;  //12
    int32_t AspectRatioW;
    int32_t AspectRatioH;
    uint32_t PULLDOWN;          //1
    uint32_t INTERLACE;         //1
    uint32_t TFCNTRFLAG;        //1

    //Simple/Main profile fields
    uint32_t LOOPFILTER;       //1 uimsbf
    uint32_t MULTIRES;         //1 uimsbf
    uint32_t FASTUVMC;         //1 uimsbf
    uint32_t EXTENDED_MV;      //1 uimsbf
    uint32_t DQUANT;           //2 uimsbf
    uint32_t VSTRANSFORM;      //1 uimsbf
    uint32_t OVERLAP;          //1 uimsbf
    uint32_t SYNCMARKER;       //1 uimsbf
    uint32_t RANGERED;         //1 uimsbf
    uint32_t MAXBFRAMES;       //3 uimsbf
    uint32_t QUANTIZER;        //2 uimsbf

    //common fields
    uint32_t FINTERPFLAG;        //1
    uint32_t HRD_PARAM_FLAG;     //1

    //HRD PARAMS
   uint32_t HRD_NUM_LEAKY_BUCKETS; //5

    uint16_t widthMB;
    uint16_t heightMB;

    uint16_t MaxWidthMB;    //maximum MB in row (calculated from MAX_CODED_WIDTH
    uint16_t MaxHeightMB;   //maximum MB in column (calculated from MAX_CODED_HEIGHT

    //entry point
    uint32_t BROKEN_LINK;
    uint32_t CLOSED_ENTRY;
    uint32_t PANSCAN_FLAG;
    uint32_t REFDIST_FLAG;

    uint32_t CODED_WIDTH;
    uint32_t CODED_HEIGHT;
    uint32_t EXTENDED_DMV;
    uint32_t RANGE_MAPY_FLAG;
    int32_t RANGE_MAPY;
    uint32_t RANGE_MAPUV_FLAG;
    int32_t RANGE_MAPUV;
    uint32_t RNDCTRL;     // 1 rounding control bit

    // video info
    uint16_t          VideoFormat;
    uint16_t          VideoFullRange;
    uint16_t          ColourDescriptionPresent;
    uint16_t          ColourPrimaries;
    uint16_t          TransferCharacteristics;
    uint16_t          MatrixCoefficients;


}VC1SequenceLayerHeader;


typedef struct
{
  //common fields
    uint32_t  PTYPE;
    uint32_t  PQINDEX;
    uint32_t  HALFQP;
//common fields.Slice parameters
    uint32_t  is_slice;

//interlace fields
    uint8_t  TFF;           // 1 top field first
    uint8_t  RFF;           // repeat field
    uint8_t  PTypeField1;
    uint8_t  PTypeField2;
    uint8_t  CurrField;
    uint8_t  BottomField;    //Is current field top or nor
    uint32_t  INTCOMFIELD;     //variable size intensity compensation field

    //range reduce simple/main profile
    int32_t RANGEREDFRM;      //1

//I P B picture. VopDQuant
    uint32_t  m_DQProfile;
    uint32_t  m_DQuantFRM;
    uint32_t  m_DQBILevel;
    uint32_t  m_AltPQuant;
    uint32_t  PQUANT;
    uint32_t  m_PQuant_mode;
    uint32_t  QuantizationType;      //0 - uniform, 1 - nonuniform

//only for advanced profile
    uint32_t  FCM;           // variable size frame coding mode

//I BI picture
    VC1Bitplane    ACPRED;    // variable size  AC Prediction
    uint32_t   CONDOVER;  // variable size  conditional overlap flag
    VC1Bitplane    OVERFLAGS; // variable size  conditional overlap macroblock pattern flags
//I BI picture. interlace frame
    VC1Bitplane    FIELDTX;     //variable size field transform flag

// P B picture
    uint32_t   TTMBF;
    uint32_t   TTFRM;
    uint32_t    TTFRM_ORIG; //for future H/W support needs
    VC1Bitplane    SKIPMB;
    VC1Bitplane    MVTYPEMB;
    uint32_t   MVMODE;

// P B picture. Interlace frame
    uint32_t   DMVRANGE;              //variable size extended differential MV Range Flag
// P B picture. Interlace field
    int32_t   REFDIST;               //variable size  P Reference Distance

//P only. Interlace field
    uint32_t NUMREF;                //1     Number of reference picture
    uint32_t REFFIELD;              //1     Reference field picture indicator

//B only
    uint32_t     BFRACTION;
    uint32_t     BFRACTION_index;
    VC1Bitplane      m_DirectMB;
    int32_t           ScaleFactor;
// B only. Interlace field
    VC1Bitplane      FORWARDMB;         //variable size B Field forward mode
                                        //MB bit syntax element
// tables
    uint32_t RNDCTRL;     // 1 rounding control bit

    uint32_t TRANSDCTAB;
    uint32_t MVMODE2;
    uint32_t MVTAB;
    uint32_t CBPTAB;
    uint32_t MBMODETAB;
    uint32_t MV2BPTAB;   //2             2 mv block pattern table
    uint32_t MV4BPTAB;   //2             4 mv block pattern table
    uint32_t PQUANTIZER;
    uint32_t MVRANGE;
    uint32_t DQSBEdge;
    // P picture
    uint32_t LUMSCALE;
    uint32_t LUMSHIFT;
    uint32_t LUMSCALE1;
    uint32_t LUMSHIFT1;

    uint32_t POSTPROC;

    uint32_t TRANSACFRM;
    uint32_t TRANSACFRM2;

    uint32_t  RPTFRM;        // 2 repeat frame count
    uint32_t  UVSAMP;

    uint32_t MV4SWITCH;
}VC1PictureLayerHeader;

typedef struct
{
    int32_t *m_Bitplane_IMODE;
    int32_t *m_BitplaneTaledbits;
    int32_t *BFRACTION;
    int32_t *REFDIST_TABLE;


} VC1VLCTables;


typedef struct
{
    uint32_t*     pBitstream;
    int32_t      bitOffset;
} IppiBitstream;

//1.3.3    The decoder dynamically allocates memory for frame
//buffers and other structures. Following are the size requirements.
//For the frame buffer, the following formula indicates the memory requirement:
//frame_memory = {(h + 64) x (w + 64) + [(h / 2 + 32) x (w / 2 + 32)] x 2}bytes


struct Frame
{
    Frame():m_bIsExpanded(0),
            FCM(0),
            ICFieldMask(0),
            corrupted(0)
    {
    }

    uint8_t       m_bIsExpanded;
    uint32_t      FCM;

    //1000  - IC for Top First Field
    //0100  - IC for Bottom First Field
    //0010  - IC for Top Second Field
    //0001  - IC for Bottom Second Field
    uint32_t      ICFieldMask;

    uint16_t      corrupted;

};

#define VC1MAXFRAMENUM 9*VC1FRAMEPARALLELPAIR + 9 // for <= 8 threads. Change if numThreads > 8
#define VC1NUMREFFRAMES 2 // 2 - reference frames
struct VC1FrameBuffer
{
    Frame*        m_pFrames;

    int32_t        m_iPrevIndex;
    int32_t        m_iNextIndex;
    int32_t        m_iCurrIndex;
    int32_t        m_iRangeMapIndex;
    int32_t        m_iRangeMapIndexPrev; // for correct va support
    int32_t        m_iDisplayIndex;
    int32_t        m_iToFreeIndex;

    // for skipping processing
    int32_t        m_iToSkipCoping;

    // for external index set up
    int32_t        m_iBFrameIndex;

    Frame operator [] (uint32_t number)
    {
        return m_pFrames[number];
    };

};

typedef struct
{
    uint32_t start_pos;
    uint32_t HeightMB;
    uint32_t is_last_deblock;
    uint32_t isNeedDbl;
}VC1DeblockInfo;

struct VC1Context
{
    VC1SequenceLayerHeader     m_seqLayerHeader;
    VC1PictureLayerHeader*     m_picLayerHeader;

    VC1PictureLayerHeader*     m_InitPicLayer;

    VC1VLCTables*              m_vlcTbl;

    VC1FrameBuffer             m_frmBuff;

    IppiBitstream          m_bitstream;

    uint8_t*                 m_pBufferStart;
    uint32_t                 m_FrameSize;
    //start codes
    uint32_t*                m_Offsets;
    uint32_t*                m_values;


    VC1Bitplane            m_pBitplane;
    VC1DeblockInfo         DeblockInfo;
    uint32_t                 RefDist;
    uint32_t*                pRefDist;

    // Intensity compensation: lookup tables only for P-frames
    uint8_t          m_bIntensityCompensation;


    uint32_t                        PrevFCM;
    uint32_t                        NextFCM;

    int32_t                        bp_round_count;
};

#endif //__umc_vc1_common_defs_H__
#endif //MFX_ENABLE_VC1_VIDEO_DECODE
