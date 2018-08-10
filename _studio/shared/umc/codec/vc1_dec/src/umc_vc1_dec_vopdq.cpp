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

//Figure 14:  Syntax diagram for VOPDQUANT in
//(Progressive P, Interlace I and Interlace P) picture header
//3.2.1.27    VOPDQUANT Syntax Elements
VC1Status VOPDQuant(VC1Context* pContext)
{
    uint32_t tempValue;
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    uint32_t DQUANT  = pContext->m_seqLayerHeader.DQUANT;
    picLayerHeader->m_DQuantFRM = 0;
    picLayerHeader->DQSBEdge    = 0;
    picLayerHeader->m_DQBILevel = 0;

    //pContext->m_picLayerHeader->bVopdquantCoded = 1;
    if(DQUANT == 1)
    {
        //The DQUANTFRM field is a 1 bit value that is present only
        //when DQUANT = 1.  If DQUANTFRM = 0 then the current picture
        //is only quantized with PQUANT.
        VC1_GET_BITS(1, picLayerHeader->m_DQuantFRM);

        if(picLayerHeader->m_DQuantFRM == 1)
        {
            //The DQPROFILE field is a 2 bits value that is present
            //only when DQUANT = 1 and DQUANTFRM = 1.  It indicates
            //where we are allowed to change quantization step sizes
            //within the current picture.
            //Table 15:  Macroblock Quantization Profile (DQPROFILE) Code Table
            //FLC    Location
            //00    All four Edges
            //01    Double Edges
            //10    Single Edges
            //11    All Macroblocks
            VC1_GET_BITS(2,picLayerHeader->m_DQProfile);
            switch (picLayerHeader->m_DQProfile)
            {
                case VC1_DQPROFILE_ALL4EDGES:
                    picLayerHeader->m_PQuant_mode = VC1_ALTPQUANT_EDGES;
                    break;
                case VC1_DQPROFILE_SNGLEDGES:
                {
                    //uint32_t m_DQSBEdge;
                    //The DQSBEDGE field is a 2 bits value that is present
                    //when DQPROFILE = Single Edge.  It indicates which edge
                    //will be quantized with ALTPQUANT.
                    //Table 16:  Single Boundary Edge Selection (DQSBEDGE) Code Table
                    //FLC    Boundary Edge
                    //00    Left
                    //01    Top
                    //10    Right
                    //11    Bottom
                    VC1_GET_BITS(2, picLayerHeader->DQSBEdge);
                    picLayerHeader->m_PQuant_mode = 1<<picLayerHeader->DQSBEdge;
                    break;
                }
                case VC1_DQPROFILE_DBLEDGES:
                {
                    //uint32_t m_DQDBEdge;
                    //The DQSBEDGE field is a 2 bits value that is present
                    //when DQPROFILE = Double Edge.  It indicates which two
                    //edges will be quantized with ALTPQUANT.
                    //Table 17:  Double Boundary Edges Selection (DQDBEDGE) Code Table
                    //FLC    Boundary Edges
                    //00    Left and Top
                    //01    Top and Right
                    //10    Right and Bottom
                    //11    Bottom and Left
                    VC1_GET_BITS(2, picLayerHeader->DQSBEdge);
                    picLayerHeader->m_PQuant_mode = (picLayerHeader->DQSBEdge>1)?VC1_ALTPQUANT_BOTTOM:VC1_ALTPQUANT_TOP;
                    picLayerHeader->m_PQuant_mode |= ((picLayerHeader->DQSBEdge%3)? VC1_ALTPQUANT_RIGTHT:VC1_ALTPQUANT_LEFT);
                    break;
                }
                case VC1_DQPROFILE_ALLMBLKS:
                {
                    //The DQBILEVEL field is a 1 bit value that is present
                    //when DQPROFILE = All Macroblock.  If DQBILEVEL = 1,
                    //then each macroblock in the picture can take one of
                    //two possible values (PQUANT or ALTPQUANT).  If
                    //DQBILEVEL = 0, then each macroblock in the picture
                    //can take on any quantization step size.
                    VC1_GET_BITS(1, picLayerHeader->m_DQBILevel);
                    picLayerHeader->m_PQuant_mode = (picLayerHeader->m_DQBILevel)? VC1_ALTPQUANT_MB_LEVEL:VC1_ALTPQUANT_ANY_VALUE;
                    break;
                }
            }
        }
        else
            picLayerHeader->m_PQuant_mode=VC1_ALTPQUANT_NO;
    }
    else if (DQUANT == 2)
    {
        picLayerHeader->m_PQuant_mode = VC1_ALTPQUANT_EDGES;
        picLayerHeader->m_DQuantFRM = 1;
    }
    else
        picLayerHeader->m_PQuant_mode=VC1_ALTPQUANT_NO;
    //PQDIFF is a 3 bit field that encodes either the PQUANT
    //differential or encodes an escape code.
    //If PQDIFF does not equal 7 then PQDIFF encodes the
    //differential and the ABSPQ field does not follow in
    //the bitstream. In this case:
    //      ALTPQUANT = PQUANT + PQDIFF + 1
    //If PQDIFF equals 7 then the ABSPQ field follows in
    //the bitstream and ALTPQUANT is decoded as:
    //      ALTPQUANT = ABSPQ
    if (picLayerHeader->m_DQuantFRM)
    {
        if(DQUANT==2 || !(picLayerHeader->m_DQProfile == VC1_DQPROFILE_ALLMBLKS
                            && picLayerHeader->m_DQBILevel == 0))
        {
            VC1_GET_BITS(3, tempValue); //PQDIFF

            if(tempValue == 7) // escape
            {
                //ABSPQ is present in the bitstream if PQDIFF equals 7.
                //In this case, ABSPQ directly encodes the value of
                //ALTPQUANT as described above.
                VC1_GET_BITS(5, tempValue);       //m_ABSPQ

                picLayerHeader->m_AltPQuant = tempValue;       //m_ABSPQ
            }
            else
            {
                picLayerHeader->m_AltPQuant = picLayerHeader->PQUANT + tempValue + 1;
            }
        }
    }
    return VC1_OK;
}

static const uint8_t MapPQIndToQuant_Impl[] =
{
    VC1_UNDEF_PQUANT,
    1, 2, 3, 4, 5, 6, 7, 8,
    6, 7, 8, 9, 10,11,12,13,
    14,15,16,17,18,19,20,21,
    22,23,24,25,27,29,31
};

VC1Status CalculatePQuant(VC1Context* pContext)
{
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;
    VC1SequenceLayerHeader* seqLayerHeader = &pContext->m_seqLayerHeader;

    picLayerHeader->PQUANT = picLayerHeader->PQINDEX;
    picLayerHeader->QuantizationType = VC1_QUANTIZER_UNIFORM;

    if(seqLayerHeader->QUANTIZER == 0)
    {
        //If the implicit quantizer is used (signaled by sequence field
        //QUANTIZER = 00, see section 3.1.19) then PQINDEX specifies both
        //the picture quantizer scale (PQUANT) and the quantizer (3QP or
        //5QP deadzone) used for the frame. Table 5 shows how PQINDEX is
        //translated to PQUANT and the quantizer for implicit mode.
        if(picLayerHeader->PQINDEX < 9)
        {
            picLayerHeader->QuantizationType = VC1_QUANTIZER_UNIFORM;
        }
        else
        {
            picLayerHeader->QuantizationType = VC1_QUANTIZER_NONUNIFORM;
            picLayerHeader->PQUANT = MapPQIndToQuant_Impl[picLayerHeader->PQINDEX];
        }
    }
    else //01 or 10 or 11 binary
    {
        //If the quantizer is signaled explicitly at the sequence or frame
        //level (signaled by sequence field QUANTIZER = 01, 10 or 11 see
        //section 3.1.19) then PQINDEX is translated to the picture quantizer
        //stepsize PQUANT as indicated by Table 6.
        if((seqLayerHeader->QUANTIZER == 2) || ((picLayerHeader->PQUANTIZER == 0) && (seqLayerHeader->QUANTIZER == 1)))
        {
            picLayerHeader->QuantizationType = VC1_QUANTIZER_NONUNIFORM;
        }
    
    }
    return VC1_OK;
}


#endif //MFX_ENABLE_VC1_VIDEO_DECODE
