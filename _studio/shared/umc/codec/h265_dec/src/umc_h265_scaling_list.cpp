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
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#include "umc_h265_dec_defs.h"
#include "umc_h265_bitstream_headers.h"
#include "umc_h265_tables.h"

namespace UMC_HEVC_DECODER
{

// Allocate and initialize scaling list tables
void H265ScalingList::init()
{
    VM_ASSERT(!m_initialized);

    for (uint32_t sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        uint32_t scalingListNum = g_scalingListNum[sizeId];
        uint32_t scalingListSize = g_scalingListSize[sizeId];

        int16_t* pScalingList = h265_new_array_throw<int16_t>(scalingListNum * scalingListSize * SCALING_LIST_REM_NUM);

        for (uint32_t listId = 0; listId < scalingListNum; listId++)
        {
            for (uint32_t qp = 0; qp < SCALING_LIST_REM_NUM; qp++)
            {
                m_dequantCoef[sizeId][listId][qp] = pScalingList + (qp * scalingListSize);
            }
            pScalingList += (SCALING_LIST_REM_NUM * scalingListSize);
        }
    }

    //alias list [1] as [3].
    for (uint32_t qp = 0; qp < SCALING_LIST_REM_NUM; qp++)
    {
        m_dequantCoef[SCALING_LIST_32x32][3][qp] = m_dequantCoef[SCALING_LIST_32x32][1][qp];
    }

    m_initialized = true;
}

// Deallocate scaling list tables
void H265ScalingList::destroy()
{
    if (!m_initialized)
        return;

    for (uint32_t sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        delete [] m_dequantCoef[sizeId][0][0];
        m_dequantCoef[sizeId][0][0] = 0;
    }

    m_initialized = false;
}

// Calculated coefficients used for dequantization
void H265ScalingList::calculateDequantCoef(void)
{
    VM_ASSERT(m_initialized);

    static const uint32_t g_scalingListSizeX[4] = { 4, 8, 16, 32 };

    for (uint32_t sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (uint32_t listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
            for (uint32_t qp = 0; qp < SCALING_LIST_REM_NUM; qp++)
            {
                uint32_t width = g_scalingListSizeX[sizeId];
                uint32_t height = g_scalingListSizeX[sizeId];
                uint32_t ratio = g_scalingListSizeX[sizeId] / std::min(MAX_MATRIX_SIZE_NUM, (int32_t)g_scalingListSizeX[sizeId]);
                int16_t *dequantcoeff;
                int32_t *coeff = getScalingListAddress(sizeId, listId);

                dequantcoeff = getDequantCoeff(listId, qp, sizeId);
                processScalingListDec(coeff, dequantcoeff, g_invQuantScales[qp], height, width, ratio,
                    std::min(MAX_MATRIX_SIZE_NUM, (int32_t)g_scalingListSizeX[sizeId]), getScalingListDC(sizeId, listId));
            }
        }
    }
}

// Initialize scaling list with default data
void H265ScalingList::initFromDefaultScalingList()
{
    VM_ASSERT (!m_initialized);

    init();

    for (uint32_t sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
        for (uint32_t listId = 0; listId < g_scalingListNum[sizeId]; listId++)
        {
            MFX_INTERNAL_CPY(getScalingListAddress(sizeId, listId),
                getScalingListDefaultAddress(sizeId, listId),
                sizeof(int32_t) * std::min(MAX_MATRIX_COEF_NUM, (int32_t)g_scalingListSize[sizeId]));
            setScalingListDC(sizeId, listId, SCALING_LIST_DC);
        }
    }

    calculateDequantCoef();
}

// Calculated coefficients used for dequantization in one scaling list matrix
void H265ScalingList::processScalingListDec(int32_t *coeff, int16_t *dequantcoeff, int32_t invQuantScales, uint32_t height, uint32_t width, uint32_t ratio, uint32_t sizuNum, uint32_t dc)
{
    for(uint32_t j = 0; j < height; j++)
    {
        for(uint32_t i = 0; i < width; i++)
        {
            dequantcoeff[j * width + i] = (int16_t)(invQuantScales * coeff[sizuNum * (j / ratio) + i / ratio]);
        }
    }
    if(ratio > 1)
    {
        dequantcoeff[0] = (int16_t)(invQuantScales * dc);
    }
}

// Returns default scaling matrix for specified parameters
const int32_t* H265ScalingList::getScalingListDefaultAddress(unsigned sizeId, unsigned listId)
{
    const int32_t *src = 0;
    switch(sizeId)
    {
    case SCALING_LIST_4x4:
        src = g_quantTSDefault4x4;
        break;
    case SCALING_LIST_8x8:
        src = (listId<3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
        break;
    case SCALING_LIST_16x16:
        src = (listId<3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
        break;
    case SCALING_LIST_32x32:
        src = (listId<1) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
        break;
    default:
        VM_ASSERT(0);
        src = NULL;
        break;
    }
    return src;
}

// Copy data from predefined scaling matrixes
void H265ScalingList::processRefMatrix(unsigned sizeId, unsigned listId , unsigned refListId)
{
  MFX_INTERNAL_CPY(getScalingListAddress(sizeId, listId),
      ((listId == refListId) ? getScalingListDefaultAddress(sizeId, refListId) : getScalingListAddress(sizeId, refListId)),
      sizeof(int32_t)*std::min(MAX_MATRIX_COEF_NUM, (int32_t)g_scalingListSize[sizeId]));
}

} // end namespace UMC_HEVC_DECODER

#endif // MFX_ENABLE_H265_VIDEO_DECODE
