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

VC1Status DecodePictureLayer_ProgressiveBpicture(VC1Context* pContext)
{
    VC1Status vc1Res = VC1_OK;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;
    picLayerHeader->RNDCTRL = seqLayerHeader->RNDCTRL;

    //3.2.1.7
    //PQINDEX is a 5-bit field that signals the quantizer scale index
    //for the entire frame. It is present in all picture types.  If the
    //implicit quantizer is used (signaled by sequence field
    //QUANTIZER = 00, see section 3.1.19) then PQINDEX specifies both
    //the picture quantizer scale (PQUANT) and the quantizer
    //(3QP or 5QP deadzone) used for the frame. Table 5 shows how
    //PQINDEX is translated to PQUANT and the quantizer for implicit mode.
    //If the quantizer is signaled explicitly at the sequence or frame
    //level (signaled by sequence field QUANTIZER = 01, 10 or 11 see
    //section 3.1.19) then PQINDEX is translated to the picture quantizer
    //stepsize PQUANT as indicated by Table 6.
    VC1_GET_BITS(5, picLayerHeader->PQINDEX);

    if(picLayerHeader->PQINDEX <= 8)
    {
        //3.2.1.8
        //HALFQP is a 1bit field present in all frame types if QPINDEX
        //is less than or equal to 8. The HALFQP field allows the picture
        //quantizer to be expressed in half step increments over the low
        //PQUANT range. If HALFQP = 1 then the picture quantizer stepsize
        //is PQUANT + ?. If HALFQP = 0 then the picture quantizer
        //stepize is PQUANT. Therefore, if the 3QP deadzone quantizer
        //is used then half stepsizes are possible up to PQUANT = 9
        //(i.e., PQUANT = 1, 1.5, 2, 2.5 8.5, 9) and then only integer
        //stepsizes are allowable above PQUANT = 9. For the 5QP deadzone
        //quantizer, half stepsizes are possible up to PQUANT = 7 (i.e.,
        //1, 1.5, 2, 2.5 6.5, 7).
        VC1_GET_BITS(1, picLayerHeader->HALFQP);
    }
    if(seqLayerHeader->QUANTIZER == 01)
    {
        //3.2.1.9
        //PQUANTIZER is a 1 bit field present in all frame types if the
        //sequence level field QUANTIZER = 01 (see section 3.1.19).
        //In this case, the quantizer used for the frame is specified by
        //PQUANTIZER. If PQUANTIZER = 0 then the 5QP deadzone quantizer
        //is used for the frame. If PQUANTIZER = 1 then the 3QP deadzone
        //quantizer is used.
        VC1_GET_BITS(1, picLayerHeader->PQUANTIZER);       //PQUANTIZER
    }

    CalculatePQuant(pContext);

    MVRangeDecode(pContext);

    //VC-1 Table 47: B Picture MVMODE codetable
    //MVMODE VLC    Mode
    //1                1 MV
    //0                1 MV Half-pel bilinear

    VC1_GET_BITS(1, picLayerHeader->MVMODE);
    picLayerHeader->MVMODE =(picLayerHeader->MVMODE==1)? VC1_MVMODE_1MV:VC1_MVMODE_HPELBI_1MV;

    //3.2.1.15
    //The DIRECTMB field is present only present in B pictures.
    //The DIRECTMB field uses bitplane coding to indicate the
    //macroblocks in the B picture that are coded in direct mode.
    //The DIRECTMB field may also signal that the direct mode is
    //signaled in raw mode in which case the direct mode is signaled
    //at the macroblock level (see section 3.2.2.11). Refer to section
    //4.10 for a description of the bitplane coding method.
    DecodeBitplane(pContext, &picLayerHeader->m_DirectMB,
                   seqLayerHeader->widthMB, seqLayerHeader->heightMB,0);
    DecodeBitplane(pContext, &picLayerHeader->SKIPMB,
                   seqLayerHeader->widthMB,seqLayerHeader->heightMB,0);

    VC1_GET_BITS(2, picLayerHeader->MVTAB);       //MVTAB
    VC1_GET_BITS(2, picLayerHeader->CBPTAB);       //CBPTAB

    vc1Res = VOPDQuant(pContext);
    if (seqLayerHeader->VSTRANSFORM == 1)
    {
        VC1_GET_BITS(1, picLayerHeader->TTMBF);

        if(picLayerHeader->TTMBF)
        {
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
#endif //MFX_ENABLE_VC1_VIDEO_DECODE
