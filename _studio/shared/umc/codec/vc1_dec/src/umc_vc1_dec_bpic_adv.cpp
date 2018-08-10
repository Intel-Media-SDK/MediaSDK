// Copyright (c) 2017-2018 Intel Corporation
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

#include "umc_vc1_dec_seq.h"
#include "umc_vc1_huffman.h"
#include "umc_vc1_common_tables.h"

static const uint32_t bc_lut_2[] = {0,1,2,3};
static const uint32_t bc_lut_1[] = {4,0,1,3};

VC1Status DecodePictHeaderParams_ProgressiveBpicture_Adv (VC1Context* pContext)
{
    VC1Status vc1Res = VC1_OK;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;


    //extended MV range flag
    MVRangeDecode(pContext);

    //motion vector mode
    VC1_GET_BITS(1, picLayerHeader->MVMODE);
    picLayerHeader->MVMODE =(picLayerHeader->MVMODE==1)? VC1_MVMODE_1MV:VC1_MVMODE_HPELBI_1MV;


    //B frame direct mode macroblock bit syntax element
    DecodeBitplane(pContext, &picLayerHeader->m_DirectMB,
                   seqLayerHeader->widthMB, seqLayerHeader->heightMB,0);

    //skipped macroblock bit syntax element
    DecodeBitplane(pContext, &picLayerHeader->SKIPMB,
                   seqLayerHeader->widthMB, seqLayerHeader->heightMB,0);

    //motion vector table
    VC1_GET_BITS(2, picLayerHeader->MVTAB);       //MVTAB

    //coded block pattern table
    VC1_GET_BITS(2,picLayerHeader->CBPTAB);       //CBPTAB

    vc1Res = VOPDQuant(pContext);

    if(seqLayerHeader->VSTRANSFORM)
    {
        //macroblock-level transform type flag
        VC1_GET_BITS(1, picLayerHeader->TTMBF);

        if(picLayerHeader->TTMBF)
        {
            //frame-level transform type
            VC1_GET_BITS(2, picLayerHeader->TTFRM_ORIG);
            picLayerHeader->TTFRM = 1 << picLayerHeader->TTFRM_ORIG;
        }
        else
        {
            picLayerHeader->TTFRM = VC1_BLK_INTER;
        }
    }
    else
    {
        picLayerHeader->TTFRM = VC1_BLK_INTER8X8;
    }


    VC1_GET_BITS(1, picLayerHeader->TRANSACFRM);//TRANSACFRM
    if(picLayerHeader->TRANSACFRM == 1)
    {
        VC1_GET_BITS(1, picLayerHeader->TRANSACFRM);
        picLayerHeader->TRANSACFRM++;
    }

    VC1_GET_BITS(1, picLayerHeader->TRANSDCTAB);       //TRANSDCTAB

    return vc1Res;
}


VC1Status DecodePictHeaderParams_InterlaceBpicture_Adv(VC1Context* pContext)
{
    VC1Status vc1Res = VC1_OK;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;

    uint32_t tempValue;


    {
        //B picture fraction
        int8_t  z1;
        int16_t z2;
        DecodeHuffmanPair(&pContext->m_bitstream.pBitstream,
                                    &pContext->m_bitstream.bitOffset,
                                    pContext->m_vlcTbl->BFRACTION,
                                    &z1, &z2);
        VM_ASSERT (z2 != VC1_BRACTION_INVALID);
        VM_ASSERT (!(z2 == VC1_BRACTION_BI && seqLayerHeader->PROFILE==VC1_PROFILE_ADVANCED));

        if (z2 == VC1_BRACTION_BI)
        {
            picLayerHeader->PTYPE = VC1_BI_FRAME;
        }
        picLayerHeader->BFRACTION = (z1*2>=z2)?1:0;
        picLayerHeader->ScaleFactor = ((256+z2/2)/z2)*z1;
        if (z1 < 8 && z2 < 9)
            picLayerHeader->BFRACTION_index = VC1_BFraction_indexes[z1][z2];
    }


    //extended MV range flag
    MVRangeDecode(pContext);

    //extended differential MV Range Flag
    if(seqLayerHeader->EXTENDED_DMV == 1)
    {
        VC1_GET_BITS(1, picLayerHeader->DMVRANGE);
        if(picLayerHeader->DMVRANGE==0)
        {
            //binary code 0
            picLayerHeader->DMVRANGE = VC1_DMVRANGE_NONE;
        }
        else
        {
            VC1_GET_BITS(1, picLayerHeader->DMVRANGE);
            if(picLayerHeader->DMVRANGE==0)
            {
               //binary code 10
               picLayerHeader->DMVRANGE = VC1_DMVRANGE_HORIZONTAL_RANGE;
            }
            else
            {
                VC1_GET_BITS(1, picLayerHeader->DMVRANGE);
                if(picLayerHeader->DMVRANGE==0)
                {
                    //binary code 110
                    picLayerHeader->DMVRANGE = VC1_DMVRANGE_VERTICAL_RANGE;
                }
                else
                {
                    //binary code 111
                    picLayerHeader->DMVRANGE = VC1_DMVRANGE_HORIZONTAL_VERTICAL_RANGE;
                }
            }
        }
    }

    //intensity compensation
    VC1_GET_BITS(1, tempValue);       //INTCOMP

    //B frame direct mode macroblock bit syntax element
    DecodeBitplane(pContext, &picLayerHeader->m_DirectMB,
                   seqLayerHeader->widthMB, seqLayerHeader->heightMB,0);

    //skipped macroblock bit syntax element
    DecodeBitplane(pContext, &picLayerHeader->SKIPMB,
                   seqLayerHeader->widthMB, seqLayerHeader->heightMB,0);


    //in B pic MVMODE always VC1_MVMODE_1MV
    picLayerHeader->MVMODE = VC1_MVMODE_1MV;

    //motion vector table
    VC1_GET_BITS(2, picLayerHeader->MBMODETAB);       //MBMODETAB

    //motion vector table
    VC1_GET_BITS(2, picLayerHeader->MVTAB);       //MVTAB


    //coded block pattern table
    VC1_GET_BITS(3,picLayerHeader->CBPTAB);       //CBPTAB

    VC1_GET_BITS(2, picLayerHeader->MV2BPTAB);       //MV2BPTAB

    VC1_GET_BITS(2, picLayerHeader->MV4BPTAB)        //MV4BPTAB;

    vc1Res = VOPDQuant(pContext);

    if(seqLayerHeader->VSTRANSFORM)
    {
        //macroblock-level transform type flag
        VC1_GET_BITS(1, picLayerHeader->TTMBF);

        if(picLayerHeader->TTMBF)
        {
            //frame-level transform type
            VC1_GET_BITS(2, picLayerHeader->TTFRM_ORIG);
            picLayerHeader->TTFRM = 1 << picLayerHeader->TTFRM_ORIG;
        }
        else
        {
            picLayerHeader->TTFRM = VC1_BLK_INTER;
        }
    }
    else
    {
        picLayerHeader->TTFRM = VC1_BLK_INTER8X8;
    }

    //frame-level transform AC Coding set index
    VC1_GET_BITS(1, picLayerHeader->TRANSACFRM);//TRANSACFRM
    if(picLayerHeader->TRANSACFRM == 1)
    {
        VC1_GET_BITS(1, picLayerHeader->TRANSACFRM);
        picLayerHeader->TRANSACFRM++;
    }

    VC1_GET_BITS(1, picLayerHeader->TRANSDCTAB);       //TRANSDCTAB


    return vc1Res;
}


VC1Status DecodeFieldHeaderParams_InterlaceFieldBpicture_Adv (VC1Context* pContext)
{
    VC1Status vc1Res = VC1_OK;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;
    uint32_t tempValue;


    picLayerHeader->NUMREF = 1;


    VC1_GET_BITS(5,picLayerHeader->PQINDEX);

    if(picLayerHeader->PQINDEX<=8)
    {
        VC1_GET_BITS(1,picLayerHeader->HALFQP);
    }
    else
        picLayerHeader->HALFQP = 0;


    if(seqLayerHeader->QUANTIZER == 1)
    {
        VC1_GET_BITS(1,picLayerHeader->PQUANTIZER);    //PQUANTIZER
    }

    CalculatePQuant(pContext);

    if(seqLayerHeader->POSTPROCFLAG)
    {
         //post processing
          VC1_GET_BITS(2,tempValue);        //POSTPROC
    }

    //extended MV range flag
    MVRangeDecode(pContext);

    //extended differential MV Range Flag
    DMVRangeDecode(pContext);

    //motion vector mode
    if(picLayerHeader->PQUANT > 12)
    {
        //MVMODE VLC    Mode
        //1             1 MV Half-pel bilinear
        //01            1 MV
        //001           1 MV Half-pel
        //000           Mixed MV
        int32_t bit_count = 1;
        VC1_GET_BITS(1, picLayerHeader->MVMODE);
        while((picLayerHeader->MVMODE == 0) && (bit_count < 3))
        {
            VC1_GET_BITS(1, picLayerHeader->MVMODE);
            bit_count++;
        }
        if (bit_count < 3)
            picLayerHeader->MVMODE = VC1_LUT_SET(bit_count,bc_lut_1);
        else
            if(picLayerHeader->MVMODE == 0)
                picLayerHeader->MVMODE = VC1_MVMODE_MIXED_MV;
            else
                picLayerHeader->MVMODE = VC1_MVMODE_HPEL_1MV;
    }
    else
    {
        //MVMODE VLC    Mode
        //1             1 MV
        //01            Mixed MV
        //001           1 MV Half-pel
        //000           1 MV Half-pel bilinear
        int32_t bit_count = 1;
        VC1_GET_BITS(1, picLayerHeader->MVMODE);
        while((picLayerHeader->MVMODE == 0) && (bit_count < 3))
        {
            VC1_GET_BITS(1, picLayerHeader->MVMODE);
            bit_count++;
        }
        if (bit_count < 3)
            picLayerHeader->MVMODE = VC1_LUT_SET(bit_count,bc_lut_2);
        else
            if(picLayerHeader->MVMODE == 0)
                picLayerHeader->MVMODE = VC1_MVMODE_HPELBI_1MV;
            else
                picLayerHeader->MVMODE = VC1_MVMODE_HPEL_1MV;
    }

    //FORWARDMB
    if (picLayerHeader->CurrField == 0)
    DecodeBitplane(pContext, &picLayerHeader->FORWARDMB,
                   seqLayerHeader->widthMB, (seqLayerHeader->heightMB+1)/2,0);
    else
    DecodeBitplane(pContext, &picLayerHeader->FORWARDMB,
                   seqLayerHeader->widthMB, (seqLayerHeader->heightMB+1)/2,
                   seqLayerHeader->MaxWidthMB * ((seqLayerHeader->heightMB+1)/2));

    //motion vector table
    VC1_GET_BITS(3, picLayerHeader->MBMODETAB);       //MBMODETAB

    VC1_GET_BITS(3, picLayerHeader->MVTAB);       //MVTAB

    //coded block pattern table
    VC1_GET_BITS(3, picLayerHeader->CBPTAB);       //CBPTAB

    if(picLayerHeader->MVMODE == VC1_MVMODE_MIXED_MV)
    {
        VC1_GET_BITS(2, picLayerHeader->MV4BPTAB)        //MV4BPTAB;
    }

    vc1Res = VOPDQuant(pContext);

    if(seqLayerHeader->VSTRANSFORM == 1)
    {
        //macroblock - level transform type flag
        VC1_GET_BITS(1, picLayerHeader->TTMBF);

        if(picLayerHeader->TTMBF)
        {
            //frame-level transform type
            VC1_GET_BITS(2, picLayerHeader->TTFRM_ORIG);
            picLayerHeader->TTFRM = 1 << picLayerHeader->TTFRM_ORIG;
        }
        else
            picLayerHeader->TTFRM = VC1_BLK_INTER;
    }
    else
    {
        picLayerHeader->TTFRM = VC1_BLK_INTER8X8;
    }

    //frame-level transform AC Coding set index
    VC1_GET_BITS(1, picLayerHeader->TRANSACFRM);//TRANSACFRM
    if(picLayerHeader->TRANSACFRM == 1)
    {
        VC1_GET_BITS(1, picLayerHeader->TRANSACFRM);
        picLayerHeader->TRANSACFRM++;
    }

    VC1_GET_BITS(1, picLayerHeader->TRANSDCTAB);       //TRANSDCTAB


    picLayerHeader->REFDIST = *pContext->pRefDist;


    return vc1Res;
}

#endif //MFX_ENABLE_VC1_VIDEO_DECODE
