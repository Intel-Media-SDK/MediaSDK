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

#pragma once

#include "ts_alloc.h"
#include "ts_bitstream.h"

template<class U>
class tsSampleAbstract
{
public:
    typedef U SampleType;
    virtual ~tsSampleAbstract() {}
    virtual operator U() = 0;
    virtual U operator=(U l) = 0;
    inline U operator=(tsSampleAbstract<U>& l) { return (*this = (U)l); }
};

template<class U>
class tsSample : public tsSampleAbstract<U>
{
public:
    tsSampleAbstract<U>* m_pImpl;
    bool m_own;

    tsSample(tsSampleAbstract<U>* pImpl, bool ownPtr = false)
        : m_pImpl(pImpl)
        , m_own(ownPtr)
    {}
    ~tsSample() { if (m_own) delete m_pImpl; }
    inline operator U() { return (U)*m_pImpl; }
    inline U operator=(U l) { return (*m_pImpl = l);  }
    inline U operator=(tsSample<U>& l) { return (*this = (U)l); }
};

template<class U, class MaxU>
class tsSampleImpl : public tsSampleAbstract<U>
{
private:
    MaxU* m_p;
    MaxU  m_mask;
    mfxU16 m_shift;
public:
    tsSampleImpl(void* p = 0, MaxU mask = (MaxU(-1) >> ((sizeof(MaxU) - sizeof(U)) * 8)), mfxU16 shift = 0)
        : m_p((MaxU*)p)
        , m_mask(mask)
        , m_shift(shift)
    {}
    inline tsSampleAbstract<U>& Set(void* p, MaxU mask = (MaxU(-1) >> ((sizeof(MaxU) - sizeof(U)) * 8)), mfxU16 shift = 0)
    {
        m_p = (MaxU*)p;
        m_mask = mask;
        m_shift = shift;
        return *this;
    }
    inline operator U() { return U((m_p[0] & m_mask) >> m_shift); }
    inline U operator=(U l)
    {
        m_p[0] &= ~m_mask;
        m_p[0] |= ((l << m_shift) & m_mask);
        return l;
    }
};

class tsFrameAbstract
{
public:
    tsSample<mfxU8> m_sample8;
    tsSample<mfxU16> m_sample16;

    tsFrameAbstract()
        : m_sample8(0)
        , m_sample16(0)
    {}

    virtual ~tsFrameAbstract() { }

    virtual tsSample<mfxU8>&  Y  (mfxU32 w, mfxU32 h) { g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR); return m_sample8; }
    virtual tsSample<mfxU8>&  U  (mfxU32 w, mfxU32 h) { g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR); return m_sample8; }
    virtual tsSample<mfxU8>&  V  (mfxU32 w, mfxU32 h) { g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR); return m_sample8; }
    virtual tsSample<mfxU8>&  R  (mfxU32 w, mfxU32 h) { g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR); return m_sample8; }
    virtual tsSample<mfxU8>&  G  (mfxU32 w, mfxU32 h) { g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR); return m_sample8; }
    virtual tsSample<mfxU8>&  B  (mfxU32 w, mfxU32 h) { g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR); return m_sample8; }
    virtual tsSample<mfxU8>&  A  (mfxU32 w, mfxU32 h) { g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR); return m_sample8; }
    virtual tsSample<mfxU16>& Y16(mfxU32 w, mfxU32 h) { g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR); return m_sample16; }
    virtual tsSample<mfxU16>& U16(mfxU32 w, mfxU32 h) { g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR); return m_sample16; }
    virtual tsSample<mfxU16>& V16(mfxU32 w, mfxU32 h) { g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR); return m_sample16; }
    virtual bool isYUV() { return false; }
    virtual bool isYUV16() { return false; }
    virtual bool isRGB() { return false; }

    virtual bool Copy(tsFrameAbstract const & src, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo) { return false; }
};

class tsFrameYV12 : public tsFrameAbstract
{
public:
    mfxU32 m_pitch;
    mfxU8* m_y;
    mfxU8* m_u;
    mfxU8* m_v;
    tsSampleImpl<mfxU8, mfxU8> m_sample_impl;
    tsSample<mfxU8> m_sample;

    tsFrameYV12(mfxFrameData d)
        : m_pitch( mfxU32(d.PitchHigh << 16) + d.PitchLow)
        , m_y(d.Y)
        , m_u(d.U)
        , m_v(d.V)
        , m_sample(&m_sample_impl)
    {}

    virtual ~tsFrameYV12() { }

    inline bool isYUV() {return true;};
    inline tsSample<mfxU8>& Y(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y[h * m_pitch + w]); return m_sample; }
    inline tsSample<mfxU8>& U(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_u[h/2 * m_pitch/2 + w/2]); return m_sample; }
    inline tsSample<mfxU8>& V(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_v[h/2 * m_pitch/2 + w/2]); return m_sample; }
};

class tsFrameYUY2 : public tsFrameAbstract
{
private:
    mfxU32 m_pitch;
    mfxU8* m_y;
    mfxU8* m_u;
    mfxU8* m_v;
    tsSampleImpl<mfxU8, mfxU8> m_sample_impl;
    tsSample<mfxU8> m_sample;
public:
    tsFrameYUY2(mfxFrameData d)
        : m_pitch( mfxU32(d.PitchHigh << 16) + d.PitchLow)
        , m_y(d.Y)
        , m_u(d.U)
        , m_v(d.V)
        , m_sample(&m_sample_impl)
    {}

    virtual ~tsFrameYUY2() { }

    inline bool isYUV() {return true;};
    inline tsSample<mfxU8>& Y(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y[h * m_pitch + w * 2]); return m_sample; }
    inline tsSample<mfxU8>& U(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_u[h * m_pitch + (w / 2) * 4]); return m_sample; }
    inline tsSample<mfxU8>& V(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_v[h * m_pitch + (w / 2) * 4]); return m_sample; }

    bool Copy(tsFrameAbstract const & src, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo);
};

class tsFrameAYUV : public tsFrameAbstract
{
private:
    mfxU32 m_pitch;
    mfxU8* m_y;
    mfxU8* m_u;
    mfxU8* m_v;
    tsSampleImpl<mfxU8, mfxU8> m_sample_impl;
    tsSample<mfxU8> m_sample;
public:
    tsFrameAYUV(mfxFrameData d)
        : m_pitch(mfxU32(d.PitchHigh << 16) + d.PitchLow)
        , m_y(d.Y)
        , m_u(d.U)
        , m_v(d.V)
        , m_sample(&m_sample_impl)
    {}

    virtual ~tsFrameAYUV() { }

    inline bool isYUV() { return true; };
    inline tsSample<mfxU8>& Y(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y[h * m_pitch + w * 4]); return m_sample; }
    inline tsSample<mfxU8>& U(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_u[h * m_pitch + w * 4]); return m_sample; }
    inline tsSample<mfxU8>& V(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_v[h * m_pitch + w * 4]); return m_sample; }

    bool Copy(tsFrameAbstract const & src, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo);
};

class tsFrameP010 : public tsFrameAbstract
{
public:
    mfxU32 m_pitch;
    mfxU16* m_y;
    mfxU16* m_uv;
    tsSampleImpl<mfxU16, mfxU16> m_sample_impl;
    tsSample<mfxU16> m_sample;

    tsFrameP010(mfxFrameData d)
        : m_pitch( mfxU32(d.PitchHigh << 16) + d.PitchLow)
        , m_y(d.Y16)
        , m_uv(d.U16)
        , m_sample(&m_sample_impl)
    {}

    virtual ~tsFrameP010() { }

    inline bool isYUV16() {return true;};
    inline tsSample<mfxU16>& Y16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y[h * (m_pitch/2) + w], 0xffc0, 6); return m_sample; }
    inline tsSample<mfxU16>& U16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_uv[(h/2) * (m_pitch/2) + (w%2==0?w:(w-1))], 0xffc0, 6); return m_sample; }
    inline tsSample<mfxU16>& V16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_uv[(h/2) * (m_pitch/2) + (w%2==0?w:(w-1)) + 1], 0xffc0, 6); return m_sample; }

    bool Copy(tsFrameAbstract const & src, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo);
};

class tsFrameP010s0 : public tsFrameP010
{
public:
    tsFrameP010s0(mfxFrameData d)
        : tsFrameP010(d)
    {}

    virtual ~tsFrameP010s0() { }

    inline bool isYUV16() { return true; };
    inline tsSample<mfxU16>& Y16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y[h * (m_pitch / 2) + w]); return m_sample; }
    inline tsSample<mfxU16>& U16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_uv[(h / 2) * (m_pitch / 2) + (w % 2 == 0 ? w : (w - 1))]); return m_sample; }
    inline tsSample<mfxU16>& V16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_uv[(h / 2) * (m_pitch / 2) + (w % 2 == 0 ? w : (w - 1)) + 1]); return m_sample; }
};

class tsFrameP016 : public tsFrameAbstract
{
public:
    mfxU32 m_pitch;
    mfxU16* m_y;
    mfxU16* m_uv;
    tsSampleImpl<mfxU16, mfxU16> m_sample_impl;
    tsSample<mfxU16> m_sample;

    tsFrameP016(mfxFrameData d)
        : m_pitch( mfxU32(d.PitchHigh << 16) + d.PitchLow)
        , m_y(d.Y16)
        , m_uv(d.U16)
        , m_sample(&m_sample_impl)
    {}

    virtual ~tsFrameP016() { }

    inline bool isYUV16() {return true;};
    inline tsSample<mfxU16>& Y16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y[h * (m_pitch/2) + w], 0xfff0, 4); return m_sample; }
    inline tsSample<mfxU16>& U16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_uv[(h/2) * (m_pitch/2) + (w%2==0?w:(w-1))], 0xfff0, 4); return m_sample; }
    inline tsSample<mfxU16>& V16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_uv[(h/2) * (m_pitch/2) + (w%2==0?w:(w-1)) + 1], 0xfff0, 4); return m_sample; }

    bool Copy(tsFrameAbstract const & src, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo);
};

class tsFrameY210 : public tsFrameAbstract
{
public:
    mfxU32 m_pitch;
    mfxU16* m_y;
    mfxU16* m_u;
    mfxU16* m_v;
    tsSampleImpl<mfxU16, mfxU16> m_sample_impl;
    tsSample<mfxU16> m_sample;

    tsFrameY210(mfxFrameData d)
        : m_pitch((mfxU32(d.PitchHigh << 16) + d.PitchLow) / 2)
        , m_y(d.Y16)
        , m_u(d.U16)
        , m_v(d.V16)
        , m_sample(&m_sample_impl)
    {}

    virtual ~tsFrameY210() { }

    inline bool isYUV16() { return true; };
    inline tsSample<mfxU16>& Y16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y[h * m_pitch + w * 2], 0xffc0, 6); return m_sample; }
    inline tsSample<mfxU16>& U16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_u[h * m_pitch + (w / 2) * 4], 0xffc0, 6); return m_sample; }
    inline tsSample<mfxU16>& V16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_v[h * m_pitch + (w / 2) * 4], 0xffc0, 6);return m_sample; }

    bool Copy(tsFrameAbstract const & src, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo);
};

class tsFrameY216 : public tsFrameAbstract
{
public:
    mfxU32 m_pitch;
    mfxU16* m_y;
    mfxU16* m_u;
    mfxU16* m_v;
    tsSampleImpl<mfxU16, mfxU16> m_sample_impl;
    tsSample<mfxU16> m_sample;

    tsFrameY216(mfxFrameData d)
        : m_pitch((mfxU32(d.PitchHigh << 16) + d.PitchLow) / 2)
        , m_y(d.Y16)
        , m_u(d.U16)
        , m_v(d.V16)
        , m_sample(&m_sample_impl)
    {}

    virtual ~tsFrameY216() { }

    inline bool isYUV16() { return true; };
    inline tsSample<mfxU16>& Y16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y[h * m_pitch + w * 2], 0xfff0, 4); return m_sample; }
    inline tsSample<mfxU16>& U16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_u[h * m_pitch + (w / 2) * 4], 0xfff0, 4); return m_sample; }
    inline tsSample<mfxU16>& V16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_v[h * m_pitch + (w / 2) * 4], 0xfff0, 4);return m_sample; }

    bool Copy(tsFrameAbstract const & src, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo);
};

class tsFrameY410 : public tsFrameAbstract
{
private:
    mfxU32 m_pitch;
    mfxY410* m_y;
    tsSampleImpl<mfxU16, mfxU32> m_sample_impl;
    tsSample<mfxU16> m_sample;
public:
    tsFrameY410(mfxFrameData d)
        : m_pitch((mfxU32(d.PitchHigh << 16) + d.PitchLow) / 4)
        , m_y(d.Y410)
        , m_sample(&m_sample_impl)
    {}

    virtual ~tsFrameY410() { }

    inline bool isYUV16() { return true; };
    inline tsSample<mfxU16>& U16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y[h * m_pitch + w], 0x000003ff,  0); return m_sample; }
    inline tsSample<mfxU16>& Y16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y[h * m_pitch + w], 0x000ffc00, 10); return m_sample; }
    inline tsSample<mfxU16>& V16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y[h * m_pitch + w], 0x3ff00000, 20); return m_sample; }

    bool Copy(tsFrameAbstract const & src, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo);
};

class tsFrameY416 : public tsFrameAbstract
{
private:
    mfxU32 m_pitch;
    mfxU16* m_y;
    mfxU16* m_u;
    mfxU16* m_v;
    tsSampleImpl<mfxU16, mfxU16> m_sample_impl;
    tsSample<mfxU16> m_sample;
public:
    tsFrameY416(mfxFrameData d)
        : m_pitch((mfxU32(d.PitchHigh << 16) + d.PitchLow) / 2)
        , m_y(d.Y16)
        , m_u(d.U16)
        , m_v(d.V16)
        , m_sample(&m_sample_impl)
    {}

    virtual ~tsFrameY416() { }

    inline bool isYUV16() { return true; };
    inline tsSample<mfxU16>& U16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y[h * m_pitch + w * 4], 0xffc0, 4); return m_sample; }
    inline tsSample<mfxU16>& Y16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_u[h * m_pitch + w * 4], 0xffc0, 4); return m_sample; }
    inline tsSample<mfxU16>& V16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_v[h * m_pitch + w * 4], 0xffc0, 4); return m_sample; }

    bool Copy(tsFrameAbstract const & src, mfxFrameInfo const & srcInfo, mfxFrameInfo const & dstInfo);
};

class tsFrameRGB4 : public tsFrameAbstract
{
private:
    mfxU32 m_pitch;
    mfxU8* m_r;
    mfxU8* m_g;
    mfxU8* m_b;
    mfxU8* m_a;
    tsSampleImpl<mfxU8, mfxU8> m_sample_impl;
    tsSample<mfxU8> m_sample;
public:
    tsFrameRGB4(mfxFrameData d)
        : m_pitch( mfxU32(d.PitchHigh << 16) + d.PitchLow)
        , m_r(d.R)
        , m_g(d.G)
        , m_b(d.B)
        , m_a(d.A)
        , m_sample(&m_sample_impl)
    {}

    virtual ~tsFrameRGB4() { }

    inline bool isRGB() {return true;};
    inline tsSample<mfxU8>& R(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_r[h * m_pitch + w * 4]); return m_sample; }
    inline tsSample<mfxU8>& G(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_g[h * m_pitch + w * 4]); return m_sample; }
    inline tsSample<mfxU8>& B(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_b[h * m_pitch + w * 4]); return m_sample; }
    inline tsSample<mfxU8>& A(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_a[h * m_pitch + w * 4]); return m_sample; }
};

class tsFrameR16 : public tsFrameAbstract
{
private:
    mfxU32 m_pitch;
    mfxU16* m_y16;
    tsSampleImpl<mfxU16, mfxU16> m_sample_impl;
    tsSample<mfxU16> m_sample;
public:
    tsFrameR16(mfxFrameData d)
        : m_pitch( mfxU32(d.PitchHigh << 16) + d.PitchLow)
        , m_y16(d.Y16)
        , m_sample(&m_sample_impl)
    {}

    virtual ~tsFrameR16() { }

    inline bool isYUV16() {return true;};
    inline tsSample<mfxU16>& Y16(mfxU32 w, mfxU32 h) { m_sample_impl.Set(&m_y16[h * m_pitch/2 + w]); return m_sample; }
};

class tsFrame
{
private:
    tsFrameAbstract* m_pFrame;
public:
    mfxFrameInfo m_info;

    tsFrame(mfxFrameSurface1);
    ~tsFrame();

    tsFrame& operator=(tsFrame&);

    inline tsSample<mfxU8>& Y(mfxU32 w, mfxU32 h) { return m_pFrame->Y(w, h); };
    inline tsSample<mfxU8>& U(mfxU32 w, mfxU32 h) { return m_pFrame->U(w, h); };
    inline tsSample<mfxU8>& V(mfxU32 w, mfxU32 h) { return m_pFrame->V(w, h); };
    inline tsSample<mfxU8>& R(mfxU32 w, mfxU32 h) { return m_pFrame->R(w, h); };
    inline tsSample<mfxU8>& G(mfxU32 w, mfxU32 h) { return m_pFrame->G(w, h); };
    inline tsSample<mfxU8>& B(mfxU32 w, mfxU32 h) { return m_pFrame->B(w, h); };
    inline tsSample<mfxU8>& A(mfxU32 w, mfxU32 h) { return m_pFrame->A(w, h); };
    inline tsSample<mfxU16>& Y16(mfxU32 w, mfxU32 h) { return m_pFrame->Y16(w, h); };
    inline tsSample<mfxU16>& U16(mfxU32 w, mfxU32 h) { return m_pFrame->U16(w, h); };
    inline tsSample<mfxU16>& V16(mfxU32 w, mfxU32 h) { return m_pFrame->V16(w, h); };
    inline bool isYUV() {return m_pFrame->isYUV(); };
    inline bool isYUV16() {return m_pFrame->isYUV16(); };
    inline bool isRGB() {return m_pFrame->isRGB(); };
};


class tsSurfaceProcessor
{
public:
    mfxU32 m_max;
    mfxU32 m_cur;
    bool   m_eos;

    tsSurfaceProcessor(mfxU32 n_frames = 0xFFFFFFFF);
    virtual ~tsSurfaceProcessor() {}

    virtual mfxFrameSurface1* ProcessSurface(mfxFrameSurface1* ps, mfxFrameAllocator* pfa);
    virtual mfxStatus ProcessSurface(mfxFrameSurface1&) { g_tsStatus.check(MFX_ERR_UNDEFINED_BEHAVIOR); return MFX_ERR_UNDEFINED_BEHAVIOR; };
};


class tsNoiseFiller : public tsSurfaceProcessor
{
public:
    tsNoiseFiller(mfxU32 n_frames = 0xFFFFFFFF);
    ~tsNoiseFiller();

    mfxStatus ProcessSurface(mfxFrameSurface1& s);
};


class tsRawReader : public tsSurfaceProcessor, tsReader
{
private:
    mfxFrameSurface1 m_surf;
    mfxU32           m_fsz;
    mfxU8*           m_buf;

    void Init(mfxFrameInfo fi);
public:
    bool m_disable_shift_hack;

    tsRawReader(const char* fname, mfxFrameInfo fi, mfxU32 n_frames = 0xFFFFFFFF);
    tsRawReader(mfxBitstream bs, mfxFrameInfo fi, mfxU32 n_frames = 0xFFFFFFFF);
    ~tsRawReader();

    mfxStatus ResetFile(bool reset_frame_order = false);

    mfxStatus ProcessSurface(mfxFrameSurface1& s);
};

class tsSurfaceWriter : public tsSurfaceProcessor
{
private:
    FILE* m_file;
public:
    tsSurfaceWriter(const char* fname, bool append = false);
    ~tsSurfaceWriter();
    mfxStatus ProcessSurface(mfxFrameSurface1& s);
};

inline mfxStatus WriteSurface(const char* fname, mfxFrameSurface1& s, bool append = false)
{
    tsSurfaceWriter wr(fname, append);
    return wr.ProcessSurface(s);
}

class tsAutoFlushFiller : public tsSurfaceProcessor
{
public:
    tsAutoFlushFiller(mfxU32 n_frames = 0xFFFFFFFF) : tsSurfaceProcessor(n_frames) {};
    ~tsAutoFlushFiller() {};

    mfxFrameSurface1* ProcessSurface(mfxFrameSurface1* ps, mfxFrameAllocator* pfa)
    {
        if (m_eos)
            return 0;

        if (m_cur++ >= m_max)
        {
            m_eos = true;
            return 0;
        }

        return ps;
    }
};

mfxF64 PSNR(tsFrame& ref, tsFrame& src, mfxU32 id);
mfxF64 SSIM(tsFrame& ref, tsFrame& src, mfxU32 id);

bool operator == (const mfxFrameSurface1& s1, const mfxFrameSurface1& s2);
bool operator != (const mfxFrameSurface1& s1, const mfxFrameSurface1& s2);
