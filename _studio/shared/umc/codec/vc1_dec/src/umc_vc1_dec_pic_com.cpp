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

#include <string.h>
#include "umc_vc1_dec_seq.h"
#include "umc_vc1_huffman.h"
#include "umc_vc1_common_tables.h"

VC1Status DecodePictureHeader (VC1Context* pContext,  bool isExtHeader)
{
    VC1Status vc1Sts = VC1_OK;
    uint32_t tempValue;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;
    uint32_t SkFrameSize = 10;

    if (isExtHeader)
        SkFrameSize -= 8;

    if(seqLayerHeader->FINTERPFLAG == 1)
        VC1_GET_BITS(1, tempValue); //INTERPFRM

    VC1_GET_BITS(2, tempValue);       //FRMCNT

    if(seqLayerHeader->RANGERED == 1)
    {
        VC1_GET_BITS(1,picLayerHeader->RANGEREDFRM);
        picLayerHeader->RANGEREDFRM <<= 3;
    }
    else
        picLayerHeader->RANGEREDFRM = 0;

    if(seqLayerHeader->MAXBFRAMES == 0)
    {
        VC1_GET_BITS(1, picLayerHeader->PTYPE);//0 = I, 1 = P
    }
    else
    {
        VC1_GET_BITS(1, picLayerHeader->PTYPE); //1 = P
        if(picLayerHeader->PTYPE == 0)
        {
            VC1_GET_BITS(1, picLayerHeader->PTYPE);
            if(picLayerHeader->PTYPE == 0)
            {
                picLayerHeader->PTYPE = VC1_B_FRAME;
                //it can be BI, will be detected in BFRACTION
            }
            else
            {
                picLayerHeader->PTYPE = VC1_I_FRAME;
            }
        }
        else
        {
            picLayerHeader->PTYPE = VC1_P_FRAME;
        }
    }

    if(picLayerHeader->PTYPE == VC1_B_FRAME)
    {
        int8_t  z1;
        int16_t z2;
        DecodeHuffmanPair(&pContext->m_bitstream.pBitstream,
                                    &pContext->m_bitstream.bitOffset,
                                    pContext->m_vlcTbl->BFRACTION,
                                    &z1,
                                    &z2);
        VM_ASSERT (z2 != VC1_BRACTION_INVALID);
        VM_ASSERT (!(z2 == VC1_BRACTION_BI && seqLayerHeader->PROFILE==VC1_PROFILE_ADVANCED));
        if(0 == z2)
            return VC1_ERR_INVALID_STREAM;
        if (z2 == VC1_BRACTION_BI)
        {
            picLayerHeader->PTYPE = VC1_BI_FRAME;
        }
        picLayerHeader->BFRACTION = (z1*2>=z2)?1:0;
        picLayerHeader->ScaleFactor = ((256+z2/2)/z2)*z1;
        if (z1 < 8 && z2 < 9)
            picLayerHeader->BFRACTION_index = VC1_BFraction_indexes[z1][z2];
    }

    if(pContext->m_FrameSize < SkFrameSize) //changed from 2
    {
        picLayerHeader->PTYPE |= VC1_SKIPPED_FRAME;
    }

    return vc1Sts;
}


VC1Status Decode_PictureLayer(VC1Context* pContext)
{
    VC1Status vc1Sts = VC1_OK;
    switch(pContext->m_picLayerHeader->PTYPE)
    {
    case VC1_I_FRAME:
    case VC1_BI_FRAME:
        vc1Sts = DecodePictureLayer_ProgressiveIpicture(pContext);

        break;
    case VC1_P_FRAME:
        //only for simple and main profile
        vc1Sts = DecodePictureLayer_ProgressivePpicture(pContext);
        break;

    case VC1_B_FRAME:

        vc1Sts = DecodePictureLayer_ProgressiveBpicture(pContext);
        break;
    }

    if (VC1_IS_SKIPPED(pContext->m_picLayerHeader->PTYPE))
        pContext->m_frmBuff.m_iDisplayIndex = pContext->m_frmBuff.m_iCurrIndex;

    return vc1Sts;
}

#endif //MFX_ENABLE_VC1_VIDEO_DECODE
