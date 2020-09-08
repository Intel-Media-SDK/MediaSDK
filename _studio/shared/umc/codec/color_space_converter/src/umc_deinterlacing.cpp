// Copyright (c) 2018-2020 Intel Corporation
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

#include "umc_deinterlacing.h"
#include "umc_video_data.h"
#include "ippi.h"
#include "ippvc.h"

using namespace UMC;

Deinterlacing::Deinterlacing()
{
  mMethod = DEINTERLACING_DUPLICATE;
}

Status Deinterlacing::SetMethod(DeinterlacingMethod method)
{
  mMethod = method;
  return UMC_OK;
}

static void DeinterlacingEdgeDetect(uint8_t *psrc,
                                    int32_t iSrcPitch,
                                    uint8_t *pdst,
                                    int32_t iDstPitch,
                                    int32_t w,
                                    int32_t h);

Status Deinterlacing::GetFrame(MediaData *input, MediaData *output)
{
  VideoData *in = DynamicCast<VideoData>(input);
  VideoData *out = DynamicCast<VideoData>(output);
  DeinterlacingMethod method = mMethod;
  int k;

  if (NULL == in || NULL == out) {
    return UMC_ERR_NULL_PTR;
  }

  ColorFormat cFormat = in->GetColorFormat();
  if (out->GetColorFormat() != cFormat) {
    return UMC_ERR_INVALID_PARAMS;
  }
  int32_t in_Width = in->GetWidth();
  int32_t in_Height = in->GetHeight();
  int32_t out_Width = out->GetWidth();
  int32_t out_Height = out->GetHeight();
  if ( (in_Width != out_Width) || (in_Height != out_Height) ) {
    return UMC_ERR_INVALID_PARAMS;
  }

  for (k = 0; k < in->GetNumPlanes(); k++) {
    VideoData::PlaneInfo srcPlane = {};
    const uint8_t *pSrc0; //, *pSrc1;
    uint8_t *pDst0, *pDst1;
    int srcPitch, dstPitch;
    mfxSize size;

    in->GetPlaneInfo(&srcPlane, k);
    pSrc0 = (const uint8_t*)in->GetPlanePointer(k);
    srcPitch = (int32_t)in->GetPlanePitch(k);
    pDst0 = (uint8_t*)out->GetPlanePointer(k);
    dstPitch = (int32_t)out->GetPlanePitch(k);
    size.width = srcPlane.m_ippSize.width * srcPlane.m_iSamples * srcPlane.m_iSampleSize;
    size.height = srcPlane.m_ippSize.height;

    if (method == DEINTERLACING_BLEND) {
      if (srcPlane.m_iSampleSize != 1) {
        //return UMC_ERR_UNSUPPORTED;
        method = DEINTERLACING_DUPLICATE;
      }
      mfxiDeinterlaceFilterTriangle_8u_C1R(pSrc0, srcPitch,
                                           pDst0, dstPitch,
                                           size,
                                           128,
                                           IPP_LOWER | IPP_UPPER | IPP_CENTER);
      continue;
    }

    if (method == DEINTERLACING_SPATIAL) {
      if (k == 0) { // Y
        if (srcPlane.m_iSampleSize != 1 || srcPlane.m_iSamples != 1) {
          //return UMC_ERR_UNSUPPORTED;
          method = DEINTERLACING_DUPLICATE;
        }
        DeinterlacingEdgeDetect((uint8_t*)pSrc0, srcPitch,
                                pDst0, dstPitch,
                                size.width, size.height);
        continue;
      } else { // U,V
        method = DEINTERLACING_DUPLICATE;
      }
    }

    // DEINTERLACING_DUPLICATE
    //pSrc1 = pSrc0 += srcPitch;
    pSrc0 += srcPitch;
    srcPitch *= 2;
    pDst1 = pDst0 + dstPitch;
    dstPitch *= 2;
    size.height /= 2;
    mfxiCopy_8u_C1R(pSrc0, srcPitch, pDst0, dstPitch, size);
    mfxiCopy_8u_C1R(pSrc0, srcPitch, pDst1, dstPitch, size);
  }

  return UMC_OK;
}

static void DeinterlacingEdgeDetect(uint8_t *psrc,
                                    int32_t iSrcPitch,
                                    uint8_t *pdst,
                                    int32_t iDstPitch,
                                    int32_t w,
                                    int32_t h)
{
  int32_t /*hi, lo,*/ x, y;
  int32_t res;
  uint8_t *pInA, *pInB, *pInC, *pInD, *pInE, *pInF;
  int32_t dif1, dif2, dif3;
  mfxSize roi = {w, h/2};

  // copy even lines
  mfxiCopy_8u_C1R(psrc, 2*iSrcPitch, pdst, 2*iDstPitch, roi);

  // then line #1 and line before last
  std::copy(psrc + iSrcPitch, psrc + 2*iSrcPitch, pdst + iDstPitch);
  std::copy(psrc + (h - 2) * iSrcPitch, psrc + (h - 2) * iSrcPitch + w, pdst + (h - 1)*iDstPitch);

  psrc += 3*iSrcPitch;
  pdst += 3*iDstPitch;
  pInB = psrc - iSrcPitch;
  pInA = pInB - 1;
  pInC = pInB + 1;
  pInE = psrc + iSrcPitch;
  pInD = pInE - 1;
  pInF = pInE + 1;


  for (y = 3; y <= h - 3; y += 2) {
    pdst[0] = psrc[0];
    pdst[w - 1] = psrc[w - 1];

    for (x = 1; x < w - 1; x++)
    {
      dif1 = abs((int32_t)pInA[x] - (int32_t)pInF[x]);
      dif2 = abs((int32_t)pInC[x] - (int32_t)pInD[x]);
      dif3 = abs((int32_t)pInB[x] - (int32_t)pInE[x]);

      if (dif1 < dif2) {
        if (dif1 < dif3) {
          res = ((int32_t)pInA[x] + (int32_t)pInF[x]) >> 1; //1
        } else {
          res = ((int32_t)pInB[x] + (int32_t)pInE[x]) >> 1; //3
        }
      } else {
        if (dif2 < dif3) {
          res = ((int32_t)pInC[x] + (int32_t)pInD[x]) >> 1; //2
        } else {
          res = ((int32_t)pInB[x] + (int32_t)pInE[x]) >> 1; //3
        }
      }
      pdst[x] = (uint8_t)res;
    }
    psrc += 2*iSrcPitch;
    pdst += 2*iDstPitch;
    pInA += 2*iSrcPitch;
    pInB += 2*iSrcPitch;
    pInC += 2*iSrcPitch;
    pInD += 2*iSrcPitch;
    pInE += 2*iSrcPitch;
    pInF += 2*iSrcPitch;
  }
}
