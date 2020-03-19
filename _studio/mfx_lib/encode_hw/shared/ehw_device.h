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

#pragma once
#include "feature_blocks/mfx_feature_blocks_utils.h"
#include "libmfx_core_interface.h"
#include <list>

namespace MfxEncodeHW
{

struct DDIExecParam
{
    mfxU32 Function = 0;
    struct Param
    {
        void*  pData    = nullptr;
        mfxU32 Size     = 0;
        mfxU32 Num      = 0;

        template<class T>
        T& Cast(mfxU32 idx = 0) const
        {
            bool bInvalid = !pData
                || (Size * std::max<mfxU32>(Num, 1)) < (sizeof(T) * (idx + 1));
            if (bInvalid)
                throw std::logic_error("Invalid DDIExecParam::Param data");
            return *((T*)pData);
        }
    } In, Out, Resource;

    template<mfxU32 F>
    static bool IsFunction(const DDIExecParam& xpar) { return xpar.Function == F; }
};

struct DDIFeedback
{
    using TGet    = MfxFeatureBlocks::CallChain<const void*, mfxU32>;
    using TUpdate = MfxFeatureBlocks::CallChain<mfxStatus, mfxU32>;

    std::list<DDIExecParam> ExecParam;
    TGet                    Get;
    TUpdate                 Update;
    TUpdate                 Remove;
    bool                    bNotReady = false;
};

struct PackedData
{
    mfxU8* pData;
    mfxU32 BitLen;
    bool   bHasEP;
    bool   bLongSC;
    std::map<mfxU32, mfxU32> PackInfo;
};

class Device
{
public:
    virtual ~Device() {}

    virtual mfxStatus Create(
        VideoCORE&    core
        , GUID        guid
        , mfxU32      width
        , mfxU32      height
        , bool        isTemporal) = 0;

    virtual bool      IsValid() const = 0;
    virtual mfxStatus QueryCaps(void* pCaps, mfxU32 size) = 0;
    virtual mfxStatus QueryCompBufferInfo(mfxU32, mfxFrameInfo&) = 0;
    virtual mfxStatus Init(const std::list<DDIExecParam>*) = 0;
    virtual mfxStatus Execute(const DDIExecParam&) = 0;
    virtual mfxStatus QueryStatus(DDIFeedback& fb, mfxU32 id) = 0;
    virtual mfxStatus BeginPicture(mfxHDL) = 0;
    virtual mfxStatus EndPicture() = 0;
    virtual mfxU32    GetLastErr() const = 0;
    virtual void      Trace(const DDIExecParam&, bool /*bAfterExec*/, mfxU32 /*res*/) {}
};

} //namespace MfxEncodeHW