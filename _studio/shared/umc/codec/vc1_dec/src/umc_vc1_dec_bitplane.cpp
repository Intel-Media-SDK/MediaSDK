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

#include <string.h>
#include <tuple>

#include "umc_vc1_dec_seq.h"
#include "umc_vc1_huffman.h"

//4.10    Bitplane Coding
//Certain macroblock-specific information can be encoded in one bit per
//macroblock. For example, whether or not any information is present for
// a macroblock (i.e., whether or not it is skipped) can be signaled with
// one bit. In these cases, the status for all macroblocks in a frame can
// be coded as a bitplane and transmitted in the frame header. VC1 uses
// bitplane coding in three cases to signal information about the macroblocks
// in a frame. These are: 1) signaling skipped macroblocks, 2) signaling field
// or frame macroblock mode and 3) signaling 1-MV or 4-MV motion vector mode
// for each macroblock. This section describes the bitplane coding scheme.
//Frame-level bitplane coding is used to encode two-dimensional binary arrays.
// The size of each array is rowMB ? colMB, where rowMB and colMB are the
//number of macroblock rows and columns respectively.  Within the bitstream,
// each array is coded as a set of consecutive bits.  One of seven modes is
// used to encode each array.
//The seven modes are enumerated below.
//1.    Raw mode - coded as one bit per symbol
//2.    Normal-2 mode - two symbols coded jointly
//3.    Differential-2 mode - differential coding of bitplane, followed by
//        coding two residual symbols jointly
//4.    Normal-6 mode - six symbols coded jointly
//5.    Differential-6 mode - differential coding of bitplane, followed by
//        coding six residual symbols jointly
//6.    Rowskip mode - one bit skip to signal rows with no set bits
//7.    Columnskip mode - one bit skip to signal columns with no set bits
//Section 3.3 shows the syntax elements that make up the bitplane coding scheme.
//The follow sections describe how to decode the bitstream and reconstruct the
//bitplane.



static void InverseDiff(VC1Bitplane* pBitplane, int32_t widthMB, int32_t heightMB,int32_t MaxWidthMB)
{
    int32_t i, j;

    for(i = 0; i < heightMB; i++)
    {
        for(j = 0; j < widthMB; j++)
        {
            if((i == 0 && j == 0))
            {
                pBitplane->m_databits[i*MaxWidthMB + j] = pBitplane->m_databits[i*MaxWidthMB + j] ^ pBitplane->m_invert;
            }
            else if(j == 0)
            {
                pBitplane->m_databits[i*MaxWidthMB + j] = pBitplane->m_databits[i*MaxWidthMB + j] ^pBitplane->m_databits[MaxWidthMB*(i-1)];
            }
            else if(((i>0) && (pBitplane->m_databits[i*MaxWidthMB+j-1] != pBitplane->m_databits[(i-1)*MaxWidthMB+j])))
            {
                pBitplane->m_databits[i*MaxWidthMB + j] = pBitplane->m_databits[i*MaxWidthMB + j] ^ pBitplane->m_invert;
            }
            else
            {
                pBitplane->m_databits[i*MaxWidthMB + j] = pBitplane->m_databits[i*MaxWidthMB + j] ^ pBitplane->m_databits[i*MaxWidthMB + j - 1];
            }
        }
    }

}


static void InverseBitplane(VC1Bitplane* pBitplane, int32_t size)
{
    int32_t i;

    for(i = 0; i < size; i++)
    {
        pBitplane->m_databits[i] = pBitplane->m_databits[i] ^ 1;
    }
}


static void Norm2ModeDecode(VC1Context* pContext,VC1Bitplane* pBitplane, int32_t width, int32_t height,int32_t MaxWidthMB)
{
    int32_t i;
    int32_t j,k;
    int32_t tmp_databits = 0;

    if((width*height) & 1)
    {
        VC1_GET_BITS(1, tmp_databits);
        pBitplane->m_databits[0] = (uint8_t)tmp_databits;
    }

    j = (width*height) & 1;
    k = 0;
    for(i = (width*height) & 1; i < (width*height/2)*2; i+=2)
    {
        int32_t tmp;
        int32_t index = k*MaxWidthMB + j;
        
        j++;
        if(j == width) {j = 0; k++;}

        int32_t indexNext = k*MaxWidthMB + j;

        j++;
        if(j == width) {j = 0; k++;}

        VC1_GET_BITS(1, tmp);
        if(tmp == 0)
        {
            pBitplane->m_databits[index]   = 0;
            pBitplane->m_databits[indexNext] = 0;
        }
        else
        {
            VC1_GET_BITS(1, tmp);
            if(tmp == 1)
            {
                pBitplane->m_databits[index]   = 1;
                pBitplane->m_databits[indexNext] = 1;
            }
            else
            {
                VC1_GET_BITS(1, tmp);
                if(tmp == 0)
                {
                    pBitplane->m_databits[index]   = 1;
                    pBitplane->m_databits[indexNext] = 0;
                }
                else
                {
                    pBitplane->m_databits[index]   = 0;
                    pBitplane->m_databits[indexNext] = 1;
                }
            }
        }
    }

}


static void Norm6ModeDecode(VC1Context* pContext, VC1Bitplane* pBitplane, int32_t width, int32_t height,int32_t MaxWidthMB)
{
    int ret;
    int32_t i, j;
    int32_t k;
    int32_t ResidualX = 0;
    int32_t ResidualY = 0;
    uint8_t _2x3tiled = (((width%3)!=0)&&((height%3)==0));


    if(_2x3tiled)
    {
        int32_t sizeW = width/2;
        int32_t sizeH = height/3;
        uint8_t *currRowTails =  pBitplane->m_databits;

        for(i = 0; i < sizeH; i++)
        {
            //set pointer to start of tail row
            currRowTails =  &pBitplane->m_databits[i*3*MaxWidthMB];

            //move tails start if number of MB in row is odd
            //this column bits will be decoded after
            currRowTails += width&1;

            for(j = 0; j < sizeW; j++)
            {
                ret = DecodeHuffmanOne(
                            &pContext->m_bitstream.pBitstream,
                            &pContext->m_bitstream.bitOffset,
                            &k,
                            pContext->m_vlcTbl->m_BitplaneTaledbits
                            );
                std::ignore = ret;
                VM_ASSERT(ret == 0);

                currRowTails[0] = (uint8_t)(k&1);
                currRowTails[1] = (uint8_t)((k&2)>>1);

                currRowTails[MaxWidthMB + 0] = (uint8_t)((k&4)>>2);
                currRowTails[MaxWidthMB + 1] = (uint8_t)((k&8)>>3);

                currRowTails[2*MaxWidthMB + 0] = (uint8_t)((k&16)>>4);
                currRowTails[2*MaxWidthMB + 1] = (uint8_t)((k&32)>>5);

                currRowTails+=2;

             }
        }
        ResidualX = width & 1;
        ResidualY = 0;
    }
    else //3x2 tiled
    {
        int32_t sizeW = width/3;
        int32_t sizeH = height/2;
        uint8_t *currRowTails =  pBitplane->m_databits;

        for(i = 0; i < sizeH; i++)
        {
            //set pointer to start of tail row
            currRowTails =  &pBitplane->m_databits[i*2*MaxWidthMB];

            //move tails start if number of MB in row is odd
            //this column bits will be decoded after
            currRowTails += width%3;
            currRowTails += (height&1)*width;

            for(j = 0; j < sizeW; j++)
            {
                ret = DecodeHuffmanOne(
                            &pContext->m_bitstream.pBitstream,
                            &pContext->m_bitstream.bitOffset,
                            &k,
                            pContext->m_vlcTbl->m_BitplaneTaledbits
                            );
                VM_ASSERT(ret == ippStsNoErr);

                currRowTails[0] = (uint8_t)(k&1);
                currRowTails[1] = (uint8_t)((k&2)>>1);
                currRowTails[2] = ((uint8_t)(k&4)>>2);

                currRowTails[MaxWidthMB + 0] = (uint8_t)((k&8)>>3);
                currRowTails[MaxWidthMB + 1] = (uint8_t)((k&16)>>4);
                currRowTails[MaxWidthMB + 2] = (uint8_t)((k&32)>>5);

                currRowTails+=3;

             }
        }
        ResidualX = width % 3;
        ResidualY = height & 1;
    }

    //ResidualY 0 or 1 or 2
    for(i = 0; i < ResidualX; i++)
    {
        int32_t ColSkip;
        VC1_GET_BITS(1, ColSkip);

        if(1 == ColSkip)
        {
            for(j = 0; j < height; j++)
            {
                int32_t Value = 0;
                VC1_GET_BITS(1, Value);
                pBitplane->m_databits[i + MaxWidthMB * j] = (uint8_t)Value;
            }
        }
        else
        {
            for(j = 0; j < height; j++)
            {
                pBitplane->m_databits[i + MaxWidthMB * j] = 0;
            }
        }
    }

    //ResidualY 0 or 1
    for(j = 0; j < ResidualY; j++)
    {
        int32_t RowSkip;

        VC1_GET_BITS(1, RowSkip);

        if(1 == RowSkip)
        {
            for(i = ResidualX; i < width; i++)
            {
                int32_t Value = 0;
                VC1_GET_BITS(1, Value);
                pBitplane->m_databits[i] = (uint8_t)Value;
            }
        }
        else
        {
            for(i = ResidualX; i < width; i++)
            {
                pBitplane->m_databits[i] = 0;
            }
        }
    }

}

void DecodeBitplane(VC1Context* pContext, VC1Bitplane* pBitplane, int32_t width, int32_t height,int32_t offset)
{
    int32_t tmp;
    int32_t i, j;
    int ret;
    int32_t tmp_invert = 0;
    int32_t tmp_databits = 0;

    memset(pBitplane, 0, sizeof(VC1Bitplane));

        ++pContext->bp_round_count;
    if (VC1_MAX_BITPANE_CHUNCKS == pContext->bp_round_count)
        pContext->bp_round_count = 0;

    uint32_t HeightMB = pContext->m_seqLayerHeader.heightMB;
    if(pContext->m_seqLayerHeader.INTERLACE)
        HeightMB = HeightMB + (HeightMB & 1);
    
    if (pContext->bp_round_count >= 0)
        pBitplane->m_databits = pContext->m_pBitplane.m_databits +
        HeightMB*
        pContext->m_seqLayerHeader.MaxWidthMB*pContext->bp_round_count + offset;
    else
        pBitplane->m_databits = pContext->m_pBitplane.m_databits -
        HeightMB * pContext->m_seqLayerHeader.MaxWidthMB + offset;

    //4.10.1
    //The INVERT field shown in the syntax diagram of Figure 30 is a one bit
    //code, which if set indicates that the bitplane has more set bits than
    //zero bits. Depending on INVERT and the mode, the decoder must invert
    //the interpreted bitplane to recreate the original.
    VC1_GET_BITS(1, tmp_invert);
    pBitplane->m_invert = (uint8_t)tmp_invert;

    //VC-1 Table 68: IMODE Codetable
    //CODING MODE    CODEWORD
    //Raw            0000
    //Norm-2        10
    //Diff-2        001
    //Norm-6        11
    //Diff-6        0001
    //Rowskip        010
    //Colskip        011
    ret = DecodeHuffmanOne(
                &pContext->m_bitstream.pBitstream,
                &pContext->m_bitstream.bitOffset,
                &pBitplane->m_imode,
                pContext->m_vlcTbl->m_Bitplane_IMODE
                );
    std::ignore = ret;
    VM_ASSERT(ret == 0);

    //The DATABITS field shown in the syntax diagram of Figure 28 is an entropy
    //coded stream of symbols that is based on the coding mode. The seven coding
    //modes are described in the following sections.
    switch(pBitplane->m_imode)
    {
    case VC1_BITPLANE_RAW_MODE:
        //nothing to do

        break;
    case VC1_BITPLANE_NORM2_MODE:
        //4.10.3.3    Normal-2 mode
        //If rowMB x colMB is odd, the first symbol is encoded raw.  Subsequent
        //symbols are encoded pairwise, in natural scan order.  The binary VLC
        //table in Table 57 is used to encode symbol pairs.
        //Table 57: Norm-2/Diff-2 Code Table
        //SYMBOL 2N    SYMBOL 2N + 1    CODEWORD
        //    0            0                0
        //    1            0                100
        //    0            1                101
        //    1            1                11
        Norm2ModeDecode(pContext, pBitplane, width, height,pContext->m_seqLayerHeader.MaxWidthMB);

        if(pBitplane->m_invert)
        {
            InverseBitplane(pBitplane, pContext->m_seqLayerHeader.MaxWidthMB*height);
        }

        break;
    case VC1_BITPLANE_DIFF2_MODE:

        //decode differentional bits
        Norm2ModeDecode(pContext, pBitplane, width, height,pContext->m_seqLayerHeader.MaxWidthMB);
        //restore original
        InverseDiff(pBitplane, width, height,pContext->m_seqLayerHeader.MaxWidthMB);

        break;
    case VC1_BITPLANE_NORM6_MODE:
        //In the Norm-6 and Diff-6 modes, the bitplane is encoded in groups of
        //six pixels.  These pixels are grouped into either 2x3 or 3x2 tiles.
        //The bitplane is tiled maximally using a set of rules, and the remaining
        //pixels are encoded using a variant of row-skip and column-skip modes.
        //3x2 "vertical" tiles are used if and only if rowMB is a multiple of 3
        //and colMB is not.  Else, 2x3 "horizontal" tiles are used
        Norm6ModeDecode(pContext, pBitplane, width, height,pContext->m_seqLayerHeader.MaxWidthMB);

        if(pBitplane->m_invert)
        {
            InverseBitplane(pBitplane, pContext->m_seqLayerHeader.MaxWidthMB*height);
        }
        break;
    case VC1_BITPLANE_DIFF6_MODE:

        //decode differentional bits
        Norm6ModeDecode(pContext, pBitplane, width, height,pContext->m_seqLayerHeader.MaxWidthMB);
        //restore original
        InverseDiff(pBitplane, width, height,pContext->m_seqLayerHeader.MaxWidthMB);

        break;
    case VC1_BITPLANE_ROWSKIP_MODE:
        //In the row-skip coding mode, all-zero rows are skipped with one bit overhead.
        //The syntax is as shown in Figure 79.
        //If the entire row is zero, a zero bit is sent as the ROWSKIP symbol,
        //and ROWBITS is skipped.  If there is a set bit in the row, ROWSKIP
        //is set to 1, and the entire row is sent raw (ROWBITS).  Rows are
        //scanned from the top to the bottom of the frame.
        for(i = 0; i < height; i++)
        {
            VC1_GET_BITS(1, tmp);
            if(tmp == 0)
            {
                for(j = 0; j < width; j++)
                    pBitplane->m_databits[pContext->m_seqLayerHeader.MaxWidthMB*i + j] = 0;
            }
            else
            {
                for(j = 0; j < width; j++)
                {
                    VC1_GET_BITS(1, tmp_databits);
                    pBitplane->m_databits[pContext->m_seqLayerHeader.MaxWidthMB*i + j] = (uint8_t)tmp_databits;
                }
            }
        }
        if(pBitplane->m_invert)
        {
            InverseBitplane(pBitplane, pContext->m_seqLayerHeader.MaxWidthMB*height);
        }

        break;
    case VC1_BITPLANE_COLSKIP_MODE:
        //Column-skip is the transpose of row-skip.  Columns are scanned from the
        //left to the right of the frame.
        for(i = 0; i < width; i++)
        {
            VC1_GET_BITS(1, tmp);
            if(tmp == 0)
            {
                for(j = 0; j < height; j++)
                    pBitplane->m_databits[i + j*pContext->m_seqLayerHeader.MaxWidthMB] = 0;
            }
            else
            {
                for(j = 0; j < height; j++)
                {
                    VC1_GET_BITS(1, tmp_databits);
                    pBitplane->m_databits[i + j*pContext->m_seqLayerHeader.MaxWidthMB] = (uint8_t)tmp_databits;
                }
            }
        }
        if(pBitplane->m_invert)
        {
            InverseBitplane(pBitplane, pContext->m_seqLayerHeader.MaxWidthMB*height);
        }
        break;

    }
}

#endif //MFX_ENABLE_VC1_VIDEO_DECODE
