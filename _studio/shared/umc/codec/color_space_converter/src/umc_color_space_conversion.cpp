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

#include "umc_color_space_conversion.h"
#include "umc_video_data.h"
#include "ippi.h"
#include "ippcc.h"
#include "ippvc.h"

using namespace UMC;

template <class T> inline
void SwapValues(T &one, T& two)
{
  T tmp;
  tmp = one;
  one = two;
  two = tmp;
}

static void ConvertImage_16s8u_C1R(const int16_t *pSrc,
                                  int32_t   iSrcStride,
                                  int32_t   iSrcBitsPerSample,
                                  uint8_t    *pDst,
                                  int32_t   iDstStride,
                                  mfxSize size);
static IppStatus cc_BGRAToBGR(const uint8_t *pSrc,
                              int32_t   iSrcStride,
                              uint8_t    *pDst,
                              int32_t   iDstStride,
                              mfxSize srcSize);
static IppStatus cc_BGRToBGRA(const uint8_t *pSrc,
                              int32_t   iSrcStride,
                              uint8_t    *pDst,
                              int32_t   iDstStride,
                              mfxSize srcSize);
static IppStatus cc_BGR555ToBGR(const uint16_t *pSrc,
                                int32_t   iSrcStride,
                                uint8_t    *pDst,
                                int32_t   iDstStride,
                                mfxSize srcSize);
static IppStatus cc_BGR565ToBGR(const uint16_t *pSrc,
                                int32_t   iSrcStride,
                                uint8_t    *pDst,
                                int32_t   iDstStride,
                                mfxSize srcSize);
static IppStatus cc_Y41P_to_I420(const uint8_t *pSrc,
                                 int32_t   iSrcStride,
                                 uint8_t    **pDst,
                                 int32_t   *iDstStride,
                                 mfxSize srcSize);
static IppStatus cc_I420_to_Y41P(const uint8_t **pSrc,
                                 int32_t   *iSrcStride,
                                 uint8_t    *pDst,
                                 int32_t   iDstStride,
                                 mfxSize srcSize);

static IppStatus cc_YUV411_to_YUV420(const uint8_t *pSrc[3],
                                     int32_t   iSrcStride[3],
                                     uint8_t    *pDst[3],
                                     int32_t   iDstStride[3],
                                     mfxSize srcSize);

static IppStatus cc_RGB3_to_NV12(const uint8_t *pSrc,
                       int32_t   iSrcStride,
                       uint8_t* pDst[2],
                       int32_t dstStep[2],
                       mfxSize srcSize,
                       uint8_t isInterlace);

static IppStatus cc_RGB4_to_NV12(const uint8_t *pSrc,
                       int32_t   iSrcStride,
                       uint8_t* pDst[2],
                       int32_t dstStep[2],
                       mfxSize srcSize,
                       uint8_t isInterlace);

static Status CopyImage(VideoData *pSrc, VideoData *pDst, int flag, int bSwapUV)
{
  VideoData::PlaneInfo src = {};
  VideoData::PlaneInfo dst = {};
  mfxSize size = {};
  int cPlanes = 0;
  int iDstPlane = 0;
  int sts = ippStsNoErr;

  cPlanes = pSrc->GetNumPlanes();
  if (cPlanes > pDst->GetNumPlanes()) cPlanes = pDst->GetNumPlanes();

  for (iDstPlane = 0; iDstPlane < cPlanes; iDstPlane++) {
    int iSrcPlane = iDstPlane;
    if (bSwapUV) {
      if (iDstPlane == 1) iSrcPlane = 2; else
      if (iDstPlane == 2) iSrcPlane = 1;
    }

    pSrc->GetPlaneInfo(&src, iSrcPlane);
    pDst->GetPlaneInfo(&dst, iDstPlane);

    size.width = src.m_ippSize.width * src.m_iSamples;
    size.height = src.m_ippSize.height;

    if (src.m_iSampleSize == dst.m_iSampleSize) {
      size.width *= src.m_iSampleSize;
      if (flag == 2 && src.m_iBitDepth >= 0) { // case VC1->YUV420
        sts = mfxiRangeMapping_VC1_8u_C1R(src.m_pPlane, (int32_t)src.m_nPitch, dst.m_pPlane, (int32_t)dst.m_nPitch, size, src.m_iBitDepth);
      } else {
        sts = mfxiCopy_8u_C1R(src.m_pPlane, (int32_t)src.m_nPitch, dst.m_pPlane, (int32_t)dst.m_nPitch, size);
      }
    } else if (src.m_iSampleSize == 2 && dst.m_iSampleSize == 1) {
      ConvertImage_16s8u_C1R((const int16_t*)src.m_pPlane, (int32_t)src.m_nPitch, src.m_iBitDepth, dst.m_pPlane, (int32_t)dst.m_nPitch, size);
    } else {
      return UMC_ERR_UNSUPPORTED;
    }
  }

  return (ippStsNoErr == sts) ? UMC_OK : UMC_ERR_FAILED;
}

int cc_IMC3_to_YUV420(const uint8_t *pSrc[3], int32_t pSrcStep[3], uint8_t *pDst[3], int32_t pDstStep[3], mfxSize srcSize)
{
    mfxSize roi[3] = {
        {srcSize.width, srcSize.height},
        {srcSize.width >> 1, srcSize.height >> 1},
        {srcSize.width >> 1, srcSize.height >> 1}
    };

    for (int32_t c = 0; c < 3; c++)
    {
        mfxiCopy_8u_C1R(
            pSrc[c],
            pSrcStep[c],
            pDst[c],
            pDstStep[c],
            roi[c]);
    }
    return ippStsNoErr;
}

Status ColorSpaceConversion::GetFrame(MediaData *input, MediaData *output)
{
    VideoData *in = DynamicCast<VideoData>(input);
    VideoData *out = DynamicCast<VideoData>(output);

    if (NULL == in || NULL == out)
    {
        return UMC_ERR_NULL_PTR;
    }

    Status sts = UMC_OK;

    if (in->GetPictureStructure() != PS_FRAME && out->GetPictureStructure() != PS_FRAME)
    {
        VideoData in_interlace;
        VideoData out_interlace;

        in_interlace = *in;
        out_interlace = *out;

        PictureStructure inTemp = in_interlace.GetPictureStructure();
        PictureStructure outTemp = out_interlace.GetPictureStructure();

        in_interlace.SetPictureStructure(PS_FRAME);
        out_interlace.SetPictureStructure(PS_FRAME);

        in_interlace.ConvertPictureStructure(PS_TOP_FIELD);
        out_interlace.ConvertPictureStructure(PS_TOP_FIELD);

        sts = GetFrameInternal(&in_interlace, &out_interlace);
        if (sts == UMC_OK)
        {
            in_interlace.ConvertPictureStructure(PS_BOTTOM_FIELD);
            out_interlace.ConvertPictureStructure(PS_BOTTOM_FIELD);

            sts = GetFrameInternal(&in_interlace, &out_interlace);
        }

        in_interlace.SetPictureStructure(inTemp);
        out_interlace.SetPictureStructure(outTemp);
    }
    else
    {
        sts = GetFrameInternal(input, output);
    }

    return sts;
}

Status ColorSpaceConversion::GetFrameInternal(MediaData *input, MediaData *output)
{
  VideoData *in = DynamicCast<VideoData>(input);
  VideoData *out = DynamicCast<VideoData>(output);

  if (NULL == in || NULL == out) {
    return UMC_ERR_NULL_PTR;
  }

  mfxSize srcSize = {in->GetWidth(), in->GetHeight()};
  mfxSize dstSize = {out->GetWidth(), out->GetHeight()};
  if (srcSize.width != dstSize.width || srcSize.height != dstSize.height) {
    return UMC_ERR_INVALID_PARAMS;
  }

  ColorFormat srcFormat = in->GetColorFormat();
  ColorFormat dstFormat = out->GetColorFormat();
  int bSrcSwapUV = 0;
  int bDstSwapUV = 0;

  if (srcFormat == YV12) // process YV12 as YUV420
  {
    bSrcSwapUV = 1;
    srcFormat = YUV420;
  }
  if (dstFormat == YV12) // process YV12 as YUV420
  {
    bDstSwapUV = 1;
    dstFormat = YUV420;
  }

  int flag_OnlyCopy = 0;
  if (srcFormat == dstFormat) flag_OnlyCopy = 1;
  if (YUV_VC1 == srcFormat && YUV420 == dstFormat) flag_OnlyCopy = 2;
  if (YUV420A == srcFormat && YUV420 == dstFormat) flag_OnlyCopy = 3;
  if (GRAYA == srcFormat && GRAY == dstFormat) flag_OnlyCopy = 4;
  if (flag_OnlyCopy) {
      return CopyImage(in, out, flag_OnlyCopy, bSrcSwapUV ^ bDstSwapUV);
  }

  const uint8_t *pSrc[3] = {(uint8_t*)in->GetPlanePointer(0),
                            (uint8_t*)in->GetPlanePointer(1),
                            (uint8_t*)in->GetPlanePointer(2)};
  int32_t pSrcStep[3] = {(int32_t)in->GetPlanePitch(0),
                        (int32_t)in->GetPlanePitch(1),
                        (int32_t)in->GetPlanePitch(2)};
  uint8_t *pDst[3] = {(uint8_t*)out->GetPlanePointer(0),
                      (uint8_t*)out->GetPlanePointer(1),
                      (uint8_t*)out->GetPlanePointer(2)};
  int32_t pDstStep[3] = {(int32_t)out->GetPlanePitch(0),
                        (int32_t)out->GetPlanePitch(1),
                        (int32_t)out->GetPlanePitch(2)};
  if (bSrcSwapUV) {
    SwapValues(pSrc[1], pSrc[2]);
    SwapValues(pSrcStep[1], pSrcStep[2]);
  }
  if (bDstSwapUV) {
    SwapValues(pDst[1], pDst[2]);
    SwapValues(pDstStep[1], pDstStep[2]);
  }
  if (srcFormat == YUV422 && dstFormat != YUV420) { // 422->X as 420->X
    pSrcStep[1] *= 2;
    pSrcStep[2] *= 2;
    srcFormat = YUV420;
  }
  const uint8_t *pYVU[3] = {pSrc[0], pSrc[2], pSrc[1]};
  int32_t pYVUStep[3] = {pSrcStep[0], pSrcStep[2], pSrcStep[1]};
  int status;

  switch (srcFormat) {
  case IMC3:
    switch (dstFormat) {
    case YUV420:
      //status = ippiYCbCr411ToYCbCr420_8u_P3R(pSrc, pSrcStep, pDst, pDstStep, srcSize);

        status = cc_IMC3_to_YUV420(pSrc, pSrcStep, pDst, pDstStep, srcSize);
      break;
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  case YUV411:
    switch (dstFormat) {
    case YUV420:
      //status = ippiYCbCr411ToYCbCr420_8u_P3R(pSrc, pSrcStep, pDst, pDstStep, srcSize);
      status = cc_YUV411_to_YUV420(pSrc, pSrcStep, pDst, pDstStep, srcSize);
      break;
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  case YUV420:
    switch (dstFormat) {
    case YUV422:
      status = mfxiYCbCr420ToYCbCr422_8u_P3R(pSrc, pSrcStep, pDst, pDstStep, srcSize);
      break;
    case Y41P:
      status = cc_I420_to_Y41P(pYVU, pYVUStep, pDst[0], pDstStep[0], srcSize);
      break;
    case YUY2:
      status = mfxiYCrCb420ToYCbCr422_8u_P3C2R(pYVU, pYVUStep, pDst[0], pDstStep[0], srcSize);
      break;
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  case YUV422:
  case YUV422A:
    switch (dstFormat) {
    case YUV420:
      status = mfxiYCbCr422ToYCbCr420_8u_P3R(pSrc, pSrcStep, pDst, pDstStep, srcSize);
      break;
    case YUY2:
      status = mfxiYCbCr422_8u_P3C2R(pSrc, pSrcStep, pDst[0], pDstStep[0], srcSize);
      break;
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  case YUV444:
    switch (dstFormat) {
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  case YUY2:
    switch (dstFormat) {
    case YUV420:
      status = mfxiYCbCr422ToYCbCr420_8u_C2P3R(pSrc[0], pSrcStep[0], pDst, pDstStep, srcSize);
      break;
    case YUV422:
      status = mfxiYCbCr422_8u_C2P3R( pSrc[0], pSrcStep[0], pDst, pDstStep, srcSize);
      break;
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  case UYVY:
    switch (dstFormat) {
    case YUV422:
      status = mfxiCbYCr422ToYCbCr422_8u_C2P3R( pSrc[0], pSrcStep[0], pDst, pDstStep, srcSize);
      break;
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  case NV12:
    switch (dstFormat) {
    case YUV420:
      status = mfxiYCbCr420_8u_P2P3R(pSrc[0], pSrcStep[0], pSrc[1], pSrcStep[1], pDst, pDstStep, srcSize);
      break;
    case YUY2:
      status = mfxiYCbCr420ToYCbCr422_8u_P2C2R(pSrc[0], pSrcStep[0], pSrc[1], pSrcStep[1], pDst[0], pDstStep[0], srcSize);
      break;
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  case RGB24:
    switch (dstFormat) {
    case RGB32:
      status = cc_BGRToBGRA(pSrc[0], pSrcStep[0], pDst[0], pDstStep[0], srcSize);
      break;
    case NV12:
      status = cc_RGB3_to_NV12(pSrc[0], pSrcStep[0], pDst, pDstStep, srcSize, false);
      break;
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  case RGB32:
    switch (dstFormat) {
    case RGB24:
      status = cc_BGRAToBGR(pSrc[0], pSrcStep[0], pDst[0], pDstStep[0], srcSize);
      break;
    case NV12:
      status = cc_RGB4_to_NV12(pSrc[0], pSrcStep[0], pDst, pDstStep, srcSize, false);
      break;
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  case RGB555:
    switch (dstFormat) {
    case RGB24:
      status = cc_BGR555ToBGR((const uint16_t*)pSrc[0], pSrcStep[0], pDst[0], pDstStep[0], srcSize);
      break;
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  case RGB565:
    switch (dstFormat) {
    case RGB24:
      status = cc_BGR565ToBGR((const uint16_t*)pSrc[0], pSrcStep[0], pDst[0], pDstStep[0], srcSize);
      break;
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  case Y41P:
    switch (dstFormat) {
    case YUV420:
      status = cc_Y41P_to_I420(pSrc[0], pSrcStep[0], pDst, pDstStep, srcSize);
      break;
    default:
      return UMC_ERR_NOT_IMPLEMENTED;
    }
    break;
  default:
    return UMC_ERR_NOT_IMPLEMENTED;
  }

  return (status == ippStsNoErr) ? UMC_OK : UMC_ERR_INVALID_PARAMS;
}

////////////////////////////////////////////////////////////////////////

static void ConvertImage_16s8u_C1R(const int16_t *pSrc,
                                  int32_t   iSrcStride,
                                  int32_t   iSrcBitsPerSample,
                                  uint8_t    *pDst,
                                  int32_t   iDstStride,
                                  mfxSize size)
{
  int iWidth = size.width;
  int iHeight = size.height;
  int rnd = (1 << (iSrcBitsPerSample - 8 - 1));
  int32_t x, y;

  for (y = 0; y < iHeight; y += 1) {
    for (x = 0; x < iWidth; x += 1) {
      pDst[x] = (uint8_t) ((pSrc[x] + rnd) >> (iSrcBitsPerSample - 8));
    }
    pSrc = (int16_t *) ((uint8_t *) pSrc + iSrcStride);
    pDst = pDst + iDstStride;
  }
}

IppStatus cc_BGRAToBGR(const uint8_t *pSrc,
                       int32_t   iSrcStride,
                       uint8_t    *pDst,
                       int32_t   iDstStride,
                       mfxSize srcSize)
{
  int i, j ;

  for (i = 0; i < srcSize.height; i++) {
    for (j = 0; j < srcSize.width; j++) {
      pDst[3*j + 0] = pSrc[4*j + 0];
      pDst[3*j + 1] = pSrc[4*j + 1];
      pDst[3*j + 2] = pSrc[4*j + 2];
    }
    pSrc += iSrcStride;
    pDst += iDstStride;
  }
  return ippStsNoErr;
}

IppStatus cc_BGRToBGRA(const uint8_t *pSrc,
                       int32_t   iSrcStride,
                       uint8_t    *pDst,
                       int32_t   iDstStride,
                       mfxSize srcSize)
{
  int i, j ;

  for (i = 0; i < srcSize.height; i++) {
    for (j = 0; j < srcSize.width; j++) {
      pDst[4*j + 0] = pSrc[3*j + 0];
      pDst[4*j + 1] = pSrc[3*j + 1];
      pDst[4*j + 2] = pSrc[3*j + 2];
      pDst[4*j + 3] = 0xFF;
    }
    pSrc += iSrcStride;
    pDst += iDstStride;
  }
  return ippStsNoErr;
}

IppStatus cc_BGR555ToBGR(const uint16_t *pSrc,
                         int32_t   iSrcStride,
                         uint8_t    *pDst,
                         int32_t   iDstStride,
                         mfxSize srcSize)
{
  int i, j;
  for (i = 0; i < srcSize.height; i++) {
    for (j = 0; j < srcSize.width; j++) {
      uint16_t pix = pSrc[j];
      //pDst[3*j + 0] = (uint8_t)((pix >> 10) & 0x1f);
      //pDst[3*j + 1] = (uint8_t)((pix >> 5) & 0x1f);
      //pDst[3*j + 2] = (uint8_t)((pix & 0x1f) << 3);
      pDst[3*j + 2] = (uint8_t)((pix & 0x7c00) >> 7);
      pDst[3*j + 1] = (uint8_t)((pix & 0x03e0) >> 2);
      pDst[3*j + 0] = (uint8_t)((pix & 0x001f) << 3);
    }
    pSrc = (uint16_t *) ((uint8_t *) pSrc + iSrcStride);
    pDst += iDstStride;
  }
  return ippStsNoErr;
}

IppStatus cc_BGR565ToBGR(const uint16_t *pSrc,
                         int32_t   iSrcStride,
                         uint8_t    *pDst,
                         int32_t   iDstStride,
                         mfxSize srcSize)
{
  int i, j;
  for (i = 0; i < srcSize.height; i++) {
    for (j = 0; j < srcSize.width; j++) {
      uint16_t pix = pSrc[j];
      pDst[3*j + 2] = (uint8_t)((pix & 0xf800) >> 8);
      //pDst[3*j + 0] = (uint8_t)((pix & 0x001f) << 3);
      pDst[3*j + 1] = (uint8_t)((pix & 0x07e0) >> 3);
      pDst[3*j + 0] = (uint8_t)((pix & 0x001f) << 3);
      //pDst[3*j + 2] = (uint8_t)((pix & 0xf800) >> 8);
    }
    pSrc = (uint16_t *) ((uint8_t *) pSrc + iSrcStride);
    pDst += iDstStride;
  }
  return ippStsNoErr;
}

IppStatus cc_Y41P_to_I420(const uint8_t *pSrc,
                          int32_t   iSrcStride,
                          uint8_t    **pDst,
                          int32_t   *iDstStride,
                          mfxSize srcSize)
{
  uint8_t *Y = pDst[0];
  uint8_t *U = pDst[1];
  uint8_t *V = pDst[2];
  int Y_stride = iDstStride[0];
  int U_stride = iDstStride[1];
  int V_stride = iDstStride[2];
  int i, j;

  for (j = 0; j < srcSize.height; j++) {
    for (i = 0; i < srcSize.width/8; i++) {
      int U0 = pSrc[12*i + 0];
      int Y0 = pSrc[12*i + 1];
      int V0 = pSrc[12*i + 2];
      int Y1 = pSrc[12*i + 3];
      int U4 = pSrc[12*i + 4];
      int Y2 = pSrc[12*i + 5];
      int V4 = pSrc[12*i + 6];
      int Y3 = pSrc[12*i + 7];
      int Y4 = pSrc[12*i + 8];
      int Y5 = pSrc[12*i + 9];
      int Y6 = pSrc[12*i + 10];
      int Y7 = pSrc[12*i + 11];
      Y[8*i + 0] = (uint8_t)Y0;
      Y[8*i + 1] = (uint8_t)Y1;
      Y[8*i + 2] = (uint8_t)Y2;
      Y[8*i + 3] = (uint8_t)Y3;
      Y[8*i + 4] = (uint8_t)Y4;
      Y[8*i + 5] = (uint8_t)Y5;
      Y[8*i + 6] = (uint8_t)Y6;
      Y[8*i + 7] = (uint8_t)Y7;
      if (!(j & 1)) {
        U[4*i + 0] = (uint8_t)U0;
        U[4*i + 1] = (uint8_t)U0;
        U[4*i + 2] = (uint8_t)U4;
        U[4*i + 3] = (uint8_t)U4;
        V[4*i + 0] = (uint8_t)V0;
        V[4*i + 1] = (uint8_t)V0;
        V[4*i + 2] = (uint8_t)V4;
        V[4*i + 3] = (uint8_t)V4;
      }
    }
    pSrc += iSrcStride;
    Y += Y_stride;
    if (j & 1) {
      U += U_stride;
      V += V_stride;
    }
  }
  return ippStsNoErr;
}

IppStatus cc_I420_to_Y41P(const uint8_t **pSrc,
                          int32_t   *iSrcStride,
                          uint8_t    *pDst,
                          int32_t   iDstStride,
                          mfxSize srcSize)
{
  const uint8_t *Y = pSrc[0];
  const uint8_t *U = pSrc[1];
  const uint8_t *V = pSrc[2];
  int Y_stride = iSrcStride[0];
  int U_stride = iSrcStride[1];
  int V_stride = iSrcStride[2];
  int i, j;

  for (j = 0; j < srcSize.height; j++) {
    for (i = 0; i < srcSize.width/8; i++) {
      int Y0 = Y[8*i + 0];
      int Y1 = Y[8*i + 1];
      int Y2 = Y[8*i + 2];
      int Y3 = Y[8*i + 3];
      int Y4 = Y[8*i + 4];
      int Y5 = Y[8*i + 5];
      int Y6 = Y[8*i + 6];
      int Y7 = Y[8*i + 7];
      int U0 = U[4*i + 0];
      int U4 = U[4*i + 2];
      int V0 = V[4*i + 0];
      int V4 = V[4*i + 2];
      pDst[12*i + 0] = (uint8_t)U0;
      pDst[12*i + 1] = (uint8_t)Y0;
      pDst[12*i + 2] = (uint8_t)V0;
      pDst[12*i + 3] = (uint8_t)Y1;
      pDst[12*i + 4] = (uint8_t)U4;
      pDst[12*i + 5] = (uint8_t)Y2;
      pDst[12*i + 6] = (uint8_t)V4;
      pDst[12*i + 7] = (uint8_t)Y3;
      pDst[12*i + 8] = (uint8_t)Y4;
      pDst[12*i + 9] = (uint8_t)Y5;
      pDst[12*i + 10] = (uint8_t)Y6;
      pDst[12*i + 11] = (uint8_t)Y7;
    }
    pDst += iDstStride;
    Y += Y_stride;
    if (j & 1) {
      U += U_stride;
      V += V_stride;
    }
  }
  return ippStsNoErr;
}


IppStatus cc_YUV411_to_YUV420(const uint8_t *pSrc[3],
                          int32_t   pSrcStep[3],
                          uint8_t    *pDst[3],
                          int32_t   pDstStep[3],
                          mfxSize srcSize)
{
    int32_t h,w;
    int32_t srcStepU , srcStepV ;
    int32_t dstStepU , dstStepV ;
    int  width ;
    int  height ;
    const uint8_t* srcu;
    const uint8_t* srcv;
    IppStatus sts = ippStsNoErr;

    uint8_t* dstu;
    uint8_t* dstv;

    srcu = pSrc[1];
    srcv = pSrc[2];
    dstu = pDst[1];
    dstv = pDst[2];
    srcStepU = pSrcStep[1];
    srcStepV = pSrcStep[2];
    dstStepU = pDstStep[1];
    dstStepV = pDstStep[2];
    width  = srcSize.width ;
    height = srcSize.height;

    /* Y plane */
    sts = mfxiCopy_8u_C1R( pSrc[0], pSrcStep[0], pDst[0], pDstStep[0],  srcSize );
    if (ippStsNoErr != sts)
        return sts;

    for( h = 0; h < height ; h +=2)
    {
        for( w = 0; w < (width/4 -1) ;w ++ )
        {
            dstu[w*2] = srcu[w];
            dstu[w*2+1] = (srcu[w] + srcu[w+1]) / 2;

            dstv[w*2] = srcv[w];
            dstv[w*2+1] = (srcv[w] + srcv[w+1]) / 2;
        }
        dstu[w*2] = dstu[w*2 + 1] = srcu[w];
        dstv[w*2] = dstv[w*2 + 1] = srcv[w];

        srcu += 2*srcStepU;
        dstu += dstStepU;
        srcv += 2*srcStepV;
        dstv += dstStepV;
    }

    return sts;
}

#define kry0  0x000041cb
#define kry1  0x00008106
#define kry2  0x00001917
#define kry3  0x000025e3
#define kry4  0x00004a7f
#define kry5  0x00007062
#define kry6  0x00005e35
#define kry7  0x0000122d

static IppStatus ownBGRToYCbCr420_8u_AC4P2R(const uint8_t* pSrc, int srcStep, uint8_t* pDst[2],int dstStep[2], mfxSize roiSize )
{
  IppStatus sts = ippStsNoErr;
  int h,w;
  int dstStepY = dstStep[0];
  int width2  = roiSize.width  & ~1;
  int height2 = roiSize.height >> 1;
  uint8_t* pDstUV = pDst[1];

  for( h = 0; h < height2; h++ ){
    const uint8_t* src;
    uint8_t* dsty,*dstu,*dstv;
    src  = pSrc   + h * 2 * srcStep;
    dsty = pDst[0]+ h * 2 * dstStepY;

    dstu = pDstUV  + h * dstStep[1];
    dstv = dstu + 1;

    for( w = 0; w < width2; w += 2 ){
      int g,g1,g2,g3;
      int r,r1,r2,r3;
      int b,b1,b2,b3;
      b = src[0];g = src[1];r = src[2];
      b1= src[4];g1= src[5];r1= src[6];
      b2= src[0+srcStep];g2= src[1+srcStep];r2= src[2+srcStep];
      b3= src[4+srcStep];g3= src[5+srcStep];r3= src[6+srcStep];
      src += 8;
      dsty[0] = ( uint8_t )(( kry0 * r  + kry1 *  g + kry2 *  b + 0x108000) >> 16 );
      dsty[1] = ( uint8_t )(( kry0 * r1 + kry1 * g1 + kry2 * b1 + 0x108000) >> 16 );
      dsty[0+dstStepY] = ( uint8_t )(( kry0 * r2 + kry1 * g2 + kry2 * b2 + 0x108000) >> 16 );
      dsty[1+dstStepY] = ( uint8_t )(( kry0 * r3 + kry1 * g3 + kry2 * b3 + 0x108000) >> 16 );
      dsty += 2;
      r += r1;r += r2;r += r3;
      b += b1;b += b2;b += b3;
      g += g1;g += g2;g += g3;

      *dstu = ( uint8_t )((-kry3 * r - kry4 * g + kry5 * b + 0x2008000)>> 18 ); /* Cb */
      *dstv = ( uint8_t )(( kry5 * r - kry6 * g - kry7 * b + 0x2008000)>> 18 ); /* Cr */

      dstu += 2;
      dstv += 2;
    }
  }

  return sts;

} // int  ownBGRToYCbCr420_8u_AC4P3R( ... )

static IppStatus cc_RGB4_to_NV12(const uint8_t *pSrc,
                       int32_t   iSrcStride,
                       uint8_t* pDst[2],
                       int32_t dstStep[2],
                       mfxSize srcSize,
                       uint8_t isInterlace)
{
  IppStatus sts = ippStsNoErr;
  // alpha channel is ignore due to d3d issue

  if (!isInterlace)
  {
    sts = ownBGRToYCbCr420_8u_AC4P2R(pSrc, iSrcStride, pDst, dstStep, srcSize);
  }
  else
  {
    int32_t pDstFieldStep[2] = {dstStep[0]<<1, dstStep[1]<<1};
    mfxSize  roiFieldSize = {srcSize.width, srcSize.height >> 1};
    uint8_t* pDstSecondField[2] = {pDst[0]+dstStep[0], pDst[1]+dstStep[1]};

    /* first field */
    sts = ownBGRToYCbCr420_8u_AC4P2R(pSrc, iSrcStride<<1,
                                      pDst, pDstFieldStep, roiFieldSize);

    if( ippStsNoErr != sts ) return sts;
    /* second field */
    sts = ownBGRToYCbCr420_8u_AC4P2R(pSrc + iSrcStride, iSrcStride<<1,
                                      pDstSecondField, pDstFieldStep, roiFieldSize);

  }

  return sts;

} // int cc_RGB4_to_NV12( mfxFrameData* inData,  mfxFrameInfo* inInfo, ... )

static
IppStatus  ownBGRToYCbCr420_8u_C3P2R( const uint8_t* pSrc, int32_t srcStep, uint8_t* pDst[2],int32_t dstStep[2], mfxSize roiSize )
{
  int32_t h,w;
  int32_t dstStepY = dstStep[0];
  int32_t width2  = roiSize.width  & ~1;
  int32_t height2 = roiSize.height >> 1;
  uint8_t* pDstU = pDst[1];
  uint8_t* pDstV = pDstU + 1;

  IppStatus sts = ippStsNoErr;

  for( h = 0; h < height2; h++ ){
    const uint8_t* src;

    uint8_t* dsty, *dstu, *dstv;

    src  = pSrc   + h * 2 * srcStep;
    dsty = pDst[0]+ h * 2 * dstStepY;

    dstu = pDstU  + h * dstStep[1];
    dstv = pDstV  + h * dstStep[1];

    for( w = 0; w < width2; w += 2 ) {
      int32_t g,g1,g2,g3;
      int32_t r,r1,r2,r3;
      int32_t b,b1,b2,b3;
      b = src[0];g = src[1];r = src[2];
      b1= src[3];g1= src[4];r1= src[5];
      b2= src[0+srcStep];g2= src[1+srcStep];r2= src[2+srcStep];
      b3= src[3+srcStep];g3= src[4+srcStep];r3= src[5+srcStep];
      src += 6;
      dsty[0] = (uint8_t)(( kry0 * r  + kry1 *  g + kry2 *  b + 0x108000) >> 16 );
      dsty[1] = (uint8_t)(( kry0 * r1 + kry1 * g1 + kry2 * b1 + 0x108000) >> 16 );
      dsty[0+dstStepY] = (uint8_t)(( kry0 * r2 + kry1 * g2 + kry2 * b2 + 0x108000) >> 16 );
      dsty[1+dstStepY] = (uint8_t)(( kry0 * r3 + kry1 * g3 + kry2 * b3 + 0x108000) >> 16 );
      dsty += 2;

      r += r1;r += r2;r += r3;
      b += b1;b += b2;b += b3;
      g += g1;g += g2;g += g3;

      *dstu = ( uint8_t )((-kry3 * r - kry4 * g + kry5 * b + 0x2008000)>> 18 ); /* Cb */
      *dstv = ( uint8_t )(( kry5 * r - kry6 * g - kry7 * b + 0x2008000)>> 18 ); /* Cr */

      dstu += 2;
      dstv += 2;
    }
  }

  return sts;

} // int  ownBGRToYCbCr420_8u_C3P2R( const mfxU8* pSrc, mfxI32 srcStep, ...)

static IppStatus cc_RGB3_to_NV12(const uint8_t *pSrc,
                       int32_t   iSrcStride,
                       uint8_t* pDst[2],
                       int32_t dstStep[2],
                       mfxSize srcSize,
                       uint8_t isInterlace)
{
  IppStatus sts = ippStsNoErr;

  if (!isInterlace)
  {
    sts = ownBGRToYCbCr420_8u_C3P2R(pSrc, iSrcStride, pDst, dstStep, srcSize);
  }
  else
  {
    int32_t pDstFieldStep[2] = {dstStep[0]<<1, dstStep[1]<<1};
    mfxSize  roiFieldSize = {srcSize.width, srcSize.height >> 1};
    uint8_t* pDstSecondField[2] = {pDst[0]+dstStep[0], pDst[1]+dstStep[1]};

    /* first field */
    sts = ownBGRToYCbCr420_8u_C3P2R(pSrc, iSrcStride<<1, pDst, pDstFieldStep, roiFieldSize);
    if( ippStsNoErr != sts ) return sts;
    /* second field */
    sts = ownBGRToYCbCr420_8u_C3P2R( pSrc + iSrcStride, iSrcStride<<1,
                                     pDstSecondField, pDstFieldStep, roiFieldSize);
  }

  return sts;

} // int cc_RGB3_to_NV12( mfxFrameData* inData,  mfxFrameInfo* inInfo,...)
