// Copyright (c) 2017 Intel Corporation
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

#ifndef __UMC_VC1_DEC_SEQ_H__
#define __UMC_VC1_DEC_SEQ_H__

#include "umc_vc1_common_defs.h"
#include "umc_frame_allocator.h"

typedef struct
{
    uint16_t                   MBStartRow;
    uint16_t                   MBEndRow;
    uint16_t                   MBRowsToDecode;
    uint32_t*                  m_pstart;
    int32_t                   m_bitOffset;
    VC1PictureLayerHeader*   m_picLayerHeader;
    VC1VLCTables*            m_vlcTbl;
    bool                     is_continue;
    uint32_t                   slice_settings;
    bool                     is_NewInSlice;
    bool                     is_LastInSlice;
    int32_t                   iPrevDblkStartPos; //need to interlace frames
} SliceParams;

//sequence layer
VC1Status SequenceLayer                               (VC1Context* pContext);

//picture layer
//Simple/main
VC1Status GetNextPicHeader                            (VC1Context* pContext, bool isExtHeader);
VC1Status DecodePictureHeader                         (VC1Context* pContext,  bool isExtHeader);
VC1Status Decode_PictureLayer                         (VC1Context* pContext);

//Advanced
VC1Status DecodePictureHeader_Adv                     (VC1Context* pContext);
VC1Status GetNextPicHeader_Adv                        (VC1Context* pContext);


//frame rate calculation
void MapFrameRateIntoMfx(uint32_t& ENR,uint32_t& EDR, uint16_t FCode);
double MapFrameRateIntoUMC(uint32_t ENR,uint32_t EDR, uint32_t& FCode);

//I,P,B headers
VC1Status DecodePicHeader                             (VC1Context* pContext);

VC1Status DecodePictHeaderParams_InterlaceFieldPicture_Adv (VC1Context* pContext);
VC1Status DecodeSkippicture                       (VC1Context* pContext);


// Simple/Main
VC1Status DecodePictureLayer_ProgressiveIpicture            (VC1Context* pContext);
// Advanced
VC1Status DecodePictHeaderParams_ProgressiveIpicture_Adv    (VC1Context* pContext);
VC1Status DecodePictHeaderParams_InterlaceIpicture_Adv      (VC1Context* pContext);
VC1Status DecodeFieldHeaderParams_InterlaceFieldIpicture_Adv (VC1Context* pContext);
VC1Status Decode_InterlaceFieldIpicture_Adv                 (VC1Context* pContext);

// Simple/Main
VC1Status DecodePictureLayer_ProgressivePpicture            (VC1Context* pContext);
// Advanced
VC1Status DecodePictHeaderParams_ProgressivePpicture_Adv    (VC1Context* pContext);
VC1Status DecodePictHeaderParams_InterlacePpicture_Adv      (VC1Context* pContext);
VC1Status DecodeFieldHeaderParams_InterlaceFieldPpicture_Adv (VC1Context* pContext);
VC1Status Decode_InterlaceFieldPpicture_Adv                 (VC1Context* pContext);

// Simple/Main
VC1Status DecodePictureLayer_ProgressiveBpicture            (VC1Context* pContext);
// Advanced
VC1Status DecodePictHeaderParams_ProgressiveBpicture_Adv    (VC1Context* pContext);
VC1Status DecodePictHeaderParams_InterlaceBpicture_Adv      (VC1Context* pContext);
VC1Status Decode_InterlaceFieldBpicture_Adv                 (VC1Context* pContext);
VC1Status DecodeFieldHeaderParams_InterlaceFieldBpicture_Adv(VC1Context* pContext);

VC1Status MVRangeDecode                                (VC1Context* pContext);
VC1Status DMVRangeDecode                               (VC1Context* pContext);

//DeQuantization
VC1Status VOPDQuant(VC1Context* pContext);
VC1Status CalculatePQuant(VC1Context* pContext);

//Bitplane decoding
void  DecodeBitplane(VC1Context* pContext, VC1Bitplane* pBitplane, int32_t rowMB, int32_t colMB, int32_t offset);

VC1Status EntryPointLayer(VC1Context* m_pContext);



#endif //__umc_vc1_dec_seq_H__
#endif //MFX_ENABLE_VC1_VIDEO_DECODE
