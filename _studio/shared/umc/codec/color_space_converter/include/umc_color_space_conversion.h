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

#ifndef __UMC_COLOR_SPACE_CONVERSION_H__
#define __UMC_COLOR_SPACE_CONVERSION_H__

#include "umc_base_codec.h"

namespace UMC
{

class ColorSpaceConversion : public BaseCodec
{
  DYNAMIC_CAST_DECL(ColorSpaceConversion, BaseCodec)
public:
  // Initialize codec with specified parameter(s)
  virtual Status Init(BaseCodecParams *) { return UMC_OK; };

  // Convert next frame
  virtual Status GetFrame(MediaData *in, MediaData *out);

  // Get codec working (initialization) parameter(s)
  virtual Status GetInfo(BaseCodecParams *) { return UMC_OK; };

  // Close all codec resources
  virtual Status Close(void) { return UMC_OK; };

  // Set codec to initial state
  virtual Status Reset(void) { return UMC_OK; };

private:
  Status GetFrameInternal(MediaData *in, MediaData *out);
};

} // namespace UMC

#endif /* __UMC_COLOR_SPACE_CONVERSION_H__ */
