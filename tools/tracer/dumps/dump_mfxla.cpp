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

std::string DumpContext::dump(const std::string structName, const mfxExtLAControl &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";
    DUMP_FIELD(LookAheadDepth);
    DUMP_FIELD(DependencyDepth);
    DUMP_FIELD(DownScaleFactor);
    DUMP_FIELD(BPyramid);

    DUMP_FIELD_RESERVED(reserved1);

    DUMP_FIELD(NumOutStream);

    str += structName + ".OutStream[]={\n";
    for (int i = 0; i < _struct.NumOutStream; i++)
    {
        str += dump("", _struct.OutStream[i]) + ",\n";
    }
    str += "}\n";

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtLAControl::mfxStream &_struct)
{
    std::string str;

    DUMP_FIELD(Width);
    DUMP_FIELD(Height);

    DUMP_FIELD_RESERVED(reserved2);

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxLAFrameInfo &_struct)
{
    std::string str;
    
    DUMP_FIELD(Width);
    DUMP_FIELD(Height);

    DUMP_FIELD(FrameType);
    DUMP_FIELD(FrameDisplayOrder);
    DUMP_FIELD(FrameEncodeOrder);

    DUMP_FIELD(IntraCost);
    DUMP_FIELD(InterCost);
    DUMP_FIELD(DependencyCost);
    DUMP_FIELD(Layer);
    DUMP_FIELD_RESERVED(reserved);

    DUMP_FIELD_RESERVED(EstimatedRate);

    return str;
}

std::string DumpContext::dump(const std::string structName, const mfxExtLAFrameStatistics &_struct)
{
    std::string str;

    str += dump(structName + ".Header", _struct.Header) + "\n";

    DUMP_FIELD_RESERVED(reserved);

    DUMP_FIELD(NumAlloc);
    DUMP_FIELD(NumStream);
    DUMP_FIELD(NumFrame);
    

    str += structName + ".FrameStat[]={\n";
    for (int i = 0; i < _struct.NumStream; i++)
    {
        str += dump("", _struct.FrameStat[i]) + ",\n";
    }
    str += "}\n";


    if (_struct.OutSurface)
    {
        str += dump(structName + ".OutSurface", *_struct.OutSurface) + "\n";
    }
    else
    {
        str += structName + ".OutSurface=NULL\n";
    }

    return str;
}
