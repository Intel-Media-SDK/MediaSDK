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

static const uint32_t bc_lut_1[] = {2,0,1,3};
static const uint32_t bc_lut_2[] = {0,1,2,3};
static const uint32_t bc_lut_3[] = {2,0,1,3};
static const uint32_t bc_lut_4[] = {0,1,2,3};

VC1Status DecodePictureLayer_ProgressivePpicture(VC1Context* pContext)
{
    VC1Status vc1Res = VC1_OK;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;

    uint32_t tempValue;

    seqLayerHeader->RNDCTRL = 1 - seqLayerHeader->RNDCTRL;
    picLayerHeader->RNDCTRL = seqLayerHeader->RNDCTRL;


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

    if(seqLayerHeader->MULTIRES == 1)
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
        VC1_GET_BITS(2,tempValue);       //RESPIC
    }

    //3.2.1.16
    //The MVMODE field is present in P and B picture headers. For P
    //Pictures, The MVMODE field signals one of four motion vector
    //coding modes or one intensity compensation mode. Depending on
    //the value of PQUANT, either Table 10 or Table 11 is used to
    //decode the MVMODE field.
    if(picLayerHeader->PQUANT > 12)
    {
        //VC-1 Table 45: P Picture Low rate (PQUANT > 12) MVMODE codetable
        //MVMODE VLC    Mode
        //1              1 MV Half-pel bilinear
        //01             1 MV
        //001            1 MV Half-pel
        //0000           Mixed MV
        //0001           Intensity Compensation
        int32_t bit_count = 1;
        VC1_GET_BITS(1, picLayerHeader->MVMODE);
        while((picLayerHeader->MVMODE == 0) && (bit_count < 4))
        {
            VC1_GET_BITS(1,picLayerHeader->MVMODE);
            bit_count++;
        }

        if (bit_count < 4)
            picLayerHeader->MVMODE = VC1_LUT_SET(bit_count,bc_lut_1);
        else
        {
            if(picLayerHeader->MVMODE == 0)
                picLayerHeader->MVMODE = VC1_MVMODE_MIXED_MV;
            else
            {
                pContext->m_bIntensityCompensation = 1;
                //PQUANT > 12 picLayerHeader->MVMODE = VC1_MVMODE_INTENSCOMP;

                //3.2.1.17
                //The MVMODE2 field is only present in P pictures and only if the
                //picture header field MVMODE signals intensity compensation.
                //Refer to section 4.4.4.3 for a description of motion vector
                //mode/intensity compensation. Table 10 and Table 11 are used to
                //decode the MVMODE2 field.
                //VC-1 Table 48: P Picture Low rate (PQUANT > 12) MVMODE2 codetable
                //MVMODE VLC    Mode
                //1             1 MV Half-pel bilinear
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
                    picLayerHeader->MVMODE = VC1_LUT_SET(bit_count,bc_lut_3);
                else
                    if(picLayerHeader->MVMODE == 0)
                        picLayerHeader->MVMODE = VC1_MVMODE_MIXED_MV;
                    else
                        picLayerHeader->MVMODE = VC1_MVMODE_HPEL_1MV;

                //3.2.1.19
                //The LUMSCALE field is only present in P pictures and only if
                //the picture header field MVMODE signals intensity compensation
                //or INTCOMP = 1. Refer to section 4.4.8 for a description of
                //intensity compensation.
                VC1_GET_BITS(6, picLayerHeader->LUMSCALE);

                //3.2.1.20
                //The LUMSHIFT field is only present in P pictures and only if
                //the picture header field MVMODE signals intensity compensation
                //or INTCOMP = 1. Refer to section 4.4.8 for a description of
                //intensity compensation.
                VC1_GET_BITS(6, picLayerHeader->LUMSHIFT);

            }

        picLayerHeader->MVMODE2 = pContext->m_picLayerHeader->MVMODE;
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
                //PQUANT <= 12   picLayerHeader->MVMODE = VC1_MVMODE_INTENSCOMP;
                //3.2.1.17
                //The MVMODE2 field is only present in P pictures and only if the
                //picture header field MVMODE signals intensity compensation.
                //Refer to section 4.4.4.3 for a description of motion vector
                //mode/intensity compensation. Table 10 and Table 11 are used to
                //decode the MVMODE2 field.
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

                //3.2.1.19
                //The LUMSCALE field is only present in P pictures and only if
                //the picture header field MVMODE signals intensity compensation
                //or INTCOMP = 1. Refer to section 4.4.8 for a description of
                //intensity compensation.
                VC1_GET_BITS(6, picLayerHeader->LUMSCALE);

                //3.2.1.20
                //The LUMSHIFT field is only present in P pictures and only if
                //the picture header field MVMODE signals intensity compensation
                //or INTCOMP = 1. Refer to section 4.4.8 for a description of
                //intensity compensation.
                VC1_GET_BITS(6, picLayerHeader->LUMSHIFT);


                picLayerHeader->MVMODE2 = pContext->m_picLayerHeader->MVMODE;
            }
    }

    if(picLayerHeader->MVMODE  == VC1_MVMODE_MIXED_MV )
    {
        //3.2.1.21
        //The MVTYPE field is present in P pictures if MVMODE or MVMODE2
        //indicates that Mixed MV motion vector mode is used.
        //The MVTYPEMB field uses bitplane coding to signal the motion
        //vector type (1 or 4 MV) for each macroblock in the frame.
        //Refer to section 4.10 for a description of the bitplane coding
        //method. Refer to section 4.4.5.2 for a description of the motion
        //vector decode process.
        DecodeBitplane(pContext, &picLayerHeader->MVTYPEMB,
                       seqLayerHeader->widthMB, seqLayerHeader->heightMB,0);
    }

    //The SKIPMB field is present only present in P pictures.
    //The SKIPMB field encodes the skipped macroblocks in P pictures
    //using a bitplane coding method. Refer to section 4.10 for a
    //description of the bitplane coding method.
         DecodeBitplane(pContext, &picLayerHeader->SKIPMB,
                        seqLayerHeader->widthMB, seqLayerHeader->heightMB,0);

    //The MVTAB field is a 2 bit value present only in P frames.
    //The MVTAB field indicates which of four Huffman tables are used to
    //encode the motion vector data. Refer to section 4.4.5.2 for a
    //description of the motion vector decoding process.
    //Table 14: MVTAB code-table
    //FLC    Motion Vector Huffman Table
    //00    Huffman Table 0
    //10    Huffman Table 1
    //01    Huffman Table 2
    //11    Huffman Table 3
    //The motion vector Huffman tables are listed in section 5.7.
    VC1_GET_BITS(2, picLayerHeader->MVTAB);       //MVTAB

    //The CBPTAB field is a 2 bit value present only in P frames.
    //This field signals the Huffman table used to decode the CBPCY
    //field (described in section 4.4.5.2) for each coded macroblock in
    //P-pictures. Refer to section 4.4.4.6 for a description of how the
    //CBP Huffman table is used in the decoding process.
    //The CBPCY Huffman tables are listed in sections 5.2 and 5.3.
    //00    CBP Table 0
    //10    CBP Table 1
    //01    CBP Table 2
    //11    CBP Table 3
    VC1_GET_BITS(2,picLayerHeader->CBPTAB);       //CBPTAB

    vc1Res = VOPDQuant(pContext);

    if(seqLayerHeader->VSTRANSFORM == 1)
    {
        //3.2.1.28
        //This field is present only in P-picture headers and only if the
        //sequence-level field VSTRANSFORM = 1.
        //If TTMBF = 1 then the TTFRM field is also present in the picture
        //layer.
        VC1_GET_BITS(1, picLayerHeader->TTMBF);

        if(picLayerHeader->TTMBF == 1)
        {
            //This field is present in P-picture and B-frame headers if
            //VSTRANSFORM = 1 and TTMBF = 0. The TTFRM field is decoded
            //using Transform type select code-table
            //FLC    Transform type
            //00        8x8  VC1_BLK_INTER8X8 = 0x1
            //01        8x4  VC1_BLK_INTER8X4 = 0x2
            //10        4x8  VC1_BLK_INTER4X8 = 0x4
            //11        4x4  VC1_BLK_INTER4X4 = 0x8
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

    //The same index is used for Y and CbCr blocks
    //Transform AC coding set index code-table
    //  VLC    Coding set index
    //  0        0
    //  10        1
    //  11        2
    VC1_GET_BITS(1, picLayerHeader->TRANSACFRM); //TRANSACFRM
    if(picLayerHeader->TRANSACFRM == 1)
    {
        VC1_GET_BITS(1, picLayerHeader->TRANSACFRM);
        picLayerHeader->TRANSACFRM++;
    }

    //TRANSDCTAB is a one-bit field that signals which of two Huffman tables
    //is used to decode the Transform DC coefficients in intra-coded blocks.
    //If TRANSDCTAB = 0 then the low motion huffman table is used.
    //If TRANSDCTAB = 1 then the high motion huffman table is used.
    VC1_GET_BITS(1, picLayerHeader->TRANSDCTAB);       //TRANSDCTAB


    return vc1Res;
}

#endif //MFX_ENABLE_VC1_VIDEO_DECODE
