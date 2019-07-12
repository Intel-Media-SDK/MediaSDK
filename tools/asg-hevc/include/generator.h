// Copyright (c) 2018-2019 Intel Corporation
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

#ifndef __GENERATOR_H__
#define __GENERATOR_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include "refcontrol.h"
#include "test_processor.h"

class Generator : public TestProcessor {
public:
    explicit Generator() {}

    ~Generator() {}

private:
    void Init() override;

    // Get surface and load new YUV frame from file to it
    ExtendedSurface* PrepareSurface() override;

    // Save all data
    void DropFrames() override;
    void DropBuffers(ExtendedSurface& surf) override;
    virtual void SavePSData() override;

    CSmplYUVReader m_FileReader;
    CSmplYUVWriter m_FileWriter;
    BufferWriter   m_BufferWriter;
};

#endif // MFX_VERSION

#endif // __GENERATOR_H__
