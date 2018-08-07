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
#if defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "umc_vc1_dec_seq.h"
#include "umc_vc1_common_defs.h"
#include "umc_vc1_common.h"
#include "umc_structures.h"


using namespace UMC;
using namespace UMC::VC1Common;

static const uint32_t FRateExtD_tbl[] = {2,2,6,7,18,22,26};
static const uint32_t FRateExtN_tbl[] = {24,8,12,12,24,24,24};

static void reset_index(VC1Context* pContext)
{
    pContext->m_frmBuff.m_iPrevIndex  = -1;
    pContext->m_frmBuff.m_iNextIndex  = -1;
}


//3.1    Sequence-level Syntax and Semantics, figure 7
VC1Status SequenceLayer(VC1Context* pContext)
{
    uint32_t reserved;
    uint32_t i=0;
    uint32_t tempValue;

    pContext->m_seqLayerHeader.ColourDescriptionPresent = 0;

    VC1_GET_BITS(2, pContext->m_seqLayerHeader.PROFILE);
    if (!VC1_IS_VALID_PROFILE(pContext->m_seqLayerHeader.PROFILE))
        return VC1_WRN_INVALID_STREAM;

    if(pContext->m_seqLayerHeader.PROFILE == VC1_PROFILE_ADVANCED)
    {
        VC1_GET_BITS(3, pContext->m_seqLayerHeader.LEVEL);

        VC1_GET_BITS(2,tempValue);     //CHROMAFORMAT
    }
    else
    {
        //remain bites of profile
        VC1_GET_BITS(2, pContext->m_seqLayerHeader.LEVEL);
    }

    VC1_GET_BITS(3, pContext->m_seqLayerHeader.FRMRTQ_POSTPROC);

    VC1_GET_BITS(5, pContext->m_seqLayerHeader.BITRTQ_POSTPROC);

    if(pContext->m_seqLayerHeader.PROFILE == VC1_PROFILE_ADVANCED)
    {

        VC1_GET_BITS(1, pContext->m_seqLayerHeader.POSTPROCFLAG);

        VC1_GET_BITS(12, pContext->m_seqLayerHeader.MAX_CODED_WIDTH);

        VC1_GET_BITS(12, pContext->m_seqLayerHeader.MAX_CODED_HEIGHT);

        pContext->m_seqLayerHeader.CODED_HEIGHT = pContext->m_seqLayerHeader.MAX_CODED_HEIGHT;
        pContext->m_seqLayerHeader.CODED_WIDTH  = pContext->m_seqLayerHeader.MAX_CODED_WIDTH;

        uint32_t width = 0;
        uint32_t height = 0;

        width = 2*(pContext->m_seqLayerHeader.CODED_WIDTH+1);
        height = 2*(pContext->m_seqLayerHeader.CODED_HEIGHT+1);

        pContext->m_seqLayerHeader.widthMB  = (uint16_t)((width+15)/VC1_PIXEL_IN_LUMA);
        pContext->m_seqLayerHeader.heightMB = (uint16_t)((height+15)/VC1_PIXEL_IN_LUMA);

        width = 2*(pContext->m_seqLayerHeader.MAX_CODED_WIDTH+1);
        height = 2*(pContext->m_seqLayerHeader.MAX_CODED_HEIGHT+1);

        pContext->m_seqLayerHeader.MaxWidthMB  = (uint16_t)((width+15)/VC1_PIXEL_IN_LUMA);
        pContext->m_seqLayerHeader.MaxHeightMB = (uint16_t)((height+15)/VC1_PIXEL_IN_LUMA);

        VC1_GET_BITS(1, pContext->m_seqLayerHeader.PULLDOWN);

        VC1_GET_BITS(1, pContext->m_seqLayerHeader.INTERLACE);

        VC1_GET_BITS(1, pContext->m_seqLayerHeader.TFCNTRFLAG);
    }
    else
    {
        VC1_GET_BITS(1, pContext->m_seqLayerHeader.LOOPFILTER);

        VC1_GET_BITS(1, reserved);

        VC1_GET_BITS(1, pContext->m_seqLayerHeader.MULTIRES);

        VC1_GET_BITS(1, reserved);

        VC1_GET_BITS(1, pContext->m_seqLayerHeader.FASTUVMC);

        VC1_GET_BITS(1, pContext->m_seqLayerHeader.EXTENDED_MV);

        VC1_GET_BITS(2, pContext->m_seqLayerHeader.DQUANT);

        VC1_GET_BITS(1, pContext->m_seqLayerHeader.VSTRANSFORM);

        VC1_GET_BITS(1, reserved);

        VC1_GET_BITS(1, pContext->m_seqLayerHeader.OVERLAP);

        VC1_GET_BITS(1, pContext->m_seqLayerHeader.SYNCMARKER);

        VC1_GET_BITS(1, pContext->m_seqLayerHeader.RANGERED);

        VC1_GET_BITS(3, pContext->m_seqLayerHeader.MAXBFRAMES);

        VC1_GET_BITS(2, pContext->m_seqLayerHeader.QUANTIZER);
    }

    VC1_GET_BITS(1, pContext->m_seqLayerHeader.FINTERPFLAG);

    if(pContext->m_seqLayerHeader.PROFILE == VC1_PROFILE_ADVANCED)
    {
        VC1_GET_BITS(2, reserved);

        VC1_GET_BITS(1, tempValue);//DISPLAY_EXT

        if(tempValue)//DISPLAY_EXT
        {
            VC1_GET_BITS(14,tempValue);     //DISP_HORIZ_SIZE

            VC1_GET_BITS(14,tempValue);     //DISP_VERT_SIZE

            VC1_GET_BITS(1,tempValue); //ASPECT_RATIO_FLAG

            if(tempValue)
            {
                VC1_GET_BITS(4,tempValue);        //ASPECT_RATIO

                if(tempValue==15)
                {
                    VC1_GET_BITS(8, pContext->m_seqLayerHeader.AspectRatioW);        //ASPECT_HORIZ_SIZE

                    VC1_GET_BITS(8, pContext->m_seqLayerHeader.AspectRatioH);      //ASPECT_VERT_SIZE
                }
                else
                {
                    pContext->m_seqLayerHeader.AspectRatioW = 0;
                    pContext->m_seqLayerHeader.AspectRatioH = 0;
                }
            }

            VC1_GET_BITS(1,tempValue);      //FRAMERATE_FLAG

            if(tempValue)       //FRAMERATE_FLAG
            {
                VC1_GET_BITS(1,tempValue);    //FRAMERATEIND

                if(!tempValue)      //FRAMERATEIND
                {
                    VC1_GET_BITS(8, pContext->m_seqLayerHeader.FRAMERATENR);      //FRAMERATENR

                    VC1_GET_BITS(4,pContext->m_seqLayerHeader.FRAMERATEDR);  //FRAMERATEDR
                }
                else
                {
                    VC1_GET_BITS(16,tempValue);     //FRAMERATEEXP
                }

            }

            VC1_GET_BITS(1,tempValue);      //COLOR_FORMAT_FLAG

            if(tempValue)       //COLOR_FORMAT_FLAG
            {
                pContext->m_seqLayerHeader.ColourDescriptionPresent = 1;
                VC1_GET_BITS(8, tempValue);         //COLOR_PRIM
                pContext->m_seqLayerHeader.ColourPrimaries = (uint16_t)tempValue;
                VC1_GET_BITS(8, tempValue); //TRANSFER_CHAR
                pContext->m_seqLayerHeader.TransferCharacteristics = (uint16_t)tempValue;
                VC1_GET_BITS(8, tempValue);      //MATRIX_COEF
                pContext->m_seqLayerHeader.MatrixCoefficients = (uint16_t)tempValue;
            }

        }

        VC1_GET_BITS(1, pContext->m_seqLayerHeader.HRD_PARAM_FLAG);
        if(pContext->m_seqLayerHeader.HRD_PARAM_FLAG)
        {
            VC1_GET_BITS(5,pContext->m_seqLayerHeader.HRD_NUM_LEAKY_BUCKETS);
            VC1_GET_BITS(4,tempValue);//BIT_RATE_EXPONENT
            VC1_GET_BITS(4,tempValue);//BUFFER_SIZE_EXPONENT

            for(i=0; i<pContext->m_seqLayerHeader.HRD_NUM_LEAKY_BUCKETS; i++)
            {
                VC1_GET_BITS(16,tempValue);//HRD_RATE[i]
                VC1_GET_BITS(16,tempValue);//HRD_BUFFER[i]
            }
        }

    }
    // need to parse struct_B in case of simple/main profiles
    if (pContext->m_seqLayerHeader.PROFILE != VC1_PROFILE_ADVANCED)
    {
        // last in struct_C
        VC1_GET_BITS(1, tempValue);
        // skip struct_A and 0x000000C
        pContext->m_bitstream.pBitstream += 3;
        VC1_GET_BITS(3, pContext->m_seqLayerHeader.LEVEL);
        VC1_GET_BITS(1,tempValue); // CBR1
        VC1_GET_BITS(4,tempValue); // RES1
        VC1_GET_BITS(12,tempValue); // HRD_BUFFER
        VC1_GET_BITS(12,tempValue);// HRD_BUFFER
        VC1_GET_BITS(32,tempValue);// HRD_RATE
        VC1_GET_BITS(32,tempValue);// FRAME_RATE
    }

    return VC1_OK;

}

VC1Status EntryPointLayer(VC1Context* pContext)
{
    uint32_t i=0;
    uint32_t tempValue;
    uint32_t width, height;

    VC1_GET_BITS(1, pContext->m_seqLayerHeader.BROKEN_LINK);
    VC1_GET_BITS(1, pContext->m_seqLayerHeader.CLOSED_ENTRY);
    VC1_GET_BITS(1, pContext->m_seqLayerHeader.PANSCAN_FLAG);
    VC1_GET_BITS(1, pContext->m_seqLayerHeader.REFDIST_FLAG);
    VC1_GET_BITS(1, pContext->m_seqLayerHeader.LOOPFILTER);
    VC1_GET_BITS(1, pContext->m_seqLayerHeader.FASTUVMC);
    VC1_GET_BITS(1, pContext->m_seqLayerHeader.EXTENDED_MV);
    VC1_GET_BITS(2, pContext->m_seqLayerHeader.DQUANT);
    VC1_GET_BITS(1, pContext->m_seqLayerHeader.VSTRANSFORM);
    VC1_GET_BITS(1, pContext->m_seqLayerHeader.OVERLAP);
    VC1_GET_BITS(2, pContext->m_seqLayerHeader.QUANTIZER);

    if (pContext->m_seqLayerHeader.CLOSED_ENTRY)
        pContext->m_seqLayerHeader.BROKEN_LINK = 0;

    if (pContext->m_seqLayerHeader.BROKEN_LINK && !pContext->m_seqLayerHeader.CLOSED_ENTRY)
        reset_index(pContext);

    if(pContext->m_seqLayerHeader.HRD_PARAM_FLAG == 1)
    {
        for(i=0; i<pContext->m_seqLayerHeader.HRD_NUM_LEAKY_BUCKETS;i++)
        {
            VC1_GET_BITS(8, tempValue);       //m_hrd_buffer_fullness.HRD_FULLNESS[i]
        }
    }

    VC1_GET_BITS(1, tempValue);    //CODED_SIZE_FLAG
    if (tempValue == 1)
    {
        VC1_GET_BITS(12, width);
        VC1_GET_BITS(12, height);
        
        if (pContext->m_seqLayerHeader.CODED_WIDTH > pContext->m_seqLayerHeader.MAX_CODED_WIDTH)
            return VC1_FAIL;
        
        if (pContext->m_seqLayerHeader.CODED_HEIGHT > pContext->m_seqLayerHeader.MAX_CODED_HEIGHT)
            return VC1_FAIL;

        if (pContext->m_seqLayerHeader.CODED_WIDTH > width)
        {
            pContext->m_seqLayerHeader.CLOSED_ENTRY = 1;
            pContext->m_seqLayerHeader.BROKEN_LINK  = 1;
    }
        
        if (pContext->m_seqLayerHeader.CODED_HEIGHT > height)
        {
            pContext->m_seqLayerHeader.CLOSED_ENTRY = 1;
            pContext->m_seqLayerHeader.BROKEN_LINK  = 1;
        }
        
        pContext->m_seqLayerHeader.CODED_WIDTH = width;
        pContext->m_seqLayerHeader.CODED_HEIGHT = height;        
    }
    else
    {
        pContext->m_seqLayerHeader.CODED_WIDTH = pContext->m_seqLayerHeader.MAX_CODED_WIDTH;
        pContext->m_seqLayerHeader.CODED_HEIGHT = pContext->m_seqLayerHeader.MAX_CODED_HEIGHT;
    }

    width = 2*(pContext->m_seqLayerHeader.CODED_WIDTH+1);
    height = 2*(pContext->m_seqLayerHeader.CODED_HEIGHT+1);

    pContext->m_seqLayerHeader.widthMB  = (uint16_t)((width+15)/VC1_PIXEL_IN_LUMA);
    pContext->m_seqLayerHeader.heightMB = (uint16_t)((height+15)/VC1_PIXEL_IN_LUMA);

    if (pContext->m_seqLayerHeader.EXTENDED_MV == 1)
    {
        VC1_GET_BITS(1, pContext->m_seqLayerHeader.EXTENDED_DMV);
    }

    VC1_GET_BITS(1, pContext->m_seqLayerHeader.RANGE_MAPY_FLAG);   //RANGE_MAPY_FLAG
    if (pContext->m_seqLayerHeader.RANGE_MAPY_FLAG == 1)
    {
        VC1_GET_BITS(3,pContext->m_seqLayerHeader.RANGE_MAPY);
    }
    else
        pContext->m_seqLayerHeader.RANGE_MAPY = -1;

    VC1_GET_BITS(1, pContext->m_seqLayerHeader.RANGE_MAPUV_FLAG); //RANGE_MAPUV_FLAG

    if (pContext->m_seqLayerHeader.RANGE_MAPUV_FLAG == 1)
    {
        VC1_GET_BITS(3,pContext->m_seqLayerHeader.RANGE_MAPUV);
    }
    else
        pContext->m_seqLayerHeader.RANGE_MAPUV = -1;

    return VC1_OK;

}

VC1Status GetNextPicHeader_Adv(VC1Context* pContext)
{
    VC1Status vc1Sts = VC1_OK;

    memset(pContext->m_picLayerHeader, 0, sizeof(VC1PictureLayerHeader));

    vc1Sts = DecodePictureHeader_Adv(pContext);

    return vc1Sts;
}

VC1Status GetNextPicHeader(VC1Context* pContext, bool isExtHeader)
{
    VC1Status vc1Sts = VC1_OK;

    memset(pContext->m_picLayerHeader, 0, sizeof(VC1PictureLayerHeader));

    vc1Sts = DecodePictureHeader(pContext, isExtHeader);

    return vc1Sts;
}


//frame rate calculation
void MapFrameRateIntoMfx(uint32_t& ENR, uint32_t& EDR, uint16_t FCode)
{
    uint32_t FRateExtN;
    uint32_t FRateExtD;

    if (ENR && EDR)
    {
        switch (ENR)
        {
        case 1:
            ENR = 24 * 1000;
            break;
        case 2:
            ENR = 25 * 1000;
            break;
        case 3:
            ENR = 30 * 1000;
            break;
        case 4:
            ENR = 50 * 1000;
            break;
        case 5:
            ENR = 60 * 1000;
            break;
        case 6:
            ENR = 48 * 1000;
            break;
        case 7:
            ENR = 72 * 1000;
            break;
        default:
            ENR = 24 * 1000;
            break;
        }
        switch (EDR)
        {
        case 1:
            EDR = 1000;
            break;
        case 2:
            EDR = 1001;
            break;
        default:
            EDR = 1000;
            break;
        }
        return;
    }
    else
    {
        if (FCode > 6)
        {
            ENR = 0;
            EDR = 0;
            return;
        }

        FRateExtN = FRateExtN_tbl[FCode];
        FRateExtD = FRateExtD_tbl[FCode];
        ENR = FRateExtN;
        EDR = FRateExtD;

        return;
    }
 
}
double MapFrameRateIntoUMC(uint32_t ENR,uint32_t EDR, uint32_t& FCode)
{
    double frate;
    double ENRf;
    double EDRf;
    if (FCode > 6)
    {
        ENR = 0;
        EDR = 0;
        frate = (double)30;
        return frate;
    }
    else
        FCode = 2 + FCode * 4; 

    if (ENR && EDR)
    {
        FCode = 1;
        switch (ENR)
        {
        case 1:
            ENR = 24 * 1000;
            break;
        case 2:
            ENR = 25 * 1000;
            break;
        case 3:
            ENR = 30 * 1000;
            break;
        case 4:
            ENR = 50 * 1000;
            break;
        case 5:
            ENR = 60 * 1000;
            break;
        case 6:
            ENR = 48 * 1000;
            break;
        case 7:
            ENR = 72 * 1000;
            break;
        default:
            ENR = 24 * 1000;
            break;
        }
        switch (EDR)
        {
        case 1:
            EDR = 1000;
            break;
        case 2:
            EDR = 1001;
            break;
        default:
            EDR = 1000;
            break;
        }
    }
    else
    {
        ENR = 1;
        EDR = 1;
    }
    ENRf = ENR;
    EDRf = EDR;
    frate = FCode * ENRf/EDRf;
    return frate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VC1Status MVRangeDecode(VC1Context* pContext)
{
    VC1PictureLayerHeader* picLayerHeader = pContext->m_picLayerHeader;

    if (pContext->m_seqLayerHeader.EXTENDED_MV == 1)
    {
        //MVRANGE;
        //0   256 128
        //10  512 256
        //110 2048 512
        //111 4096 1024

        VC1_GET_BITS(1, picLayerHeader->MVRANGE);

        if (picLayerHeader->MVRANGE)
        {
            VC1_GET_BITS(1, picLayerHeader->MVRANGE);
            if (picLayerHeader->MVRANGE)
            {
                VC1_GET_BITS(1, picLayerHeader->MVRANGE);
                picLayerHeader->MVRANGE += 1;
            }
            picLayerHeader->MVRANGE += 1;
        }
    }
    else
    {
        picLayerHeader->MVRANGE = 0;
    }
    
    return VC1_OK;
}

VC1Status DMVRangeDecode(VC1Context* pContext)
{
    if (pContext->m_seqLayerHeader.EXTENDED_DMV == 1)
    {
        VC1_GET_BITS(1, pContext->m_picLayerHeader->DMVRANGE);
        if (pContext->m_picLayerHeader->DMVRANGE == 0)
        {
            //binary code 0
            pContext->m_picLayerHeader->DMVRANGE = VC1_DMVRANGE_NONE;
        }
        else
        {
            VC1_GET_BITS(1, pContext->m_picLayerHeader->DMVRANGE);
            if (pContext->m_picLayerHeader->DMVRANGE == 0)
            {
                //binary code 10
                pContext->m_picLayerHeader->DMVRANGE = VC1_DMVRANGE_HORIZONTAL_RANGE;
            }
            else
            {
                VC1_GET_BITS(1, pContext->m_picLayerHeader->DMVRANGE);
                if (pContext->m_picLayerHeader->DMVRANGE == 0)
                {
                    //binary code 110
                    pContext->m_picLayerHeader->DMVRANGE = VC1_DMVRANGE_VERTICAL_RANGE;
                }
                else
                {
                    //binary code 111
                    pContext->m_picLayerHeader->DMVRANGE = VC1_DMVRANGE_HORIZONTAL_VERTICAL_RANGE;
                }
            }
        }
    }

    return VC1_OK;
}

#endif //MFX_ENABLE_VC1_VIDEO_DECODE
