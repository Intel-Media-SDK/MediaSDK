// Copyright (c) 2020 Intel Corporation
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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_disp.h"
#include "hevcehw_base.h"
#include "mfx_h265_encode_hw.h"
#include "mfx_h265_fei_encode_hw.h"

namespace HEVCEHW
{
    namespace LegacyFallback
    {
#if defined(MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE)
        class FEI
            : public MfxHwH265FeiEncode::H265FeiEncode_HW
            , public ImplBase
        {
        public:
            FEI(
                VideoCORE* core
                , mfxStatus& status
                , eFeatureMode)
                : MfxHwH265FeiEncode::H265FeiEncode_HW(core, &status)
            {
            }
            virtual mfxStatus InternalQuery(
                VideoCORE& core
                , mfxVideoParam *in
                , mfxVideoParam& out) override
            {
                return MfxHwH265FeiEncode::H265FeiEncode_HW::Query(&core, in, &out);
            }
            virtual mfxStatus InternalQueryIOSurf(
                VideoCORE& core
                , mfxVideoParam& par
                , mfxFrameAllocRequest& request) override
            {
                return MfxHwH265FeiEncode::H265FeiEncode_HW::QueryIOSurf(&core, &par, &request);
            }
        };
#endif //defined(MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE)

        class MFXVideoENCODEH265_HW
            : public MfxHwH265Encode::MFXVideoENCODEH265_HW
            , public ImplBase
        {
        public:
            MFXVideoENCODEH265_HW(
                VideoCORE& core
                , mfxStatus& status
                , eFeatureMode)
                : MfxHwH265Encode::MFXVideoENCODEH265_HW(&core, &status)
            {
            }
            virtual mfxStatus InternalQuery(
                VideoCORE& core
                , mfxVideoParam *in
                , mfxVideoParam& out) override
            {
                return MfxHwH265Encode::MFXVideoENCODEH265_HW::Query(&core, in, &out);
            }
            virtual mfxStatus InternalQueryIOSurf(
                VideoCORE& core
                , mfxVideoParam& par
                , mfxFrameAllocRequest& request) override
            {
                return MfxHwH265Encode::MFXVideoENCODEH265_HW::QueryIOSurf(&core, &par, &request);
            }

            virtual ImplBase* ApplyMode(mfxU32 mode) override
            {
#if defined(MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE)
                if (mode == IMPL_MODE_FEI && !dynamic_cast<FEI*>(this))
                {
                    mfxStatus sts = MFX_ERR_NONE;
                    auto pFEI = new FEI(m_core, sts, eFeatureMode(0));

                    delete this;

                    return pFEI;
                }
#else
                std::ignore = mode;
#endif //defined(MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE)
                return this;
            }
        };
    };
};

#if defined(MFX_VA_LINUX)
    #include "hevcehw_base_lin.h"
    namespace HEVCEHWDisp { namespace Base { using namespace HEVCEHW::Linux::Base; }; };
#else
    namespace HEVCEHWDisp { namespace Base { using namespace HEVCEHW::LegacyFallback; }; };
#endif

#if defined(MFX_VA_LINUX)
    // There is no gen9/gen11 separation in code - use Base code-pass for ICL as well
    #include "hevcehw_base_lin.h"
    namespace HEVCEHWDisp { namespace Gen11 { using namespace HEVCEHW::Linux::Base; }; };
#else
    namespace HEVCEHWDisp { namespace Gen11 { using namespace HEVCEHW::LegacyFallback; }; };
#endif

#if defined(MFX_VA_LINUX)
    #include "hevcehw_g12_lin.h"
    namespace HEVCEHWDisp { namespace TGL { using namespace HEVCEHW::Linux::Gen12; }; };
    namespace HEVCEHWDisp { namespace DG1 { using namespace HEVCEHW::Linux::Gen12; }; };
#else
    namespace HEVCEHWDisp { namespace TGL { using namespace HEVCEHW::LegacyFallback; }; };
#endif

namespace HEVCEHW
{

static ImplBase* CreateSpecific(
    eMFXHWType HW
    , VideoCORE& core
    , mfxStatus& status
    , eFeatureMode mode)
{
    if (HW == MFX_HW_DG1)
        return new HEVCEHWDisp::DG1::MFXVideoENCODEH265_HW(core, status, mode);
    if (HW >= MFX_HW_TGL_LP)
        return new HEVCEHWDisp::TGL::MFXVideoENCODEH265_HW(core, status, mode);
    if (HW >= MFX_HW_ICL)
        return new HEVCEHWDisp::Gen11::MFXVideoENCODEH265_HW(core, status, mode);
    return new HEVCEHWDisp::Base::MFXVideoENCODEH265_HW(core, status, mode);
}

VideoENCODE* Create(
    VideoCORE& core
    , mfxStatus& status
    , bool bFEI)
{
    auto hw = core.GetHWType();

    if (hw < MFX_HW_SCL)
    {
        status = MFX_ERR_UNSUPPORTED;
        return nullptr;
    }

    auto p = CreateSpecific(hw, core, status, eFeatureMode::INIT);

    if (p && bFEI)
    {
        p = p->ApplyMode(IMPL_MODE_FEI);
    }

    return p;
}

mfxStatus QueryIOSurf(
    VideoCORE *core
    , mfxVideoParam *par
    , mfxFrameAllocRequest *request
    , bool bFEI)
{
    MFX_CHECK_NULL_PTR3(core, par, request);

    auto hw = core->GetHWType();

    if (hw < MFX_HW_SCL)
        return MFX_ERR_UNSUPPORTED;

    mfxStatus sts = MFX_ERR_NONE;
    std::unique_ptr<ImplBase> impl(CreateSpecific(hw, *core, sts, eFeatureMode::QUERY_IO_SURF));

    MFX_CHECK_STS(sts);
    MFX_CHECK(impl, MFX_ERR_UNKNOWN);

    impl.reset(impl.release()->ApplyMode(bFEI * IMPL_MODE_FEI));
    MFX_CHECK(impl, MFX_ERR_UNKNOWN);

    return impl->InternalQueryIOSurf(*core, *par, *request);
}

mfxStatus Query(
    VideoCORE *core
    , mfxVideoParam *in
    , mfxVideoParam *out
    , bool bFEI)
{
    MFX_CHECK_NULL_PTR2(core, out);

    auto hw = core->GetHWType();

    if (hw < MFX_HW_SCL)
        return MFX_ERR_UNSUPPORTED;

    mfxStatus sts = MFX_ERR_NONE;
    std::unique_ptr<ImplBase> impl(CreateSpecific(hw, *core, sts, in ? eFeatureMode::QUERY1 : eFeatureMode::QUERY0));

    MFX_CHECK_STS(sts);
    MFX_CHECK(impl, MFX_ERR_UNKNOWN);

    impl.reset(impl.release()->ApplyMode(bFEI * IMPL_MODE_FEI));
    MFX_CHECK(impl, MFX_ERR_UNKNOWN);

    return impl->InternalQuery(*core, in, *out);
}

} //namespace HEVCEHW

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
