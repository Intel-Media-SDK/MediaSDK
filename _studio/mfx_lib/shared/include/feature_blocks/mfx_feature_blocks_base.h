// Copyright (c) 2019 Intel Corporation
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
#include "mfx_feature_blocks_utils.h"

#include <list>
#include <map>
#include <exception>
#include <functional>

namespace MfxFeatureBlocks
{

enum eFeatureMode
{
      QUERY0        = (1 << 0)
    , QUERY1        = (1 << 1)
    , QUERY_IO_SURF = (1 << 2)
    , INIT          = (1 << 3)
    , RUNTIME       = (1 << 4)
};

enum eReorderLoc
{
    PLACE_BEFORE
    , PLACE_AFTER
};

struct ParamSupport
{
    std::list<std::function<void(const mfxVideoParam*, mfxVideoParam*)>>
        m_mvpCopySupported;
    std::map<mfxU32, std::list<std::function<void(const mfxExtBuffer*, mfxExtBuffer*)>>>
        m_ebCopySupported
        , m_ebCopyPtrs;
};

struct ParamInheritance //for Reset
{
    std::list<std::function<void(const mfxVideoParam*, mfxVideoParam*)>>
        m_mvpInheritDefault;
    std::map<mfxU32, std::list<std::function<void(const mfxVideoParam&, const mfxExtBuffer*, const mfxVideoParam&, mfxExtBuffer*)>>>
        m_ebInheritDefault;
};

struct ID
{
    mfxU32 FeatureID;
    mfxU32 BlockID;

    bool operator==(const ID other) const
    {
        return
            FeatureID == other.FeatureID
            && BlockID == other.BlockID;
    }

    bool operator<(const ID other) const
    {
        if (FeatureID < other.FeatureID)
            return true;
        return BlockID < other.BlockID;
    }
};

template<class TBlockTracer>
class FeatureBlocksCommon
    : public ParamSupport
    , public ParamInheritance
{
public:
    using ID = MfxFeatureBlocks::ID;

    virtual ~FeatureBlocksCommon() {}

    template<class T>
    struct Block : ID
    {
        typedef T TCall;

        Block(
            const ID id
            , TCall&& call
            , const char* fName = nullptr
            , const char* bName = nullptr)
            : ID(id)
            , m_featureName(fName)
            , m_blockName(bName)
            , m_call(std::move(call))
        {}

        template<class... TArg>
        typename TCall::result_type Call(TArg&& ...arg) const
        {
            TBlockTracer tr(*this, m_featureName, m_blockName);
            return m_call(std::forward<TArg>(arg)...);
        }

        const char* m_featureName;
        const char* m_blockName;
        TCall m_call;
    };

    template<class T>
    static typename std::conditional<std::is_const<T>::value
            , typename T::const_iterator, typename T::iterator>::type
    Find(T& queue, const ID id)
    {
        return std::find_if(
            queue.begin()
            , queue.end()
            , [id](const ID x) {
            return (x == id);
        });
    }

    template<class T>
    static typename std::conditional<std::is_const<T>::value
        , typename T::const_iterator, typename T::iterator>::type
    Get(T& queue, const ID id)
    {
        auto it = Find(queue, id);
        if (it == queue.end())
            throw std::logic_error("Block not found");
        return it;
    }

    template<class T>
    void Reorder(T& queue, const ID _where, const ID _first, eReorderLoc loc = PLACE_BEFORE)
    {
        auto itWhere = Get(queue, _where);
        if (loc == PLACE_AFTER)
            itWhere++;
        queue.splice(itWhere, queue, Get(queue, _first));
    }

    mfxU32 Init(mfxU32 FeatureID, mfxU32 mode)
    {
        mfxU32 prevMode = m_initialized[FeatureID];
        m_initialized[FeatureID] |= mode;
        return prevMode;
    }

    virtual const char* GetFeatureName(mfxU32 /*featureID*/) { return nullptr; }
    virtual const char* GetBlockName(ID /*id*/) { return nullptr; }

    std::map<mfxU32, mfxU32> m_initialized; //FeatureID -> FeatureMode
};

class IBlockTracer
{
public:
    IBlockTracer(
        ID /*id*/
        , const char* /*fName = nullptr*/
        , const char* /*bName = nullptr*/)
    {}
    virtual ~IBlockTracer()
    {}
};

template<class TFeatureBlocks>
class FeatureBaseCommon
{
public:
    FeatureBaseCommon() = delete;
    virtual ~FeatureBaseCommon() {}

    virtual void Init(
        mfxU32 mode/*eFeatureMode*/
        , TFeatureBlocks& blocks) = 0;

    mfxU32 GetID() const { return m_id; }

protected:
    virtual void SetSupported(ParamSupport& /*par*/) {}
    virtual void SetInherited(ParamInheritance& /*par*/) {}

    FeatureBaseCommon(mfxU32 id)
        : m_id(id)
    {}

    template<class T, class TFeatureBase>
    using TPfnInitQueue = void (TFeatureBase::*)(const TFeatureBlocks&, T);

    template<mfxU32 ABR, class T, class TFeatureBase>
    bool InitQueue(
        TPfnInitQueue<T, TFeatureBase> pfnFunc
        , TFeatureBlocks& blocks)
    {
        (dynamic_cast<TFeatureBase&>(*this).*pfnFunc)(blocks
            , [&](mfxU32 blkId, typename TFeatureBlocks::template BQ<ABR>::TCall&& call)
        {
            blocks.template Push<ABR>(ID{ m_id, blkId }, std::move(call));
        });
        return true;
    }
    
    mfxU32 m_id;
};

}; //namespace MfxFeatureBlocks
