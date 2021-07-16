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

#include <tuple>

#include "umc_vc1_dec_seq.h"
#include "umc_vc1_huffman.h"
#include "umc_vc1_common_tables.h"

VC1Status DecodePictureHeader_Adv(VC1Context* pContext)
{
    VC1Status vc1Sts = VC1_OK;
    uint32_t i = 0;
    uint32_t tempValue;
    uint32_t RFF = 0;
    uint32_t number_of_pan_scan_window;
    uint32_t RPTFRM = 0;

    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;

    picLayerHeader->RFF = 0;

    if(seqLayerHeader->INTERLACE)
    {
        //Frame Coding mode
        //0 - progressive; 10 - Frame-interlace; 11 - Field-interlace
        VC1_GET_BITS(1, picLayerHeader->FCM);
        if(picLayerHeader->FCM)
        {
            VC1_GET_BITS(1, picLayerHeader->FCM);
            if(picLayerHeader->FCM)
                picLayerHeader->FCM = VC1_FieldInterlace;
            else
                picLayerHeader->FCM = VC1_FrameInterlace;
        }
        else
            picLayerHeader->FCM = VC1_Progressive;
    }


    if(picLayerHeader->FCM != VC1_FieldInterlace)
    {
        //picture type
        //110 - I picture; 0 - P picture; 10 - B picture; 1110 - BI picture; 1111 - skipped
        VC1_GET_BITS(1, picLayerHeader->PTYPE);
        if(picLayerHeader->PTYPE)
        {
            VC1_GET_BITS(1, picLayerHeader->PTYPE);
            if(picLayerHeader->PTYPE)
            {
                VC1_GET_BITS(1, picLayerHeader->PTYPE);
                if(picLayerHeader->PTYPE)
                {
                    VC1_GET_BITS(1, picLayerHeader->PTYPE);
                    if(picLayerHeader->PTYPE)
                    {
                        //1111
                        picLayerHeader->PTYPE = VC1_P_FRAME | VC1_SKIPPED_FRAME;
                    }
                    else
                    {
                        //1110
                        picLayerHeader->PTYPE = VC1_BI_FRAME;
                    }
                }
                else
                {
                    //110
                    picLayerHeader->PTYPE = VC1_I_FRAME;
                }
            }
            else
            {
                //10
                picLayerHeader->PTYPE = VC1_B_FRAME;
            }
        }
        else
        {
            //0
            picLayerHeader->PTYPE = VC1_P_FRAME;
        }

        if(!VC1_IS_SKIPPED(picLayerHeader->PTYPE))
        {
            if(seqLayerHeader->TFCNTRFLAG)
            {
                //temporal reference frame counter
                VC1_GET_BITS(8, tempValue);       //TFCNTR
            }
        }

        if(seqLayerHeader->PULLDOWN)
        {
            if(!(seqLayerHeader->INTERLACE))
            {
                //repeat frame count
                VC1_GET_BITS(2, RPTFRM);
            }
            else
            {
                uint32_t tff;
                //top field first
                VC1_GET_BITS(1, tff);
                picLayerHeader->TFF = (uint8_t)tff;
                //repeat first field
                VC1_GET_BITS(1, RFF);
                picLayerHeader->RFF = (uint8_t)RFF;
            }
        }

        if(seqLayerHeader->PANSCAN_FLAG)
        {
            //pan scan present flag
            VC1_GET_BITS(1,tempValue);       //PS_PRESENT

            if(tempValue)       //PS_PRESENT
            {
                //calculate number ofpan scan window, see standard, p177
                if(seqLayerHeader->INTERLACE)
                {
                    if(seqLayerHeader->PULLDOWN)
                    {
                        number_of_pan_scan_window = 2 + RFF;
                    }
                    else
                    {
                        number_of_pan_scan_window = 2;
                    }
                }
                else
                {
                    if(seqLayerHeader->PULLDOWN)
                    {
                        number_of_pan_scan_window = 1 + RPTFRM;
                    }
                    else
                    {
                        number_of_pan_scan_window = 1;
                    }
                }

                //fill in pan scan window struture
                for (i = 0; i< number_of_pan_scan_window; i++)
                {
                    //PS_HOFFSET
                    VC1_GET_BITS(18,tempValue);
                    //PS_VOFFSET
                    VC1_GET_BITS(18,tempValue);
                    //PS_WIDTH
                    VC1_GET_BITS(14,tempValue);
                    //PS_HEIGHT
                    VC1_GET_BITS(14,tempValue);
                }
            }
        }

        if(!VC1_IS_SKIPPED(picLayerHeader->PTYPE))
        {
            //rounding control
            VC1_GET_BITS(1,picLayerHeader->RNDCTRL);

            if((seqLayerHeader->INTERLACE) || (picLayerHeader->FCM != VC1_Progressive))
            {
                //UV sampling format
                VC1_GET_BITS(1,tempValue);//UVSAMP
            }

            if(seqLayerHeader->FINTERPFLAG && (picLayerHeader->FCM == VC1_Progressive) )
            {
                //frame interpolation hint
                VC1_GET_BITS(1,tempValue);
            }

            if(picLayerHeader->PTYPE == VC1_B_FRAME && (picLayerHeader->FCM == VC1_Progressive) )
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

                if(0 == z2)
                {
                    return VC1_ERR_INVALID_STREAM;
                }
                if (z2 == VC1_BRACTION_BI)
                {
                    picLayerHeader->PTYPE = VC1_BI_FRAME;
                }

                picLayerHeader->BFRACTION = (z1*2>=z2)?1:0;
                picLayerHeader->ScaleFactor = ((256+z2/2)/z2)*z1;
                if (z1 < 8 && z2 < 9)
                    picLayerHeader->BFRACTION_index = VC1_BFraction_indexes[z1][z2];
            }

            //picture quantizer index
            VC1_GET_BITS(5,picLayerHeader->PQINDEX);

            if(picLayerHeader->PQINDEX<=8)
            {
                //half QP step
                VC1_GET_BITS(1,picLayerHeader->HALFQP);
            }
            else
                picLayerHeader->HALFQP = 0;


            if(seqLayerHeader->QUANTIZER == 1)
            {
                //picture quantizer type
                VC1_GET_BITS(1,pContext->m_picLayerHeader->PQUANTIZER);    //PQUANTIZER
            }

            CalculatePQuant(pContext);

            if(seqLayerHeader->POSTPROCFLAG)
            {
                //post processing
                VC1_GET_BITS(2,tempValue);        //POSTPROC
            }
        }
    }
    else
    {
        //FIELD INTERLACE FRAME
        DecodePictHeaderParams_InterlaceFieldPicture_Adv(pContext);
    }

    return vc1Sts;
}

typedef VC1Status (*DecoderPicHeader)(VC1Context* pContext);
static const DecoderPicHeader DecoderPicHeader_table[3][5] =
{
    {
        (DecoderPicHeader)(DecodePictHeaderParams_ProgressiveIpicture_Adv),
        (DecoderPicHeader)(DecodePictHeaderParams_ProgressivePpicture_Adv),
        (DecoderPicHeader)(DecodePictHeaderParams_ProgressiveBpicture_Adv),
        (DecoderPicHeader)(DecodePictHeaderParams_ProgressiveIpicture_Adv),
        (DecoderPicHeader)(DecodeSkippicture)
    },
    {
        (DecoderPicHeader)(DecodePictHeaderParams_InterlaceIpicture_Adv),
        (DecoderPicHeader)(DecodePictHeaderParams_InterlacePpicture_Adv),
        (DecoderPicHeader)(DecodePictHeaderParams_InterlaceBpicture_Adv),
        (DecoderPicHeader)(DecodePictHeaderParams_InterlaceIpicture_Adv),
        (DecoderPicHeader)(DecodeSkippicture)
    },
    {
        (DecoderPicHeader)(DecodeFieldHeaderParams_InterlaceFieldIpicture_Adv),
        (DecoderPicHeader)(DecodeFieldHeaderParams_InterlaceFieldPpicture_Adv),
        (DecoderPicHeader)(DecodeFieldHeaderParams_InterlaceFieldBpicture_Adv),
        (DecoderPicHeader)(DecodeFieldHeaderParams_InterlaceFieldIpicture_Adv),
        (DecoderPicHeader)(DecodeSkippicture)
    }
};

VC1Status DecodePicHeader(VC1Context* pContext)
{
   VC1Status vc1Sts = VC1_OK;

   vc1Sts = DecoderPicHeader_table[pContext->m_picLayerHeader->FCM][pContext->m_picLayerHeader->PTYPE](pContext);

   return vc1Sts;
}

VC1Status DecodeSkippicture(VC1Context* /* pContext */)
{
    return VC1_OK;
}

VC1Status DecodePictHeaderParams_InterlaceFieldPicture_Adv (VC1Context* pContext)
{
    VC1Status vc1Sts = VC1_OK;
    uint32_t i = 0;
    uint32_t tempValue;
    uint32_t RFF = 0;
    uint32_t number_of_pan_scan_window;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;
    picLayerHeader->RFF = 0;


    VC1_GET_BITS(3, tempValue);
    switch(tempValue)
    {
    case 0:
        //000  - I,I
        picLayerHeader->PTypeField1 = VC1_I_FRAME;
        picLayerHeader->PTypeField2 = VC1_I_FRAME;
        break;
    case 1:
        //001 - I,P
        picLayerHeader->PTypeField1 = VC1_I_FRAME;
        picLayerHeader->PTypeField2 = VC1_P_FRAME;
        break;
    case 2:
        //010 - P,I
        picLayerHeader->PTypeField1 = VC1_P_FRAME;
        picLayerHeader->PTypeField2 = VC1_I_FRAME;
        break;
    case 3:
        //011 - P,P
        picLayerHeader->PTypeField1 = VC1_P_FRAME;
        picLayerHeader->PTypeField2 = VC1_P_FRAME;
        break;
    case 4:
        //100 - B,B
        picLayerHeader->PTypeField1 = VC1_B_FRAME;
        picLayerHeader->PTypeField2 = VC1_B_FRAME;
        break;
    case 5:
        //101 - B,BI
        picLayerHeader->PTypeField1 = VC1_B_FRAME;
        picLayerHeader->PTypeField2 = VC1_BI_FRAME;
        break;
    case 6:
        //110 - BI,B
        picLayerHeader->PTypeField1 = VC1_BI_FRAME;
        picLayerHeader->PTypeField2 = VC1_B_FRAME;
        break;
    case 7:
        //111 - BI,BI
        picLayerHeader->PTypeField1 = VC1_BI_FRAME;
        picLayerHeader->PTypeField2 = VC1_BI_FRAME;
        break;
    default:
        VM_ASSERT(0);
        break;
    }

    if(seqLayerHeader->TFCNTRFLAG)
    {
        //temporal reference frame counter
        VC1_GET_BITS(8, tempValue);       //TFCNTR
    }

    if(seqLayerHeader->PULLDOWN)
    {
       if(!(seqLayerHeader->INTERLACE))
       {
          //repeat frame count
          VC1_GET_BITS(2,tempValue);//RPTFRM
       }
       else
       {
           uint32_t tff;
          //top field first
          VC1_GET_BITS(1, tff);
          picLayerHeader->TFF = (uint8_t)tff;
          //repeat first field
          VC1_GET_BITS(1, RFF);
          picLayerHeader->RFF = (uint8_t)RFF;
       }
    } else
        picLayerHeader->TFF = 1;

    if(seqLayerHeader->PANSCAN_FLAG)
    {
        //pan scan present flag
        VC1_GET_BITS(1,tempValue);       //PS_PRESENT

        if(tempValue)       //PS_PRESENT
        {
         //calculate number ofpan scan window, see standard, p177

          if(seqLayerHeader->PULLDOWN)
          {
            number_of_pan_scan_window = 2 + RFF;
          }
          else
          {
             number_of_pan_scan_window = 2;
          }

          //fill in pan scan window struture
          for (i = 0; i<number_of_pan_scan_window; i++)
          {
             //PS_HOFFSET
             VC1_GET_BITS(18,tempValue);
             //PS_VOFFSET
             VC1_GET_BITS(18,tempValue);
             //PS_WIDTH
             VC1_GET_BITS(14,tempValue);
             //PS_HEIGHT
             VC1_GET_BITS(14,tempValue);
           }
        }
    }
        //rounding control
        VC1_GET_BITS(1,picLayerHeader->RNDCTRL);

        //UV sampling format
        VC1_GET_BITS(1,tempValue);//UVSAMP

        if(seqLayerHeader->REFDIST_FLAG == 1 &&
            (picLayerHeader->PTypeField1 < VC1_B_FRAME &&
             picLayerHeader->PTypeField2 < VC1_B_FRAME))
        {
                int32_t ret;
                ret = DecodeHuffmanOne(
                                    &pContext->m_bitstream.pBitstream,
                                    &pContext->m_bitstream.bitOffset,
                                    &picLayerHeader->REFDIST,
                                    pContext->m_vlcTbl->REFDIST_TABLE);
                std::ignore = ret;
                VM_ASSERT(ret == 0);

                *pContext->pRefDist = picLayerHeader->REFDIST;
        }
        else if(seqLayerHeader->REFDIST_FLAG == 0)
        {
             *pContext->pRefDist = 0;
             picLayerHeader->REFDIST = 0;
        }
        else
        {
            picLayerHeader->REFDIST = 0;
        }

        if( (picLayerHeader->PTypeField1 >= VC1_B_FRAME &&
             picLayerHeader->PTypeField2 >= VC1_B_FRAME) )
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
           if(0 == z2)
                return VC1_ERR_INVALID_STREAM;

           picLayerHeader->BFRACTION = (z1*2>=z2)?1:0;
           picLayerHeader->ScaleFactor = ((256+z2/2)/z2)*z1;
            if (z1 < 8 && z2 < 9)
                picLayerHeader->BFRACTION_index = VC1_BFraction_indexes[z1][z2];
        }

    if(picLayerHeader->CurrField == 0)
    {
        picLayerHeader->PTYPE = picLayerHeader->PTypeField1;
        picLayerHeader->BottomField = (uint8_t)(1 - picLayerHeader->TFF);
    }
    else
    {
        picLayerHeader->BottomField = (uint8_t)(picLayerHeader->TFF);
        picLayerHeader->PTYPE = picLayerHeader->PTypeField2;
    }
    return vc1Sts;
}

#endif //MFX_ENABLE_VC1_VIDEO_DECODE


