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

#define MFX_FEATURE_BLOCKS_DECLARE_BQ_UTILS_IN_FEATURE_BLOCK \
template<class T> void Push(T& queue, const ID id, typename T::value_type::TCall&& call) \
{                                                                           \
    queue.emplace_back(typename T::value_type(id, std::move(call)           \
        , GetFeatureName(id.FeatureID), GetBlockName(id)));                 \
}                                                                           \
template<mfxU32 QID> struct BQ {};                                          \
template<mfxU32 QID> void Push(const ID id, typename BQ<QID>::TCall&& call) \
{                                                                           \
    BQ<QID>::Push(*this, id, std::move(call));                              \
}

#define MFX_FEATURE_BLOCKS_DECLARE_QUEUES_IN_FEATURE_BLOCK(NAME, ABR, RTYPE, ...)\
    static const mfxU32 BQ_##NAME = __LINE__;\
    std::list<Block<std::function<RTYPE(__VA_ARGS__)>>> m_q##NAME;

#define MFX_FEATURE_BLOCKS_DECLARE_QUEUES_EXTERNAL(NAME, ABR, RTYPE, ...)\
template<> struct FeatureBlocks::BQ <FeatureBlocks::BQ_##NAME>\
{\
    typedef std::function<RTYPE(__VA_ARGS__)> TCall;\
    typedef std::list<FeatureBlocks::Block<TCall>> TQueue;\
    static TQueue& Get(FeatureBlocks& blk) { return blk.m_q##NAME;}\
    static const TQueue& Get(const FeatureBlocks& blk) { return blk.m_q##NAME;}\
    static void Push(FeatureBlocks& blks, const ID id, TCall&& call)\
    { blks.Push(blks.m_q##NAME, id, std::move(call)); }\
};

#define MFX_FEATURE_BLOCKS_DECLARE_QUEUES_IN_FEATURE_BASE(NAME, ABR, RTYPE, ...)\
    static const mfxU32 ABR = FeatureBlocks::BQ_##NAME;\
    typedef FeatureBlocks::BQ<ABR>::TCall TCall##ABR;\
    typedef std::function<void(mfxU32 blkId, TCall##ABR&&)> TPush##ABR;\
    virtual void NAME(const FeatureBlocks&, TPush##ABR) {}
