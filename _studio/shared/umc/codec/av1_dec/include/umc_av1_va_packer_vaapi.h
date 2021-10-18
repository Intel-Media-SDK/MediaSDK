// Copyright (c) 2017-2020 Intel Corporation
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



#include "umc_defs.h"
#ifdef MFX_ENABLE_AV1_VIDEO_DECODE

#ifndef __UMC_AV1_VA_PACKER_H

#include "umc_av1_va_packer.h"

#define __UMC_AV1_VA_PACKER_H

namespace UMC
{
    class MediaData;
}

namespace UMC_AV1_DECODER
{
    class VP9Bitstream;
    class VP9DecoderFrame;

    class PackerVA
        : public Packer
    {
    public:
        PackerVA(UMC::VideoAccelerator * va);

        UMC::Status GetStatusReport(void * pStatusReport, size_t size) override;
        UMC::Status SyncTask(int32_t index, void * error) override
        {
            return m_va->SyncTask(index, error);
        }
        void BeginFrame() override;
        void EndFrame() override;

        void PackAU(std::vector<TileSet>&, AV1DecoderFrame const&, bool) override;

    private:
        void PackPicParams(VADecPictureParameterBufferAV1&, AV1DecoderFrame const&);
        void PackTileControlParams(VASliceParameterBufferAV1&, TileLocation const&);
        void PackPriorityParams();
    };

} // namespace UMC_AV1_DECODER

#endif /* __UMC_AV1_VA_PACKER_H */
#endif // MFX_ENABLE_AV1_VIDEO_DECODE

