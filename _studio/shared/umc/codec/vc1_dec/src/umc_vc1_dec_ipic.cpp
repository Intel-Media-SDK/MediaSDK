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

#include <memory.h>

#include "umc_vc1_dec_seq.h"

//Simplw/Main profiles I picture
VC1Status DecodePictureLayer_ProgressiveIpicture(VC1Context* pContext)
{
    VC1Status vc1Res = VC1_OK;
    uint32_t tempValue;

    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;

    seqLayerHeader->RNDCTRL = 1;

    //4.1.1.1 The BF field is currently undefined
    //3.2.1.6 BF is a 7-bit field that is only present in I-picture
    //headers.
    VC1_GET_BITS(7, tempValue);   //BF

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
        //(i.e., PQUANT = 1, 1.5, 2, 2.5 ... 8.5, 9) and then only integer
        //stepsizes are allowable above PQUANT = 9. For the 5QP deadzone
        //quantizer, half stepsizes are possible up to PQUANT = 7 (i.e.,
        //1, 1.5, 2, 2.5 ... 6.5, 7).
        VC1_GET_BITS(1, picLayerHeader->HALFQP);
    }

    if(seqLayerHeader->QUANTIZER == 1) // 01 binary
    {
        //3.2.1.9
        //PQUANTIZER is a 1 bit field present in all frame types if the
        //sequence level field QUANTIZER = 01 (see section 3.1.19).
        //In this case, the quantizer used for the frame is specified by
        //PQUANTIZER. If PQUANTIZER = 0 then the 5QP deadzone quantizer
        //is used for the frame. If PQUANTIZER = 1 then the 3QP deadzone
        //quantizer is used.

        VC1_GET_BITS(1, picLayerHeader->PQUANTIZER);       //PQUANTIZER
        picLayerHeader->QuantizationType = 1 - picLayerHeader->PQUANTIZER;
    }
    CalculatePQuant(pContext);

    MVRangeDecode(pContext);

    if(seqLayerHeader->MULTIRES == 1 && picLayerHeader->PTYPE    != VC1_BI_FRAME)
    {
        //see 4.1.1.5 subclause for pseudo-code how the new frame dimensions
        //are calculated if a downsampled resolution is indicated

        //The RESPIC field in I pictures specifies the scaling factor of
        //the decoded I picture relative to a full resolution frame.
        //The decoded picture may be full resolution or half the original
        //resolution in either the horizontal or vertical dimensions or half
        //resolution in both dimensions. Table 8 shows how the scaling factor
        //is encoded in the RESPIC field.
        //The resolution encoded in the I picture RESPIC field also applies
        //to all subsequent P pictures until the next I picture.
        //In other words, all P pictures are encoded at the same resolution as
        //the first I picture. Although the RESPIC field is also present in P
        //picture headers, it is not used.

        //3.2.1.11
        //The RESPIC field is always present in progressive I and P pictures
        //if MULTIRES =1 in the sequence layer. This field specifies the scaling
        //factor of the current frame relative to the full resolution frame.
        //Table 8 shows the possible values of the RESPIC field. Refer to
        //section 4.1.1.5 for a description of variable resolution coding.
        //NOTE: Although the RESPIC field is present in P picture headers,
        //it is not used. See section 4.1.1.5 for a description.
        //Table 8: Progressive picture resolution code-table
        //RESPIC FLC    Horizontal Scale    Vertical Scale
        //  00                Full                Full
        //  01                Half                Full
        //  10                Full                Half
        //  11                Half                Half
        VC1_GET_BITS(2, tempValue);       //RESPIC
    }

    //4.1.1.3 subclause
    //Table 19 is used to decode the DCTACFRM and DCTACFRM2 fields
    //Refer to section 4.1.3.4 for a description of AC coefficient decoding.

    //  VLC    Coding set index
    //  0        0
    //  10        1
    //  11        2
    VC1_GET_BITS(1, picLayerHeader->TRANSACFRM);
    if(picLayerHeader->TRANSACFRM)
    {
        VC1_GET_BITS(1, picLayerHeader->TRANSACFRM);
        picLayerHeader->TRANSACFRM += 1;
    }

    VC1_GET_BITS(1, picLayerHeader->TRANSACFRM2);
    if(picLayerHeader->TRANSACFRM2)
    {
        VC1_GET_BITS(1,picLayerHeader->TRANSACFRM2);
        picLayerHeader->TRANSACFRM2 += 1;
    }


    //TRANSDCTAB is a one-bit field that signals which of two Huffman tables
    //is used to decode the Transform DC coefficients in intra-coded blocks.
    //If TRANSDCTAB = 0 then the low motion huffman table is used.
    //If TRANSDCTAB = 1 then the high motion huffman table is used.
    VC1_GET_BITS(1, picLayerHeader->TRANSDCTAB);         //TRANSDCTAB


    return vc1Res;
}

#endif //MFX_ENABLE_VC1_VIDEO_DECODE
