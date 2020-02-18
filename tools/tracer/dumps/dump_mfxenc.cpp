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

#include "dump.h"

std::string DumpContext::dump(const std::string structName, const mfxENCInput &encIn)
{
    std::string str;
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(encIn.reserved) + "\n";
    str += structName + ".InSurface=" + ToString(encIn.InSurface) + "\n";
    str += structName + ".NumFrameL0=" + ToString(encIn.NumFrameL0) + "\n";
    str += structName + ".L0Surface=" + ToString(encIn.L0Surface) + "\n";
    str += structName + ".NumFrameL1=" + ToString(encIn.NumFrameL1) + "\n";
    str += structName + ".L1Surface=" + ToString(encIn.L1Surface) + "\n";
    str += dump_mfxExtParams(structName, encIn);
    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxENCOutput &encOut)
{
    std::string str;
    str += structName + ".reserved[]=" + DUMP_RESERVED_ARRAY(encOut.reserved) + "\n";
    str += dump_mfxExtParams(structName, encOut);
    return str;
}
