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

VC1Status DecodePictHeaderParams_ProgressiveIpicture_Adv(VC1Context* pContext)
{
    VC1Status vc1Res = VC1_OK;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;




    //AC Prediction
    DecodeBitplane(pContext, &picLayerHeader->ACPRED,
                   seqLayerHeader->widthMB,  seqLayerHeader->heightMB,0);

    if( (seqLayerHeader->OVERLAP==1) && (picLayerHeader->PQUANT<=8) )
    {
        //conditional overlap flag
        VC1_GET_BITS(1,picLayerHeader->CONDOVER);
        if(picLayerHeader->CONDOVER)
        {
            VC1_GET_BITS(1,picLayerHeader->CONDOVER);
            if(!picLayerHeader->CONDOVER)
            {
                //10
                picLayerHeader->CONDOVER = VC1_COND_OVER_FLAG_ALL;
            }
            else
            {
                //11
                picLayerHeader->CONDOVER = VC1_COND_OVER_FLAG_SOME;
                DecodeBitplane(pContext, &picLayerHeader->OVERFLAGS,
                            seqLayerHeader->widthMB, seqLayerHeader->heightMB,0);
            }
        }
        else
        {
            //0
            picLayerHeader->CONDOVER = VC1_COND_OVER_FLAG_NONE;
        }
    }


    //frame-level transform AC coding set index
    VC1_GET_BITS(1,picLayerHeader->TRANSACFRM);
    if(picLayerHeader->TRANSACFRM)
    {
        VC1_GET_BITS(1,picLayerHeader->TRANSACFRM);
        picLayerHeader->TRANSACFRM++;
    }


//frame-level transform AC table-2 index
    VC1_GET_BITS(1,picLayerHeader->TRANSACFRM2);
    if(picLayerHeader->TRANSACFRM2)
    {
        VC1_GET_BITS(1,picLayerHeader->TRANSACFRM2);
        picLayerHeader->TRANSACFRM2++;
    }

    //intra transform DC table
    VC1_GET_BITS(1,picLayerHeader->TRANSDCTAB);        //TRANSDCTAB

    //macroblock quantization
    vc1Res = VOPDQuant(pContext);


    return vc1Res;
}


VC1Status DecodePictHeaderParams_InterlaceIpicture_Adv(VC1Context* pContext)
{
    VC1Status vc1Res = VC1_OK;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;


     //field transform flag
    DecodeBitplane(pContext, &picLayerHeader->FIELDTX,
                   seqLayerHeader->widthMB,  seqLayerHeader->heightMB,0);

    //AC Prediction
    DecodeBitplane(pContext, &picLayerHeader->ACPRED,
                   seqLayerHeader->widthMB,  seqLayerHeader->heightMB,0);

    if( (seqLayerHeader->OVERLAP==1) && (picLayerHeader->PQUANT<=8) )
    {
        //conditional overlap flag
        VC1_GET_BITS(1,picLayerHeader->CONDOVER);
        if(picLayerHeader->CONDOVER)
        {
            VC1_GET_BITS(1,picLayerHeader->CONDOVER);
            if(!picLayerHeader->CONDOVER)
            {
                //10
                picLayerHeader->CONDOVER = VC1_COND_OVER_FLAG_ALL;
            }
            else
            {
                //11
                picLayerHeader->CONDOVER = VC1_COND_OVER_FLAG_SOME;
                //conditional overlap macroblock pattern flags
                DecodeBitplane(pContext, &picLayerHeader->OVERFLAGS,
                            seqLayerHeader->widthMB, seqLayerHeader->heightMB,0);
            }
        }
        else
        {
            //0
            picLayerHeader->CONDOVER = VC1_COND_OVER_FLAG_NONE;
        }
    }

    //frame-level transform AC coding set index
    VC1_GET_BITS(1,picLayerHeader->TRANSACFRM);
    if(picLayerHeader->TRANSACFRM)
    {
        VC1_GET_BITS(1,picLayerHeader->TRANSACFRM);
        picLayerHeader->TRANSACFRM++;
    }
//frame-level transform AC table-2 index
    VC1_GET_BITS(1,picLayerHeader->TRANSACFRM2);
    if(picLayerHeader->TRANSACFRM2)
    {
        VC1_GET_BITS(1,picLayerHeader->TRANSACFRM2);
        picLayerHeader->TRANSACFRM2++;
    }

    //intra transform DC table
    VC1_GET_BITS(1,picLayerHeader->TRANSDCTAB);        //TRANSDCTAB

    //macroblock quantization
    vc1Res = VOPDQuant(pContext);


    return vc1Res;
}

VC1Status DecodeFieldHeaderParams_InterlaceFieldIpicture_Adv(VC1Context* pContext)
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

    //AC Prediction
    if (picLayerHeader->CurrField == 0)
    DecodeBitplane(pContext, &picLayerHeader->ACPRED,
                   seqLayerHeader->widthMB, (seqLayerHeader->heightMB + 1)/2,0);
    else
    DecodeBitplane(pContext, &picLayerHeader->ACPRED,  seqLayerHeader->widthMB,
                   (seqLayerHeader->heightMB+1)/2,
                   seqLayerHeader->MaxWidthMB * ((seqLayerHeader->heightMB+1)/2));


    if( (seqLayerHeader->OVERLAP==1) && (picLayerHeader->PQUANT<=8) )
    {
        //conditional overlap flag
        VC1_GET_BITS(1,picLayerHeader->CONDOVER);
        if(picLayerHeader->CONDOVER)
        {
            VC1_GET_BITS(1,picLayerHeader->CONDOVER);
            if(!picLayerHeader->CONDOVER)
            {
                //10
                picLayerHeader->CONDOVER = VC1_COND_OVER_FLAG_ALL;
            }
            else
            {
                //11
                picLayerHeader->CONDOVER = VC1_COND_OVER_FLAG_SOME;
                //conditional overlap macroblock pattern flags

                if (picLayerHeader->CurrField == 0)
                    DecodeBitplane(pContext, &picLayerHeader->OVERFLAGS,
                    seqLayerHeader->widthMB, (seqLayerHeader->heightMB + 1)/2,0);
                else
                    DecodeBitplane(pContext, &picLayerHeader->OVERFLAGS,
                    seqLayerHeader->widthMB, (seqLayerHeader->heightMB + 1)/2,
                    seqLayerHeader->MaxWidthMB*((seqLayerHeader->heightMB + 1)/2));
            }
        }
        else
        {
            //0
            picLayerHeader->CONDOVER = VC1_COND_OVER_FLAG_NONE;
        }

    }

   //frame-level transform AC coding set index
    VC1_GET_BITS(1,picLayerHeader->TRANSACFRM);
    if(picLayerHeader->TRANSACFRM)
    {
        VC1_GET_BITS(1,picLayerHeader->TRANSACFRM);
        picLayerHeader->TRANSACFRM++;
    }

//frame-level transform AC table-2 index
    VC1_GET_BITS(1,picLayerHeader->TRANSACFRM2);
    if(picLayerHeader->TRANSACFRM2)
    {
        VC1_GET_BITS(1,picLayerHeader->TRANSACFRM2);
        picLayerHeader->TRANSACFRM2++;
    }

    //intra transform DC table
    VC1_GET_BITS(1,picLayerHeader->TRANSDCTAB);        //TRANSDCTAB

    //macroblock quantization
    vc1Res = VOPDQuant(pContext);


    return vc1Res;
}

#endif //MFX_ENABLE_VC1_VIDEO_DECODE
