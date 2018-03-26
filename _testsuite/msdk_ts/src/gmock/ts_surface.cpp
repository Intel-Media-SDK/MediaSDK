// Copyright (c) 2018 Intel Corporation
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

#include "ts_surface.h"
#include "math.h"

tsFrame::tsFrame(mfxFrameSurface1 s)
    : m_pFrame(0)
    , m_info(s.Info)
{
    switch(s.Info.FourCC)
    {
//    case MFX_FOURCC_NV12: m_pFrame = new tsFrameNV12(s.Data); break;
    case MFX_FOURCC_YV12: m_pFrame = new tsFrameYV12(s.Data); break;
    case MFX_FOURCC_YUY2: m_pFrame = new tsFrameYUY2(s.Data); break;
    case MFX_FOURCC_AYUV: m_pFrame = new tsFrameAYUV(s.Data); break;
    case MFX_FOURCC_P010:
        if (s.Info.Shift == 0)
            m_pFrame = new tsFrameP010s0(s.Data);
        else
            m_pFrame = new tsFrameP010(s.Data);
        break;
    case MFX_FOURCC_P016: m_pFrame = new tsFrameP016(s.Data); break;    // currently 12b only
    case MFX_FOURCC_Y210: m_pFrame = new tsFrameY210(s.Data); break;
    case MFX_FOURCC_Y216: m_pFrame = new tsFrameY216(s.Data); break;    // currently 12b only
    case MFX_FOURCC_Y410: m_pFrame = new tsFrameY410(s.Data); break;
    case MFX_FOURCC_Y416: m_pFrame = new tsFrameY416(s.Data); break;    // currently 12b only
    case MFX_FOURCC_BGR4: std::swap(s.Data.B, s.Data.R);
    case MFX_FOURCC_RGB4: m_pFrame = new tsFrameRGB4(s.Data); break;
    case MFX_FOURCC_R16:  m_pFrame = new tsFrameR16(s.Data); break;
        break;
    default: g_tsStatus.check(MFX_ERR_UNSUPPORTED);
    }
}

tsFrame::~tsFrame()
{
    if(m_pFrame)
    {
        delete m_pFrame;
    }
}

inline mfxU32 NumComponents(mfxFrameInfo fi)
{
    if(    fi.FourCC == MFX_FOURCC_RGB4
        || fi.FourCC == MFX_FOURCC_BGR4)
    {
        return 4;
    }

    if(fi.ChromaFormat == MFX_CHROMAFORMAT_YUV400)
    {
        return 1;
    }

    return 3;
}

/*
bool tsFrameNV12::Copy(tsFrameAbstract const & srcAbstract, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo)
{
    if (srcInfo.FourCC == MFX_FOURCC_NV12)
    {
        auto& src = (const tsFrameNV12 &)srcAbstract;
        mfxU32 maxw = TS_MIN(dstInfo.Width,  srcInfo.Width);
        mfxU32 maxh = TS_MIN(dstInfo.Height, srcInfo.Height);

        if (   dstInfo.CropW
            && dstInfo.CropH
            && srcInfo.CropW
            && srcInfo.CropH)
        {
            maxw = TS_MIN(dstInfo.CropW, srcInfo.CropW);
            maxh = TS_MIN(dstInfo.CropH, srcInfo.CropH);
        }

        mfxU8* pDst = m_y + (m_pitch * dstInfo.CropY) + dstInfo.CropX;
        mfxU8* pSrc = src.m_y + (src.m_pitch * srcInfo.CropY) + srcInfo.CropX;

        for (mfxU32 h = 0; h < maxh; h++)
        {
            if (!memcpy(pDst, pSrc, maxw))
                return false;
            pDst += m_pitch;
            pSrc += src.m_pitch;
        }

        if (dstInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV420 && srcInfo.ChromaFormat == MFX_CHROMAFORMAT_YUV420)
        {
            pDst = m_uv + (m_pitch * dstInfo.CropY / 2) + dstInfo.CropX;
            pSrc = src.m_uv + (src.m_pitch * srcInfo.CropY / 2) + srcInfo.CropX;
            maxh /= 2;

            for (mfxU32 h = 0; h < maxh; h++)
            {
                if (!memcpy(pDst, pSrc, maxw))
                    return false;
                pDst += m_pitch;
                pSrc += src.m_pitch;
            }
        }

        return true;
    }

    if (srcInfo.FourCC == MFX_FOURCC_YV12)
    {
        auto& src = (const tsFrameYV12 &)srcAbstract;
        mfxI32 maxw = TS_MIN(dstInfo.Width, srcInfo.Width);
        mfxI32 maxh = TS_MIN(dstInfo.Height, srcInfo.Height);

        if (   dstInfo.CropW
            && dstInfo.CropH
            && srcInfo.CropW
            && srcInfo.CropH)
        {
            maxw = TS_MIN(dstInfo.CropW, srcInfo.CropW);
            maxh = TS_MIN(dstInfo.CropH, srcInfo.CropH);
        }

        const Ipp8u* pSrc[3] = {
            src.m_y + (src.m_pitch * srcInfo.CropY) + srcInfo.CropX,
            src.m_u + (src.m_pitch / 2 * srcInfo.CropY / 2) + srcInfo.CropX / 2,
            src.m_v + (src.m_pitch / 2 * srcInfo.CropY / 2) + srcInfo.CropX / 2
        };
        mfxI32 srcPitch[3] = {(mfxI32)src.m_pitch, (mfxI32)src.m_pitch / 2, (mfxI32)src.m_pitch / 2 };
        mfxU8* pDstY = m_y + (m_pitch * dstInfo.CropY) + dstInfo.CropX;
        mfxU8* pDstUV = m_uv + (m_pitch * dstInfo.CropY / 2) + dstInfo.CropX;
        IppiSize roi = { maxw, maxh };

        if (ippiYCbCr420_8u_P3P2R(pSrc, srcPitch, pDstY, m_pitch, pDstUV, m_pitch, roi))
            return false;

        return true;
    }

    return false;
}

*/

bool tsFrameYUY2::Copy(tsFrameAbstract const & srcAbstract, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo)
{
    if (srcInfo.FourCC == MFX_FOURCC_YUY2)
    {
        auto& src = (const tsFrameYUY2 &)srcAbstract;
        mfxU32 maxw = TS_MIN(dstInfo.Width, srcInfo.Width);
        mfxU32 maxh = TS_MIN(dstInfo.Height, srcInfo.Height);

        if (   dstInfo.CropW
            && dstInfo.CropH
            && srcInfo.CropW
            && srcInfo.CropH)
        {
            maxw = TS_MIN(dstInfo.CropW, srcInfo.CropW);
            maxh = TS_MIN(dstInfo.CropH, srcInfo.CropH);
        }

        mfxU8* pDst = m_y + (m_pitch * dstInfo.CropY) + dstInfo.CropX * 2;
        mfxU8* pSrc = src.m_y + (src.m_pitch * srcInfo.CropY) + srcInfo.CropX * 2;
        maxw *= 2;

        for (mfxU32 h = 0; h < maxh; h++)
        {
            if (!memcpy(pDst, pSrc, maxw))
                return false;
            pDst += m_pitch;
            pSrc += src.m_pitch;
        }

        return true;
    }

    return false;
}

bool tsFrameAYUV::Copy(tsFrameAbstract const & srcAbstract, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo)
{
    if (srcInfo.FourCC == MFX_FOURCC_AYUV)
    {
        auto& src = (const tsFrameAYUV &)srcAbstract;
        mfxU32 maxw = TS_MIN(dstInfo.Width, srcInfo.Width);
        mfxU32 maxh = TS_MIN(dstInfo.Height, srcInfo.Height);

        if (   dstInfo.CropW
            && dstInfo.CropH
            && srcInfo.CropW
            && srcInfo.CropH)
        {
            maxw = TS_MIN(dstInfo.CropW, srcInfo.CropW);
            maxh = TS_MIN(dstInfo.CropH, srcInfo.CropH);
        }

        mfxU8* pDst = m_v + (m_pitch * dstInfo.CropY) + dstInfo.CropX * 4;
        mfxU8* pSrc = src.m_v + (src.m_pitch * srcInfo.CropY) + srcInfo.CropX * 4;
        maxw *= 4;

        for (mfxU32 h = 0; h < maxh; h++)
        {
            if (!memcpy(pDst, pSrc, maxw))
                return false;
            pDst += m_pitch;
            pSrc += src.m_pitch;
        }

        return true;
    }

    return false;
}

bool tsFrameP010::Copy(tsFrameAbstract const & srcAbstract, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo)
{
    if (  srcInfo.FourCC == MFX_FOURCC_P010
        && srcInfo.BitDepthLuma == dstInfo.BitDepthLuma
        && srcInfo.BitDepthChroma == dstInfo.BitDepthChroma
        && srcInfo.Shift == dstInfo.Shift)
    {
        auto& src = (const tsFrameP010 &)srcAbstract;
        mfxU32 maxw = TS_MIN(dstInfo.Width, srcInfo.Width);
        mfxU32 maxh = TS_MIN(dstInfo.Height, srcInfo.Height);

        if (dstInfo.CropW
            && dstInfo.CropH
            && srcInfo.CropW
            && srcInfo.CropH)
        {
            maxw = TS_MIN(dstInfo.CropW, srcInfo.CropW);
            maxh = TS_MIN(dstInfo.CropH, srcInfo.CropH);
        }

        mfxU8* pDst = (mfxU8*)m_y + (m_pitch * dstInfo.CropY) + dstInfo.CropX * 2;
        mfxU8* pSrc = (mfxU8*)src.m_y + (src.m_pitch * srcInfo.CropY) + srcInfo.CropX * 2;
        maxw *= 2;

        for (mfxU32 h = 0; h < maxh; h++)
        {
            if (!memcpy(pDst, pSrc, maxw))
                return false;
            pDst += m_pitch;
            pSrc += src.m_pitch;
        }

        pDst = (mfxU8*)m_uv + (m_pitch * dstInfo.CropY / 2) + dstInfo.CropX * 2;
        pSrc = (mfxU8*)src.m_uv + (src.m_pitch * srcInfo.CropY / 2) + srcInfo.CropX * 2;
        maxh /= 2;

        for (mfxU32 h = 0; h < maxh; h++)
        {
            if (!memcpy(pDst, pSrc, maxw))
                return false;
            pDst += m_pitch;
            pSrc += src.m_pitch;
        }

        return true;
    }

    return false;
}

bool tsFrameP016::Copy(tsFrameAbstract const & srcAbstract, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo)
{
    if (  srcInfo.FourCC == MFX_FOURCC_P016
        && srcInfo.BitDepthLuma == dstInfo.BitDepthLuma
        && srcInfo.BitDepthChroma == dstInfo.BitDepthChroma
        && srcInfo.Shift == dstInfo.Shift)
    {
        auto& src = (const tsFrameP016 &)srcAbstract;
        mfxU32 maxw = TS_MIN(dstInfo.Width, srcInfo.Width);
        mfxU32 maxh = TS_MIN(dstInfo.Height, srcInfo.Height);

        if (dstInfo.CropW
            && dstInfo.CropH
            && srcInfo.CropW
            && srcInfo.CropH)
        {
            maxw = TS_MIN(dstInfo.CropW, srcInfo.CropW);
            maxh = TS_MIN(dstInfo.CropH, srcInfo.CropH);
        }

        mfxU8* pDst = (mfxU8*)m_y + (m_pitch * dstInfo.CropY) + dstInfo.CropX * 2;
        mfxU8* pSrc = (mfxU8*)src.m_y + (src.m_pitch * srcInfo.CropY) + srcInfo.CropX * 2;
        maxw *= 2;

        for (mfxU32 h = 0; h < maxh; h++)
        {
            if (!memcpy(pDst, pSrc, maxw))
                return false;
            pDst += m_pitch;
            pSrc += src.m_pitch;
        }

        pDst = (mfxU8*)m_uv + (m_pitch * dstInfo.CropY / 2) + dstInfo.CropX * 2;
        pSrc = (mfxU8*)src.m_uv + (src.m_pitch * srcInfo.CropY / 2) + srcInfo.CropX * 2;
        maxh /= 2;

        for (mfxU32 h = 0; h < maxh; h++)
        {
            if (!memcpy(pDst, pSrc, maxw))
                return false;
            pDst += m_pitch;
            pSrc += src.m_pitch;
        }

        return true;
    }

    return false;
}

bool tsFrameY210::Copy(tsFrameAbstract const & srcAbstract, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo)
{
    if (   srcInfo.FourCC == MFX_FOURCC_Y210
        && srcInfo.BitDepthLuma == dstInfo.BitDepthLuma
        && srcInfo.BitDepthChroma == dstInfo.BitDepthChroma
        && srcInfo.Shift == dstInfo.Shift)
    {
        auto& src = (const tsFrameY210 &)srcAbstract;
        mfxU32 maxw = TS_MIN(dstInfo.Width, srcInfo.Width);
        mfxU32 maxh = TS_MIN(dstInfo.Height, srcInfo.Height);

        if (dstInfo.CropW
            && dstInfo.CropH
            && srcInfo.CropW
            && srcInfo.CropH)
        {
            maxw = TS_MIN(dstInfo.CropW, srcInfo.CropW);
            maxh = TS_MIN(dstInfo.CropH, srcInfo.CropH);
        }

        mfxU16* pDst = m_y + (m_pitch * dstInfo.CropY) + dstInfo.CropX * 2;
        mfxU16* pSrc = src.m_y + (src.m_pitch * srcInfo.CropY) + srcInfo.CropX * 2;
        maxw *= 4;

        for (mfxU32 h = 0; h < maxh; h++)
        {
            if (!memcpy(pDst, pSrc, maxw))
                return false;
            pDst += m_pitch;
            pSrc += src.m_pitch;
        }

        return true;
    }

    return false;
}

bool tsFrameY216::Copy(tsFrameAbstract const & srcAbstract, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo)
{
    if (   srcInfo.FourCC == MFX_FOURCC_Y216
        && srcInfo.BitDepthLuma == dstInfo.BitDepthLuma
        && srcInfo.BitDepthChroma == dstInfo.BitDepthChroma
        && srcInfo.Shift == dstInfo.Shift)
    {
        auto& src = (const tsFrameY216 &)srcAbstract;
        mfxU32 maxw = TS_MIN(dstInfo.Width, srcInfo.Width);
        mfxU32 maxh = TS_MIN(dstInfo.Height, srcInfo.Height);

        if (dstInfo.CropW
            && dstInfo.CropH
            && srcInfo.CropW
            && srcInfo.CropH)
        {
            maxw = TS_MIN(dstInfo.CropW, srcInfo.CropW);
            maxh = TS_MIN(dstInfo.CropH, srcInfo.CropH);
        }

        mfxU16* pDst = m_y + (m_pitch * dstInfo.CropY) + dstInfo.CropX * 2;
        mfxU16* pSrc = src.m_y + (src.m_pitch * srcInfo.CropY) + srcInfo.CropX * 2;
        maxw *= 4;

        for (mfxU32 h = 0; h < maxh; h++)
        {
            if (!memcpy(pDst, pSrc, maxw))
                return false;
            pDst += m_pitch;
            pSrc += src.m_pitch;
        }

        return true;
    }

    return false;
}

bool tsFrameY410::Copy(tsFrameAbstract const & srcAbstract, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo)
{
    if (   srcInfo.FourCC == MFX_FOURCC_Y410
        && srcInfo.BitDepthLuma == dstInfo.BitDepthLuma
        && srcInfo.BitDepthChroma == dstInfo.BitDepthChroma
        && srcInfo.Shift == dstInfo.Shift)
    {
        auto& src = (const tsFrameY410 &)srcAbstract;
        mfxU32 maxw = TS_MIN(dstInfo.Width, srcInfo.Width);
        mfxU32 maxh = TS_MIN(dstInfo.Height, srcInfo.Height);

        if (dstInfo.CropW
            && dstInfo.CropH
            && srcInfo.CropW
            && srcInfo.CropH)
        {
            maxw = TS_MIN(dstInfo.CropW, srcInfo.CropW);
            maxh = TS_MIN(dstInfo.CropH, srcInfo.CropH);
        }

        auto* pDst = m_y + (m_pitch * dstInfo.CropY) + dstInfo.CropX;
        auto* pSrc = src.m_y + (src.m_pitch * srcInfo.CropY) + srcInfo.CropX;
        maxw *= sizeof(*pDst);

        for (mfxU32 h = 0; h < maxh; h++)
        {
            if (!memcpy(pDst, pSrc, maxw))
                return false;
            pDst += m_pitch;
            pSrc += src.m_pitch;
        }

        return true;
    }

    return false;
}

bool tsFrameY416::Copy(tsFrameAbstract const & srcAbstract, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo)
{
    if (srcInfo.FourCC == MFX_FOURCC_Y416
        && srcInfo.BitDepthLuma == dstInfo.BitDepthLuma
        && srcInfo.BitDepthChroma == dstInfo.BitDepthChroma
        && srcInfo.Shift == dstInfo.Shift)
    {
        auto& src = (const tsFrameY416 &)srcAbstract;
        mfxU32 maxw = TS_MIN(dstInfo.Width, srcInfo.Width);
        mfxU32 maxh = TS_MIN(dstInfo.Height, srcInfo.Height);

        if (dstInfo.CropW
            && dstInfo.CropH
            && srcInfo.CropW
            && srcInfo.CropH)
        {
            maxw = TS_MIN(dstInfo.CropW, srcInfo.CropW);
            maxh = TS_MIN(dstInfo.CropH, srcInfo.CropH);
        }

        auto* pDst = m_y + (m_pitch * dstInfo.CropY) + dstInfo.CropX * 4;
        auto* pSrc = src.m_y + (src.m_pitch * srcInfo.CropY) + srcInfo.CropX * 4;
        maxw *= sizeof(*pDst) * 4;

        for (mfxU32 h = 0; h < maxh; h++)
        {
            if (!memcpy(pDst, pSrc, maxw))
                return false;
            pDst += m_pitch;
            pSrc += src.m_pitch;
        }

        return true;
    }

    return false;
}

tsFrame& tsFrame::operator=(tsFrame& src)
{
    if (m_pFrame->Copy(*src.m_pFrame, src.m_info, m_info))
    {
        return *this;
    }

    mfxU32 n = TS_MIN(NumComponents(m_info), NumComponents(src.m_info));
    mfxU32 maxw = TS_MIN(m_info.Width, src.m_info.Width);
    mfxU32 maxh = TS_MIN(m_info.Height, src.m_info.Height);

    if(    m_info.CropW
        && m_info.CropH
        && src.m_info.CropW
        && src.m_info.CropH)
    {
        maxw = TS_MIN(m_info.CropW, src.m_info.CropW);
        maxh = TS_MIN(m_info.CropH, src.m_info.CropH);
    }

    if(    src.m_info.Width > m_info.Width
        || src.m_info.Height > m_info.Height)
    {
        g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR);
    }

    if(isYUV() && src.isYUV())
    {
        for(mfxU32 h = 0; h < maxh; h ++)
        {
            for(mfxU32 w = 0; w < maxw; w ++)
            {
                Y(w + m_info.CropX, h + m_info.CropY) = src.Y(w + src.m_info.CropX, h + src.m_info.CropY);
                if(n == 3)
                {
                    U(w + m_info.CropX, h + m_info.CropY) = src.U(w + src.m_info.CropX, h + src.m_info.CropY);
                    V(w + m_info.CropX, h + m_info.CropY) = src.V(w + src.m_info.CropX, h + src.m_info.CropY);
                }
            }
        }
    }
    else if(isYUV16() && src.isYUV16())
    {
        for(mfxU32 h = 0; h < maxh; h ++)
        {
            for(mfxU32 w = 0; w < maxw; w ++)
            {
                Y16(w + m_info.CropX, h + m_info.CropY) = src.Y16(w + src.m_info.CropX, h + src.m_info.CropY);
                if(n == 3)
                {
                    U16(w + m_info.CropX, h + m_info.CropY) = src.U16(w + src.m_info.CropX, h + src.m_info.CropY);
                    V16(w + m_info.CropX, h + m_info.CropY) = src.V16(w + src.m_info.CropX, h + src.m_info.CropY);
                }
            }
        }
    }
    else if (isYUV16() && src.isYUV())
    {
        mfxU16 shY = TS_MAX(8, m_info.BitDepthLuma) - 8;
        mfxU16 shC = !m_info.BitDepthChroma ? shY : TS_MAX(8, m_info.BitDepthChroma) - 8;

        for (mfxU32 h = 0; h < maxh; h++)
        {
            for (mfxU32 w = 0; w < maxw; w++)
            {
                Y16(w + m_info.CropX, h + m_info.CropY) = mfxU16((mfxU8)src.Y(w + src.m_info.CropX, h + src.m_info.CropY)) << shY;
                if (n == 3)
                {
                    U16(w + m_info.CropX, h + m_info.CropY) = mfxU16((mfxU8)src.U(w + src.m_info.CropX, h + src.m_info.CropY)) << shC;
                    V16(w + m_info.CropX, h + m_info.CropY) = mfxU16((mfxU8)src.V(w + src.m_info.CropX, h + src.m_info.CropY)) << shC;
                }
            }
        }
    }
    else if (isYUV() && src.isYUV16())
    {
        mfxU16 shY = TS_MAX(8, src.m_info.BitDepthLuma) - 8;
        mfxU16 shC = !src.m_info.BitDepthChroma ? shY : TS_MAX(8, src.m_info.BitDepthChroma) - 8;

        for (mfxU32 h = 0; h < maxh; h++)
        {
            for (mfxU32 w = 0; w < maxw; w++)
            {
                Y(w + m_info.CropX, h + m_info.CropY) = mfxU8((mfxU16)src.Y16(w + src.m_info.CropX, h + src.m_info.CropY) >> shY);
                if (n == 3)
                {
                    U(w + m_info.CropX, h + m_info.CropY) = mfxU8((mfxU16)src.U16(w + src.m_info.CropX, h + src.m_info.CropY) >> shC);
                    V(w + m_info.CropX, h + m_info.CropY) = mfxU8((mfxU16)src.V16(w + src.m_info.CropX, h + src.m_info.CropY) >> shC);
                }
            }
        }
    }
    else if(isRGB() && src.isRGB())
    {
        for(mfxU32 h = 0; h < maxh; h ++)
        {
            for(mfxU32 w = 0; w < maxw; w ++)
            {
                R(w + m_info.CropX, h + m_info.CropY) = src.R(w + src.m_info.CropX, h + src.m_info.CropY);
                G(w + m_info.CropX, h + m_info.CropY) = src.G(w + src.m_info.CropX, h + src.m_info.CropY);
                B(w + m_info.CropX, h + m_info.CropY) = src.B(w + src.m_info.CropX, h + src.m_info.CropY);
                A(w + m_info.CropX, h + m_info.CropY) = src.A(w + src.m_info.CropX, h + src.m_info.CropY);
            }
        }
    }
    else
    {
        g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR);
    }

    return *this;
}

tsSurfaceProcessor::tsSurfaceProcessor(mfxU32 n_frames)
    : m_max(n_frames)
    , m_cur(0)
    , m_eos(false)
{
}

mfxFrameSurface1* tsSurfaceProcessor::ProcessSurface(mfxFrameSurface1* ps, mfxFrameAllocator* pfa)
{
    if(ps)
    {
        if(m_cur++ >= m_max)
        {
            return 0;
        }
        bool useAllocator = pfa && !ps->Data.Y;

        if(useAllocator)
        {
            TRACE_FUNC3(mfxFrameAllocator::Lock, pfa->pthis, ps->Data.MemId, &(ps->Data));
            g_tsStatus.check(pfa->Lock(pfa->pthis, ps->Data.MemId, &(ps->Data)));
        }

        TRACE_FUNC1(tsSurfaceProcessor::ProcessSurface, *ps);
        g_tsStatus.check(ProcessSurface(*ps));

        if(useAllocator)
        {
            TRACE_FUNC3(mfxFrameAllocator::Unlock, pfa->pthis, ps->Data.MemId, &(ps->Data));
            g_tsStatus.check(pfa->Unlock(pfa->pthis, ps->Data.MemId, &(ps->Data)));
        }
    }

    if(m_eos)
    {
        m_max = m_cur;
        return 0;
    }

    return ps;
}

tsNoiseFiller::tsNoiseFiller(mfxU32 n_frames)
    : tsSurfaceProcessor(n_frames)
{
}

tsNoiseFiller::~tsNoiseFiller()
{
}

mfxStatus tsNoiseFiller::ProcessSurface(mfxFrameSurface1& s)
{
    tsFrame d(s);

    if(d.isYUV())
    {
        for(mfxU32 h = 0; h < s.Info.Height; h++)
        {
            for(mfxU32 w = 0; w < s.Info.Width; w++)
            {
                d.Y(w,h) = rand() % (1 << 8);
                d.U(w,h) = rand() % (1 << 8);
                d.V(w,h) = rand() % (1 << 8);
            }
        }
    }
    else if (d.isYUV16())
    {
        mfxU16 rangeY = (1 << TS_MAX(8, s.Info.BitDepthLuma));
        mfxU16 rangeC = !s.Info.BitDepthChroma ? rangeY : (1 << TS_MAX(8, s.Info.BitDepthChroma));

        for (mfxU32 h = 0; h < s.Info.Height; h++)
        {
            for (mfxU32 w = 0; w < s.Info.Width; w++)
            {
                d.Y16(w, h) = rand() % rangeY;
                d.U16(w, h) = rand() % rangeC;
                d.V16(w, h) = rand() % rangeC;
            }
        }
    }
    else if(d.isRGB())
    {
        for(mfxU32 h = 0; h < s.Info.Height; h++)
        {
            for(mfxU32 w = 0; w < s.Info.Width; w++)
            {
                d.R(w,h) = rand() % (1 << 8);
                d.G(w,h) = rand() % (1 << 8);
                d.B(w,h) = rand() % (1 << 8);
                d.A(w,h) = rand() % (1 << 8);
            }
        }
    }
    else
    {
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

tsRawReader::tsRawReader(const char* fname, mfxFrameInfo fi, mfxU32 n_frames)
    : tsSurfaceProcessor(n_frames)
    , tsReader(fname)
    , m_surf()
    , m_fsz(0)
    , m_buf(0)
    , m_disable_shift_hack(false)
{
    Init(fi);
}

tsRawReader::tsRawReader(mfxBitstream bs, mfxFrameInfo fi, mfxU32 n_frames)
    : tsSurfaceProcessor(n_frames)
    , tsReader(bs)
    , m_surf()
    , m_fsz(0)
    , m_buf(0)
    , m_disable_shift_hack(false)
{
    Init(fi);
}

void tsRawReader::Init(mfxFrameInfo fi)
{
    //no crops for internal surface
    if(fi.CropW) fi.Width  = fi.CropW;
    if(fi.CropH) fi.Height = fi.CropH;
    fi.CropX = fi.CropY = 0;

    mfxU32 fsz = fi.Width * fi.Height;
    mfxU32 pitch = 0;
    mfxFrameData& m_data = m_surf.Data;

    m_surf.Info = fi;

    switch(fi.ChromaFormat)
    {
    case MFX_CHROMAFORMAT_YUV400 : m_fsz = fsz; break;
    case MFX_CHROMAFORMAT_YUV420 : m_fsz = fsz * 3 / 2; break;
    case MFX_CHROMAFORMAT_YUV422 :
    case MFX_CHROMAFORMAT_YUV422V: m_fsz = fsz * 2; break;
    case MFX_CHROMAFORMAT_YUV444 : m_fsz = fsz * 4; break;
    case MFX_CHROMAFORMAT_YUV411 :
    default: g_tsStatus.check(MFX_ERR_UNSUPPORTED);
    }

    if(MFX_FOURCC_R16 == m_surf.Info.FourCC)
        m_fsz = m_fsz * 2;

    if (   MFX_FOURCC_P010 == m_surf.Info.FourCC
        || MFX_FOURCC_P016 == m_surf.Info.FourCC
        || MFX_FOURCC_Y210 == m_surf.Info.FourCC
        || MFX_FOURCC_Y216 == m_surf.Info.FourCC
        || MFX_FOURCC_Y416 == m_surf.Info.FourCC)
    {
        fsz = fsz * 2;
        m_fsz = m_fsz * 2;
    }

    m_buf = new mfxU8[m_fsz];

    switch(fi.FourCC)
    {
    case MFX_FOURCC_NV12:
        m_data.Y     = m_buf;
        m_data.UV    = m_data.Y + fsz;
        pitch        = fi.Width;
        break;
    case MFX_FOURCC_YV12:
        m_data.Y     = m_buf;
        m_data.U     = m_data.Y + fsz;
        m_data.V     = m_data.U + fsz / 4;
        pitch        = fi.Width;
        break;
    case MFX_FOURCC_YUY2:
        m_data.Y     = m_buf;
        m_data.U     = m_buf + 1;
        m_data.V     = m_buf + 3;
        pitch        = fi.Width * 2;
        break;
    case MFX_FOURCC_AYUV:
        m_data.V = m_buf;
        m_data.U = m_buf + 1;
        m_data.Y = m_buf + 2;
        m_data.A = m_buf + 3;
        pitch = fi.Width * 4;
        break;
    case MFX_FOURCC_P010:
    case MFX_FOURCC_P016:
        m_data.Y16   = (mfxU16*) m_buf;
        m_data.U16   = (mfxU16*) (m_data.Y + fsz);
        pitch        = fi.Width * 2;
        break;
    case MFX_FOURCC_Y210:
    case MFX_FOURCC_Y216:
        m_data.Y16 = (mfxU16*)m_buf;
        m_data.U16 = m_data.Y16 + 1;
        m_data.V16 = m_data.Y16 + 3;
        pitch = fi.Width * 4;
        break;
    case MFX_FOURCC_Y410:
    case MFX_FOURCC_Y416:
        m_data.Y410 = (mfxY410*)m_buf;
        pitch = fi.Width * 4;
        break;
    case MFX_FOURCC_A2RGB10:
        m_data.A2RGB10 = (mfxA2RGB10*)m_buf;
        pitch = fi.Width * 4;
        break;
    case MFX_FOURCC_RGB4:
        m_data.R     = m_buf;
        m_data.G     = m_data.R + fsz;
        m_data.B     = m_data.G + fsz;
        m_data.A     = m_data.B + fsz;
        pitch        = fi.Width;
        break;
    case MFX_FOURCC_BGR4:
        m_data.B     = m_buf;
        m_data.G     = m_data.B + fsz;
        m_data.R     = m_data.G + fsz;
        m_data.A     = m_data.R + fsz;
        pitch        = fi.Width;
        break;
    case MFX_FOURCC_R16:
        m_data.Y16   = (mfxU16*) m_buf;
        pitch        = fi.Width * 2;
        break;
    default: g_tsStatus.check(MFX_ERR_UNSUPPORTED);
    }

    m_data.PitchLow  = (pitch & 0xFFFF);
    m_data.PitchHigh = (pitch >> 16);
}

tsRawReader::~tsRawReader()
{
    if(m_buf)
    {
        delete[] m_buf;
    }
}

mfxStatus tsRawReader::ProcessSurface(mfxFrameSurface1& s)
{
    m_eos = (m_fsz != Read(m_buf, m_fsz));

    /*WTF? Should be done inside "dst = src;" based on both src.shift and dst.shift if needed at all, not here.
      Wrong place, wrong condidtion. Keeping for bkwd compatibility, have to use m_disable_shift_hack for straight P010.*/
    if(m_surf.Info.Shift > 0 && !m_disable_shift_hack)
    {
        if(m_surf.Info.FourCC == MFX_FOURCC_P010)
        {
            mfxU16 *ptr = (mfxU16 *)m_buf;
            for(mfxU32 i = 0; i < m_fsz/2; i ++)
            {
                ptr[i] = (ptr[i] << 6) & ~0x3F;
            }
        }
    }

    if(!m_eos)
    {
        tsFrame src(m_surf);
        tsFrame dst(s);

        dst = src;
        s.Data.FrameOrder = m_cur++;
    }

    return MFX_ERR_NONE;
}

mfxStatus tsRawReader::ResetFile(bool reset_frame_order)
{
    if (reset_frame_order) m_cur = 0;
    return SeekToStart();
}

tsSurfaceWriter::tsSurfaceWriter(const char* fname, bool append)
    : m_file(0)
{
#pragma warning(disable:4996)
    m_file = fopen(fname, append ? "ab" : "wb");
#pragma warning(default:4996)
}

tsSurfaceWriter::~tsSurfaceWriter()
{
    if(m_file)
    {
        fclose(m_file);
    }
}

mfxStatus tsSurfaceWriter::ProcessSurface(mfxFrameSurface1& s)
{
    mfxU32 pitch = (s.Data.PitchHigh << 16) + s.Data.PitchLow;
    size_t count = s.Info.CropW;

    if(s.Info.FourCC == MFX_FOURCC_NV12)
    {
        for(mfxU16 i = s.Info.CropY; i < (s.Info.CropH + s.Info.CropY); i ++)
        {
            if (fwrite(s.Data.Y + pitch * i + s.Info.CropX, 1, count, m_file)
                != count)
                return MFX_ERR_UNKNOWN;
        }

        for(mfxU16 i = (s.Info.CropY / 2); i < ((s.Info.CropH + s.Info.CropY) / 2); i ++)
        {
            if (fwrite(s.Data.UV + pitch * i + s.Info.CropX, 1, count, m_file)
                != count)
                return MFX_ERR_UNKNOWN;
        }
    }
    else if(s.Info.FourCC == MFX_FOURCC_RGB4)
    {
        count *= 4;
        mfxU8* ptr = 0;
        ptr = TS_MIN( TS_MIN(s.Data.R, s.Data.G), s.Data.B );
        ptr = ptr + s.Info.CropX + s.Info.CropY * pitch;

        for(mfxU32 i = s.Info.CropY; i < s.Info.CropH; i++)
        {
            if (fwrite(ptr + i * pitch, 1, count, m_file) != count)
                return MFX_ERR_UNKNOWN;
        }
    }
    else if (s.Info.FourCC == MFX_FOURCC_YUY2)
    {
        mfxU16 cropX = ((s.Info.CropX >> 1) << 2);
        count *= 2;

        for (mfxU16 i = s.Info.CropY; i < (s.Info.CropH + s.Info.CropY); i++)
        {
            if (fwrite(s.Data.Y + pitch * i + cropX, 1, count, m_file) != count)
                return MFX_ERR_UNKNOWN;
        }
    }
    else if (s.Info.FourCC == MFX_FOURCC_AYUV)
    {
        mfxU16 cropX = s.Info.CropX * 4;
        count *= 4;

        for (mfxU16 i = s.Info.CropY; i < (s.Info.CropH + s.Info.CropY); i++)
        {
            if (fwrite(s.Data.V + pitch * i + cropX, 1, count, m_file) != count)
                return MFX_ERR_UNKNOWN;
        }
    }
    else if (s.Info.FourCC == MFX_FOURCC_P010)
    {
        mfxU16 cropX = s.Info.CropX * 2;

        for (mfxU16 i = s.Info.CropY; i < (s.Info.CropH + s.Info.CropY); i++)
        {
            if (fwrite(s.Data.Y + pitch * i + cropX, 2, count, m_file) != count)
                return MFX_ERR_UNKNOWN;
        }

        for (mfxU16 i = (s.Info.CropY / 2); i < ((s.Info.CropH + s.Info.CropY) / 2); i++)
        {
            if (fwrite(s.Data.UV + pitch * i + cropX, 2, count, m_file) != count)
                return MFX_ERR_UNKNOWN;
        }
    }
    else if (s.Info.FourCC == MFX_FOURCC_Y210)
    {
        mfxU16 cropX = ((s.Info.CropX >> 1) << 2);
        count *= 2;
        pitch /= 2; //bytes to words

        for (mfxU16 i = s.Info.CropY; i < (s.Info.CropH + s.Info.CropY); i++)
        {
            if (fwrite(s.Data.Y16 + pitch * i + cropX, 2, count, m_file) != count)
                return MFX_ERR_UNKNOWN;
        }
    }
    else if (s.Info.FourCC == MFX_FOURCC_Y410)
    {
        mfxU16 cropX = s.Info.CropX;
        pitch /= 4; //bytes to dwords

        for (mfxU16 i = s.Info.CropY; i < (s.Info.CropH + s.Info.CropY); i++)
        {
            if (fwrite(s.Data.Y410 + pitch * i + cropX, 4, count, m_file) != count)
                return MFX_ERR_UNKNOWN;
        }
    }
    else
    {
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}


/*
mfxStatus tsSurfaceCRC32::ProcessSurface(mfxFrameSurface1& s)
{
    mfxU32 pitch = (s.Data.PitchHigh << 16) + s.Data.PitchLow;
    size_t count = s.Info.CropW;

    switch (s.Info.FourCC) {
        case MFX_FOURCC_NV12:
        {
            for(mfxU16 i = s.Info.CropY; i < (s.Info.CropH + s.Info.CropY); i ++)
            {
                IppStatus sts = ippsCRC32_8u(s.Data.Y + pitch * i + s.Info.CropX, (mfxU32)count, &m_crc);
                if (sts != ippStsNoErr)
                {
                    g_tsLog << "ERROR: cannot calculate CRC32 IppStatus=" << sts << "\n";
                    return MFX_ERR_ABORTED;
                }
            }
            for(mfxU16 i = (s.Info.CropY / 2); i < ((s.Info.CropH + s.Info.CropY) / 2); i ++)
            {
                IppStatus sts = ippsCRC32_8u(s.Data.UV + pitch * i + s.Info.CropX, (mfxU32)count, &m_crc);
                if (sts != ippStsNoErr)
                {
                    g_tsLog << "ERROR: cannot calculate CRC32 IppStatus=" << sts << "\n";
                    return MFX_ERR_ABORTED;
                }
            }
            break;
        }
        case MFX_FOURCC_RGB4:
        {
            count *= 4;
            mfxU8* ptr = 0;
            ptr = TS_MIN( TS_MIN(s.Data.R, s.Data.G), s.Data.B );
            ptr = ptr + s.Info.CropX + s.Info.CropY * pitch;

            for(mfxU32 i = s.Info.CropY; i < s.Info.CropH; i++)
            {
                IppStatus sts = ippsCRC32_8u(ptr + i * pitch, (mfxU32)count, &m_crc);
                if (sts != ippStsNoErr)
                {
                    g_tsLog << "ERROR: cannot calculate CRC32 IppStatus=" << sts << "\n";
                    return MFX_ERR_ABORTED;
                }
            }
            break;
        }
        case MFX_FOURCC_YUY2:
        case MFX_FOURCC_AYUV:
        case MFX_FOURCC_P010:
        case MFX_FOURCC_Y210:
        case MFX_FOURCC_Y410:
        {
            g_tsLog << "ERROR: CRC calculation is not impelemented\n";
            return MFX_ERR_ABORTED;
        }
        default:
            return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}
*/

mfxF64 PSNR(tsFrame& ref, tsFrame& src, mfxU32 id)
{
    mfxF64 size = ref.m_info.CropW * ref.m_info.CropH;
    mfxI32 diff = 0;
    mfxU64 dist = 0;
    mfxU32 chroma_step = 1;
    mfxU32 maxw = TS_MIN(ref.m_info.CropW, src.m_info.CropW);
    mfxU32 maxh = TS_MIN(ref.m_info.CropH, src.m_info.CropH);
    mfxU16 bd0 = id ? TS_MAX(8, ref.m_info.BitDepthChroma) : TS_MAX(8, ref.m_info.BitDepthLuma);
    mfxU16 bd1 = id ? TS_MAX(8, src.m_info.BitDepthChroma) : TS_MAX(8, src.m_info.BitDepthLuma);
    mfxF64 max  = (1 << bd0) - 1;

    if (bd0 != bd1)
        g_tsStatus.check(MFX_ERR_UNSUPPORTED);

    if (0 != id)
    {
        if(    ref.m_info.ChromaFormat == MFX_CHROMAFORMAT_YUV400
            || src.m_info.ChromaFormat == MFX_CHROMAFORMAT_YUV400)
        {
            g_tsStatus.check(MFX_ERR_UNSUPPORTED);
            return 0;
        }
        if(    ref.m_info.ChromaFormat == MFX_CHROMAFORMAT_YUV420
            && src.m_info.ChromaFormat == MFX_CHROMAFORMAT_YUV420)
        {
            chroma_step = 2;
            size /= 4;
        }
    }

#define GET_DIST(COMPONENT, STEP)                                              \
    for(mfxU32 y = 0; y < maxh; y += STEP)                                     \
    {                                                                          \
        for(mfxU32 x = 0; x < maxw; x += STEP)                                 \
        {                                                                      \
            diff = ref.COMPONENT(x + ref.m_info.CropX, y + ref.m_info.CropY) - \
                   src.COMPONENT(x + src.m_info.CropX, y + src.m_info.CropY);  \
            dist += (diff * diff);                                             \
        }                                                                      \
    }
    if (ref.isYUV() && src.isYUV())
    {
        switch(id)
        {
        case 0:  GET_DIST(Y, 1); break;
        case 1:  GET_DIST(U, chroma_step); break;
        case 2:  GET_DIST(V, chroma_step); break;
        default: g_tsStatus.check(MFX_ERR_UNSUPPORTED); break;
        }
    }
    else if (ref.isYUV16() && src.isYUV16())
    {
        switch (id)
        {
        case 0:  GET_DIST(Y16, 1); break;
        case 1:  GET_DIST(U16, chroma_step); break;
        case 2:  GET_DIST(V16, chroma_step); break;
        default: g_tsStatus.check(MFX_ERR_UNSUPPORTED); break;
        }
    }
    else
        g_tsStatus.check(MFX_ERR_UNSUPPORTED);

    if (0 == dist)
        return 1000.;

    return (10. * log10(max * max * (size / dist)));
}

mfxF64 SSIM(tsFrame& ref, tsFrame& src, mfxU32 id)
{
    //mfxF64 max  = (1 << 8) - 1;
    mfxF64 C1 =  6.5025;//max * max / 10000;
    mfxF64 C2 = 58.5225;//max * max * 9 / 10000;
    mfxU32 height      = TS_MIN(ref.m_info.CropH, src.m_info.CropH);
    mfxU32 width       = TS_MIN(ref.m_info.CropW, src.m_info.CropW);
    mfxU32 win_width   = 4;
    mfxU32 win_height  = 4;
    mfxU32 win_size    = win_width * win_height;
    mfxU32 win_cnt     = 0;
    mfxU32 chroma_step = 1;
    mfxF64 dist        = 0;

#undef GET_DIST
#define GET_DIST(COMPONENT, STEP)                                                             \
    win_width  *= STEP;                                                                       \
    win_height *= STEP;                                                                       \
    for(mfxU32 j = 0; j <= height - win_height; j += STEP)                                    \
    {                                                                                         \
        for(mfxU32 i = 0; i <= width - win_width; i += STEP)                                  \
        {                                                                                     \
            mfxU32 imeanRef   = 0;                                                            \
            mfxU32 imeanSrc   = 0;                                                            \
            mfxU32 ivarRef    = 0;                                                            \
            mfxU32 ivarSrc    = 0;                                                            \
            mfxU32 icovRefSrc = 0;                                                            \
                                                                                              \
            for(mfxU32 y = j; y < j + win_height; y += STEP)                                  \
            {                                                                                 \
                for(mfxU32 x = i; x < i + win_width; x += STEP)                               \
                {                                                                             \
                    mfxU32 rP = ref.COMPONENT(x + ref.m_info.CropX, y + ref.m_info.CropY);    \
                    mfxU32 sP = src.COMPONENT(x + src.m_info.CropX, y + src.m_info.CropY);    \
                                                                                              \
                    imeanRef   += rP;                                                         \
                    imeanSrc   += sP;                                                         \
                    ivarRef    += rP * rP;                                                    \
                    ivarSrc    += sP * sP;                                                    \
                    icovRefSrc += rP * sP;                                                    \
                }                                                                             \
            }                                                                                 \
                                                                                              \
            mfxF64 meanRef   = (mfxF64) imeanRef / win_size;                                  \
            mfxF64 meanSrc   = (mfxF64) imeanSrc / win_size;                                  \
            mfxF64 varRef    = (mfxF64)(ivarRef    - imeanRef * meanRef) / win_size;          \
            mfxF64 varSrc    = (mfxF64)(ivarSrc    - imeanSrc * meanSrc) / win_size;          \
            mfxF64 covRefSrc = (mfxF64)(icovRefSrc - imeanRef * meanSrc) / win_size;          \
                                                                                              \
            dist += ((meanRef * meanSrc * 2 + C1) * (covRefSrc * 2 + C2)) /                   \
                ((meanRef * meanRef + meanSrc * meanSrc + C1) * (varRef + varSrc + C2));      \
            win_cnt++;                                                                        \
        }                                                                                     \
    }

    if(ref.isYUV() && src.isYUV())
    {
        if( 0 != id )
        {
            if(    ref.m_info.ChromaFormat == MFX_CHROMAFORMAT_YUV400
                || src.m_info.ChromaFormat == MFX_CHROMAFORMAT_YUV400)
            {
                g_tsStatus.check(MFX_ERR_UNSUPPORTED);
                return 0;
            }
            if(    ref.m_info.ChromaFormat == MFX_CHROMAFORMAT_YUV420
                && src.m_info.ChromaFormat == MFX_CHROMAFORMAT_YUV420)
            {
                chroma_step = 2;
            }
        }

        switch(id)
        {
        case 0:  GET_DIST(Y, 1); break;
        case 1:  GET_DIST(U, chroma_step); break;
        case 2:  GET_DIST(V, chroma_step); break;
        default: g_tsStatus.check(MFX_ERR_UNSUPPORTED); break;
        }

    } else g_tsStatus.check(MFX_ERR_UNSUPPORTED);


    return (dist / win_cnt);
}

bool operator == (const mfxFrameSurface1& s1, const mfxFrameSurface1& s2)
{
    if (!(s1.Info == s2.Info)) return false;
    if (!(s1.Data == s2.Data)) return false;

    return true;
}

bool operator != (const mfxFrameSurface1& s1, const mfxFrameSurface1& s2){return !(s1==s2);}
