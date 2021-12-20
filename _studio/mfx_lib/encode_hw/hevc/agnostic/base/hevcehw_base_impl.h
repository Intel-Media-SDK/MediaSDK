// Copyright (c) 2019-2020 Intel Corporation
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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "mfx_task.h"
#include "hevcehw_base.h"

namespace HEVCEHW
{
namespace Base
{
    class MFXVideoENCODEH265_HW
        : public ImplBase
        , virtual protected FeatureBlocks
    {
    public:
        MFXVideoENCODEH265_HW(VideoCORE& core);

        virtual ~MFXVideoENCODEH265_HW()
        {
            Close();
        }

        void InternalInitFeatures(
            mfxStatus& status
            , eFeatureMode mode);

        virtual mfxStatus Init(mfxVideoParam *par) override;

        virtual mfxStatus Reset(mfxVideoParam *par) override;

        virtual mfxStatus Close(void) override;

        virtual mfxStatus GetVideoParam(mfxVideoParam *par) override;

        virtual mfxStatus GetFrameParam(mfxFrameParam * /*par*/) override
        {
            return MFX_ERR_UNSUPPORTED;
        }
        virtual mfxStatus GetEncodeStat(mfxEncodeStat* /*stat*/) override
        {
            return MFX_ERR_UNSUPPORTED;
        }

        virtual mfxStatus EncodeFrameCheck(
            mfxEncodeCtrl * /*ctrl*/
            , mfxFrameSurface1 * /*surface*/
            , mfxBitstream * /*bs*/
            , mfxFrameSurface1 ** /*reordered_surface*/
            , mfxEncodeInternalParams * /*pInternalParams*/
            , MFX_ENTRY_POINT * /*pEntryPoint*/) override;

        virtual mfxStatus EncodeFrame(
            mfxEncodeCtrl * /*ctrl*/
            , mfxEncodeInternalParams * /*pInternalParams*/
            , mfxFrameSurface1 * /*surface*/
            , mfxBitstream * /*bs*/) override
        {
            return MFX_ERR_UNSUPPORTED;
        }

        virtual mfxStatus EncodeFrameCheck(
            mfxEncodeCtrl * /*ctrl*/
            , mfxFrameSurface1 * /*surface*/
            , mfxBitstream * /*bs*/
            , mfxFrameSurface1 ** /*reordered_surface*/
            , mfxEncodeInternalParams * /*pInternalParams*/) override
        {
            return MFX_ERR_UNSUPPORTED;
        }

        virtual mfxStatus CancelFrame(
            mfxEncodeCtrl * /*ctrl*/
            , mfxEncodeInternalParams * /*pInternalParams*/
            , mfxFrameSurface1 * /*surface*/
            , mfxBitstream * /*bs*/) override
        {
            return MFX_ERR_UNSUPPORTED;
        }

        virtual mfxTaskThreadingPolicy GetThreadingPolicy(void) override
        {
            return MFX_TASK_THREADING_INTRA;
        }

        virtual mfxStatus InternalQuery(
            VideoCORE& core
            , mfxVideoParam *in
            , mfxVideoParam& out) override;

        virtual mfxStatus InternalQueryIOSurf(
            VideoCORE& core
            , mfxVideoParam& par
            , mfxFrameAllocRequest& request) override;

    protected:
        using TFeatureList = std::list<std::unique_ptr<FeatureBase>>;
        VideoCORE& m_core;
        TFeatureList m_features;
        StorageRW m_storage;
        mfxStatus m_runtimeErr = MFX_ERR_NONE;

        mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a);
        mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts);

        static mfxStatus Execute(void *pState, void *task, mfxU32 uid_p, mfxU32 uid_a)
        {
            if (pState)
                return ((MFXVideoENCODEH265_HW*)pState)->Execute(task, uid_p, uid_a);
            else
                return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        static mfxStatus FreeResources(void *pState, void *task, mfxStatus sts)
        {
            if (pState)
                return ((MFXVideoENCODEH265_HW*)pState)->FreeResources(task, sts);
            else
                return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        template<class T>
        T* FindFeature(mfxU32 id)
        {
            using TRFeature = decltype(m_features.front());
            auto it = std::find_if(m_features.begin(), m_features.end()
                , [id](TRFeature pFeature) { return pFeature->GetID() == id; });
            if (it == m_features.end())
            {
                return nullptr;
            }
            return dynamic_cast<T*>(it->get());
        }

        template<class T>
        T& GetFeature(mfxU32 id)
        {
            T* pFeature = FindFeature<T>(id);
            ThrowAssert(!pFeature, "Feature not found");
            return *pFeature;
        }
    };

} //Base
}// namespace HEVCEHW

#endif
