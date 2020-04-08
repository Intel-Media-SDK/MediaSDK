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

//This file is included from [umc_h265_va_packer_vaapi_g12.hpp]
//DO NOT include it into the project manually

#ifndef UMC_VA_H265_PACKER_G12_COMMON
#define UMC_VA_H265_PACKER_G12_COMMON

#if (MFX_VERSION >= 1032)

namespace UMC_HEVC_DECODER
{
    namespace G12
    {
        template <typename T, typename V>
        inline
        void FillPaletteEntries(T const* src, V dst[3][128], size_t count, size_t numComps)
        {
            assert(!(count > 128));
            assert(!(numComps > 3));

            for (size_t i = 0; i < numComps; ++i, src += count)
            {
                for (size_t j = 0; j < count; ++j)
                    dst[i][j] = static_cast<V>(*(src + j));
            }
        }

        inline
        uint32_t GetEntryPointOffsetStep(H265Slice const* slice, uint32_t tileYIdx)
        {
            H265PicParamSet const* pps = slice->GetPicParam();
            VM_ASSERT(pps);

            if (!pps->tiles_enabled_flag || !pps->entropy_coding_sync_enabled_flag)
                return 1;

            if (!(tileYIdx < pps->row_height.size()))
                throw h265_exception(UMC::UMC_ERR_FAILED);

            return slice->getTileRowHeight(tileYIdx);
        }

        inline
        uint32_t GetEntryPointOffsetNum(H265Slice const* slice)
        {
            H265PicParamSet const* pps = slice->GetPicParam();
            assert(pps);
            //H265SliceHeader const* sh = slice->GetSliceHeader();
            //VM_ASSERT(sh);

            uint32_t count = 0;
            uint32_t tileXIdx = slice->getTileXIdx();
            uint32_t tileYIdx = slice->getTileYIdx();
            uint32_t step = 0;

            step = GetEntryPointOffsetStep(slice, tileYIdx);
            for (uint32_t idx = 1; idx < slice->getTileLocationCount(); idx++)
            {
                if (0 == --step)
                {
                    count++;
                    if (++tileXIdx > (pps->num_tile_columns - 1))
                    {
                        tileXIdx = 0;
                        tileYIdx++;
                    }
                    step = GetEntryPointOffsetStep(slice, tileYIdx);
                }
            }
            return count;
        }

        /* Return number (pair.first) of entry point offsets and
           the first entry position (pair.second) of the entry point offsets for given slice */
        inline
        std::pair<uint16_t, uint16_t>
        GetEntryPoint(H265Slice const* slice)
        {
            assert(slice);

            auto sh = slice->GetSliceHeader();
            assert(sh);

            if (!sh->num_entry_point_offsets)
                return std::make_pair(0, 0);

            auto frame = slice->GetCurrentFrame();
            assert(frame);

            auto fi = frame->GetAU();
            assert(fi);

            uint32_t offset = 0;
            auto const count = fi->GetSliceCount();
            for (uint32_t i = 0; i < count; ++i)
            {
                auto s = fi->GetSlice(i);
                if (NULL == s)
                    throw h265_exception(UMC::UMC_ERR_FAILED);

                if (s == slice)
                    break;

                offset += GetEntryPointOffsetNum(s);;
            }

            return std::make_pair(
                static_cast<uint16_t>(offset),
                static_cast<uint16_t>(GetEntryPointOffsetNum(slice))
            );
        }

        template <typename T>
        inline
        void FillSubsets(H265DecoderFrameInfo const* fi, T begin, T end)
        {
            assert(fi);

            uint32_t const count = fi->GetSliceCount();
            for (uint32_t i = 0; i < count; ++i)
            {
                auto slice = fi->GetSlice(i);
                if (NULL == slice)
                    throw h265_exception(UMC::UMC_ERR_FAILED);

                H265PicParamSet const* pps = slice->GetPicParam();

                uint32_t tileXIdx = slice->getTileXIdx();
                uint32_t tileYIdx = slice->getTileYIdx();

                auto step = GetEntryPointOffsetStep(slice, tileYIdx);

                //'m_tileByteLocation' contains absolute offsets, but we have to pass relative ones just as they are in a bitstream
                //NOTE: send only entry points for tiles
                auto position = slice->m_tileByteLocation[0];
                for (uint32_t j = step; j < slice->getTileLocationCount(); j += step)
                {
                    uint32_t const entry = slice->m_tileByteLocation[j];
                    *begin++ = entry - (position  + 1);
                    position  = entry;

                    if (begin > end)
                        //provided buffer for entry points is too small
                        throw h265_exception(UMC::UMC_ERR_FAILED);

                    if (++tileXIdx > (pps->num_tile_columns - 1))
                    {
                        tileXIdx = 0;
                        tileYIdx++;
                    }

                    step = GetEntryPointOffsetStep(slice, tileYIdx);
                }
            }
        }
    } //G12
}

#endif //MFX_VERSION >= MFX_VERSION_NEXT
#endif //UMC_VA_H265_PACKER_G12
