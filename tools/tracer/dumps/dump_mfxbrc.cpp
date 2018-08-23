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

#include "dump.h"

std::string DumpContext::dump(const std::string structName, const mfxExtBRC &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";

    DUMP_FIELD_RESERVED(reserved);

    DUMP_FIELD(pthis);
    DUMP_FIELD(Init);
    DUMP_FIELD(Reset);
    DUMP_FIELD(Close);
    DUMP_FIELD(GetFrameCtrl);
    DUMP_FIELD(Update);

    DUMP_FIELD_RESERVED(reserved1);

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxBRCFrameParam &_struct)
{
    std::string str;

    DUMP_FIELD_RESERVED(reserved);

    DUMP_FIELD(EncodedOrder);
    DUMP_FIELD(DisplayOrder);
    DUMP_FIELD(CodedFrameSize);
    DUMP_FIELD(FrameType);
    DUMP_FIELD(PyramidLayer);
    DUMP_FIELD(NumRecode);
    dump_mfxExtParams(structName, _struct);

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxBRCFrameCtrl &_struct)
{
    std::string str;

    DUMP_FIELD(QpY);
    DUMP_FIELD_RESERVED(reserved1);
    DUMP_FIELD(reserved2);

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxBRCFrameStatus &_struct)
{
    std::string str;

    DUMP_FIELD(MinFrameSize);
    DUMP_FIELD(BRCStatus);
    DUMP_FIELD_RESERVED(reserved);
    DUMP_FIELD(reserved1);

    return str;
}