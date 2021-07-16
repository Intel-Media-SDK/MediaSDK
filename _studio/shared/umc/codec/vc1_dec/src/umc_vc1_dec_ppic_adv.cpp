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

#include "umc_vc1_dec_seq.h"

static const uint32_t bc_lut_1[] = {4,0,1,3};
static const uint32_t bc_lut_2[] = {0,1,2,3};
static const uint32_t bc_lut_4[] = {0,1,2};
static const uint32_t bc_lut_5[] = {0,0,1};

VC1Status DecodePictHeaderParams_ProgressivePpicture_Adv    (VC1Context* pContext)
{
    VC1Status vc1Res = VC1_OK;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;

    
    //extended MV range flag
    MVRangeDecode(pContext);

    //motion vector mode
    if(picLayerHeader->PQUANT > 12)
    {
        //VC-1 Table 45: P Picture Low rate (PQUANT > 12) MVMODE codetable
        //MVMODE VLC    Mode
        //1             1 MV Half-pel bilinear
        //01            1 MV
        //001           1 MV Half-pel
        //0000          Mixed MV
        //0001          Intensity Compensation
        int32_t bit_count = 1;
        VC1_GET_BITS(1, picLayerHeader->MVMODE);
        while((picLayerHeader->MVMODE == 0) && (bit_count < 4))
        {
            VC1_GET_BITS(1, picLayerHeader->MVMODE);
            bit_count++;
        }
        if (bit_count < 4)
            picLayerHeader->MVMODE = VC1_LUT_SET(bit_count,bc_lut_1);
        else
            if(picLayerHeader->MVMODE == 0)
                picLayerHeader->MVMODE = VC1_MVMODE_MIXED_MV;
            else
            {
                pContext->m_bIntensityCompensation = 1;
                picLayerHeader->MVMODE = VC1_MVMODE_INTENSCOMP;
                //VC-1 Table 48: P Picture Low rate (PQUANT > 12) MVMODE2 codetable
                //MVMODE VLC    Mode
                //1                1 MV Half-pel bilinear
                //01            1 MV
                //001            1 MV Half-pel
                //000            Mixed MV
                bit_count = 1;
                VC1_GET_BITS(1, picLayerHeader->MVMODE);
                while((picLayerHeader->MVMODE == 0) && (bit_count < 3))
                {
                    VC1_GET_BITS(1, picLayerHeader->MVMODE);
                    bit_count++;
                }
                if (bit_count < 3)
                picLayerHeader->MVMODE = VC1_LUT_SET(bit_count,bc_lut_5);
                else
                    if(picLayerHeader->MVMODE == 0)
                        picLayerHeader->MVMODE = VC1_MVMODE_MIXED_MV;
                    else
                        picLayerHeader->MVMODE = VC1_MVMODE_HPEL_1MV;
                //Luma scale
                VC1_GET_BITS(6, picLayerHeader->LUMSCALE);

                //Luma shift
                VC1_GET_BITS(6, picLayerHeader->LUMSHIFT);


                pContext->m_frmBuff.m_pFrames[pContext->m_frmBuff.m_iPrevIndex].m_bIsExpanded = 0;
                pContext->m_frmBuff.m_pFrames[pContext->m_frmBuff.m_iCurrIndex].ICFieldMask = 0xC;
                pContext->m_picLayerHeader->MVMODE2 = pContext->m_picLayerHeader->MVMODE;
        }
    }
    else
    {
        //VC-1 Table 46: P Picture High rate (PQUANT <= 12) MVMODE codetable
        //MVMODE VLC    Mode
        //1                1 MV
        //01            Mixed MV
        //001            1 MV Half-pel
        //0000            1 MV Half-pel bilinear
        //0001            Intensity Compensation
        int32_t bit_count = 1;
        VC1_GET_BITS(1, picLayerHeader->MVMODE);
        while((picLayerHeader->MVMODE == 0) && (bit_count < 4))
        {
            VC1_GET_BITS(1, picLayerHeader->MVMODE);
            bit_count++;
        }
        if (bit_count < 4)
            picLayerHeader->MVMODE = VC1_LUT_SET(bit_count,bc_lut_2);
        else
            if(picLayerHeader->MVMODE == 0)
                picLayerHeader->MVMODE = VC1_MVMODE_HPELBI_1MV;
            else
            {
                pContext->m_bIntensityCompensation = 1;
                picLayerHeader->MVMODE = VC1_MVMODE_INTENSCOMP;
                //VC-1 Table 49: P Picture High rate (PQUANT <= 12) MVMODE2 codetable
                //MVMODE VLC    Mode
                //1                1 MV
                //01            Mixed MV
                //001            1 MV Half-pel
                //000            1 MV Half-pel bilinear
                bit_count = 1;
                VC1_GET_BITS(1, picLayerHeader->MVMODE);
                while((picLayerHeader->MVMODE == 0) && (bit_count < 3))
                {
                    VC1_GET_BITS(1, picLayerHeader->MVMODE);
                    bit_count++;
                }
                if (bit_count < 3)
                    picLayerHeader->MVMODE = VC1_LUT_SET(bit_count,bc_lut_4);
                else
                    if(picLayerHeader->MVMODE == 0)
                        picLayerHeader->MVMODE = VC1_MVMODE_HPELBI_1MV;
                    else
                        picLayerHeader->MVMODE = VC1_MVMODE_HPEL_1MV;

                //Luma scale
                VC1_GET_BITS(6, picLayerHeader->LUMSCALE);

                //Luma shift
                VC1_GET_BITS(6, picLayerHeader->LUMSHIFT);


                pContext->m_frmBuff.m_pFrames[pContext->m_frmBuff.m_iPrevIndex].m_bIsExpanded = 0;

                pContext->m_picLayerHeader->MVMODE2 = pContext->m_picLayerHeader->MVMODE;
                pContext->m_frmBuff.m_pFrames[pContext->m_frmBuff.m_iCurrIndex].ICFieldMask = 0xC;
            }
    }

    //motion vector type bitplane
    if(picLayerHeader->MVMODE == VC1_MVMODE_MIXED_MV)
    {
        DecodeBitplane(pContext, &picLayerHeader->MVTYPEMB,
                        seqLayerHeader->widthMB, seqLayerHeader->heightMB,0);
    }

    //skipped macroblock bit syntax element
    DecodeBitplane(pContext, &picLayerHeader->SKIPMB,
            seqLayerHeader->widthMB, seqLayerHeader->heightMB,0);

    //motion vector table
    VC1_GET_BITS(2, picLayerHeader->MVTAB);       //MVTAB

    //coded block pattern table
    VC1_GET_BITS(2, picLayerHeader->CBPTAB);       //CBPTAB

    vc1Res = VOPDQuant(pContext);

    if(seqLayerHeader->VSTRANSFORM)
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
        {
            //_MAYBE_ see reference decoder - vc1decpic.c vc1DECPIC_UnpackPictureLayerPBAdvanced function
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
    picLayerHeader->TRANSACFRM2 = picLayerHeader->TRANSACFRM;

    //intra transfrmDC table
    VC1_GET_BITS(1, picLayerHeader->TRANSDCTAB);       //TRANSDCTAB


    return vc1Res;
}

VC1Status DecodePictHeaderParams_InterlacePpicture_Adv    (VC1Context* pContext)
{
    VC1Status vc1Res = VC1_OK;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;

    uint32_t tempValue;



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

    //4 motion vector switch
    VC1_GET_BITS(1, picLayerHeader->MV4SWITCH);
    if(picLayerHeader->MV4SWITCH)
       picLayerHeader->MVMODE = VC1_MVMODE_MIXED_MV;
    else
       picLayerHeader->MVMODE = VC1_MVMODE_1MV;

    //intensity compensation
    VC1_GET_BITS(1, tempValue);       //INTCOMP

    if(tempValue)       //INTCOM
    {
        pContext->m_bIntensityCompensation = 1;
        VC1_GET_BITS(6, picLayerHeader->LUMSCALE);
        VC1_GET_BITS(6, picLayerHeader->LUMSHIFT);


       pContext->m_frmBuff.m_pFrames[pContext->m_frmBuff.m_iCurrIndex].ICFieldMask = 0xC;
    }


    //skipped macroblock bit syntax element
    DecodeBitplane(pContext, &picLayerHeader->SKIPMB,
                   seqLayerHeader->widthMB, seqLayerHeader->heightMB,0);

    //motion vector table
    VC1_GET_BITS(2, picLayerHeader->MBMODETAB);       //MBMODETAB

    //motion vector table
    VC1_GET_BITS(2,picLayerHeader->MVTAB);       //MVTAB

    //coded block pattern table
    VC1_GET_BITS(3,picLayerHeader->CBPTAB);       //CBPTAB

    VC1_GET_BITS(2, picLayerHeader->MV2BPTAB);       //MV2BPTAB

    if(picLayerHeader->MV4SWITCH)
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


    return vc1Res;
}

VC1Status DecodeFieldHeaderParams_InterlaceFieldPpicture_Adv (VC1Context* pContext)
{
    VC1Status vc1Res = VC1_OK;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;

    uint32_t tempValue;


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

    //NUMREF
    VC1_GET_BITS(1,picLayerHeader->NUMREF);

    if(!picLayerHeader->NUMREF)
    {
        VC1_GET_BITS(1,picLayerHeader->REFFIELD);
    }

    //extended MV range flag
    MVRangeDecode(pContext);

    //extended differential MV Range Flag
    DMVRangeDecode(pContext);

    picLayerHeader->INTCOMFIELD = 0;
    pContext->m_bIntensityCompensation = 0;

    //motion vector mode
    if(picLayerHeader->PQUANT > 12)
    {
        //MVMODE VLC    Mode
        //1             1 MV Half-pel bilinear
        //01            1 MV
        //001           1 MV Half-pel
        //0000          Mixed MV
        //0001          Intensity Compensation
        int32_t bit_count = 1;
        VC1_GET_BITS(1, picLayerHeader->MVMODE);
        while((picLayerHeader->MVMODE == 0) && (bit_count < 4))
        {
            VC1_GET_BITS(1, picLayerHeader->MVMODE);
            bit_count++;
        }
        if (bit_count < 4)
            picLayerHeader->MVMODE = VC1_LUT_SET(bit_count,bc_lut_1);
        else
            if(picLayerHeader->MVMODE == 0)
                picLayerHeader->MVMODE = VC1_MVMODE_MIXED_MV;
            else
            {
                pContext->m_bIntensityCompensation = 1;
                picLayerHeader->MVMODE = VC1_MVMODE_INTENSCOMP;

                {
                    int32_t index_top    = pContext->m_frmBuff.m_iPrevIndex;
                    //pContext->m_bIntensityCompensation = 0;

                    //MVMODE VLC    Mode
                    //1             1 MV Half-pel bilinear
                    //01            1 MV
                    //001           1 MV Half-pel
                    //000           Mixed MV
                    bit_count = 1;
                    VC1_GET_BITS(1, picLayerHeader->MVMODE);
                    while((picLayerHeader->MVMODE == 0) && (bit_count < 3))
                    {
                        VC1_GET_BITS(1, picLayerHeader->MVMODE);
                        bit_count++;
                    }
                    if (bit_count < 3)
                        picLayerHeader->MVMODE = VC1_LUT_SET(bit_count,bc_lut_5);
                    else
                        if(picLayerHeader->MVMODE == 0)
                            picLayerHeader->MVMODE = VC1_MVMODE_MIXED_MV;
                        else
                            picLayerHeader->MVMODE = VC1_MVMODE_HPEL_1MV;

                    if (picLayerHeader->CurrField)
                    {
                        if (picLayerHeader->BottomField)
                            index_top = pContext->m_frmBuff.m_iCurrIndex;
                    }

                    //INTCOMPFIELD
                    VC1_GET_BITS(1, picLayerHeader->INTCOMFIELD);
                    if(picLayerHeader->INTCOMFIELD == 1)
                    {
                        //1
                        picLayerHeader->INTCOMFIELD = VC1_INTCOMP_BOTH_FIELD;
                    }
                    else
                    {
                        VC1_GET_BITS(1, picLayerHeader->INTCOMFIELD);
                        if(picLayerHeader->INTCOMFIELD == 1)
                        {
                            //01
                            picLayerHeader->INTCOMFIELD = VC1_INTCOMP_BOTTOM_FIELD;
                        }
                        else
                        {
                            //00
                            picLayerHeader->INTCOMFIELD = VC1_INTCOMP_TOP_FIELD;
                        }
                    }

                    if(VC1_IS_INT_TOP_FIELD(picLayerHeader->INTCOMFIELD))
                    {
                        //top
                        //Luma scale
                        VC1_GET_BITS(6, picLayerHeader->LUMSCALE);
                        //Luma shift
                        VC1_GET_BITS(6, picLayerHeader->LUMSHIFT);

                    }

                    if(VC1_IS_INT_BOTTOM_FIELD(picLayerHeader->INTCOMFIELD) )
                    {
                        //bottom in case "both field"
                    // VM_ASSERT(0);
                        //Luma scale
                        VC1_GET_BITS(6, picLayerHeader->LUMSCALE1);
                        //Luma shift
                        VC1_GET_BITS(6, picLayerHeader->LUMSHIFT1);

                    }
                    pContext->m_frmBuff.m_pFrames[pContext->m_frmBuff.m_iCurrIndex].ICFieldMask |= picLayerHeader->INTCOMFIELD << (2 * (1 - picLayerHeader->CurrField));

                    pContext->m_frmBuff.m_pFrames[index_top].m_bIsExpanded = 0;

                    picLayerHeader->MVMODE2 = picLayerHeader->MVMODE;
                }
            }
    }
    else
    {
        //MVMODE VLC    Mode
        //1             1 MV
        //01            Mixed MV
        //001           1 MV Half-pel
        //0000          1 MV Half-pel bilinear
        //0001          Intensity Compensation
        int32_t bit_count = 1;
        VC1_GET_BITS(1, picLayerHeader->MVMODE);
        while((picLayerHeader->MVMODE == 0) && (bit_count < 4))
        {
            VC1_GET_BITS(1, picLayerHeader->MVMODE);
            bit_count++;
        }
        if (bit_count < 4)
            picLayerHeader->MVMODE = VC1_LUT_SET(bit_count,bc_lut_2);
        else
            if(picLayerHeader->MVMODE == 0)
                picLayerHeader->MVMODE = VC1_MVMODE_HPELBI_1MV;
            else
            {
                pContext->m_bIntensityCompensation = 1;
                picLayerHeader->MVMODE = VC1_MVMODE_INTENSCOMP;
                {

                    bit_count = 1;
                    VC1_GET_BITS(1, picLayerHeader->MVMODE);
                    while((picLayerHeader->MVMODE == 0) && (bit_count < 3))
                    {
                        VC1_GET_BITS(1, picLayerHeader->MVMODE);
                        bit_count++;
                    }
                    if (bit_count < 3)
                        picLayerHeader->MVMODE = VC1_LUT_SET(bit_count,bc_lut_4);
                    else
                        if(picLayerHeader->MVMODE == 0)
                            picLayerHeader->MVMODE = VC1_MVMODE_HPELBI_1MV;
                        else
                            picLayerHeader->MVMODE = VC1_MVMODE_HPEL_1MV;


                    //INTCOMPFIELD
                    VC1_GET_BITS(1, picLayerHeader->INTCOMFIELD);
                    if(picLayerHeader->INTCOMFIELD == 1)
                    {
                        //1
                        picLayerHeader->INTCOMFIELD = VC1_INTCOMP_BOTH_FIELD;
                    }
                    else
                    {
                        VC1_GET_BITS(1, picLayerHeader->INTCOMFIELD);
                        if(picLayerHeader->INTCOMFIELD == 1)
                        {
                            //01
                            picLayerHeader->INTCOMFIELD = VC1_INTCOMP_BOTTOM_FIELD;
                        }
                        else
                        {
                            //00
                            picLayerHeader->INTCOMFIELD = VC1_INTCOMP_TOP_FIELD;
                        }
                    }

                    if(VC1_IS_INT_TOP_FIELD(picLayerHeader->INTCOMFIELD))
                    {
                        //top
                        //Luma scale
                        VC1_GET_BITS(6, picLayerHeader->LUMSCALE);
                        //Luma shift
                        VC1_GET_BITS(6, picLayerHeader->LUMSHIFT);

                    }

                    if(VC1_IS_INT_BOTTOM_FIELD(picLayerHeader->INTCOMFIELD) )
                    {
                        //bottom in case "both field"
                    // VM_ASSERT(0);
                        //Luma scale
                        VC1_GET_BITS(6, picLayerHeader->LUMSCALE1);
                        //Luma shift
                        VC1_GET_BITS(6, picLayerHeader->LUMSHIFT1);

                    }

                    pContext->m_frmBuff.m_pFrames[pContext->m_frmBuff.m_iCurrIndex].ICFieldMask |= picLayerHeader->INTCOMFIELD << (2*(1 - picLayerHeader->CurrField));
                    picLayerHeader->MVMODE2 = picLayerHeader->MVMODE;
                }
            }
    }

    //motion vector table
    VC1_GET_BITS(3, picLayerHeader->MBMODETAB);       //MBMODETAB

    //motion vector table
    if(picLayerHeader->NUMREF)
    {
        VC1_GET_BITS(3, picLayerHeader->MVTAB);       //MVTAB
    }
    else
    {
        VC1_GET_BITS(2, picLayerHeader->MVTAB);       //MVTAB
    }

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

    return vc1Res;
}

#endif //MFX_ENABLE_VC1_VIDEO_DECODE
