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

#ifndef __ASGLOG_H__
#define __ASGLOG_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

#include <iostream>
#include <string>
#include <sstream>
#include <tuple>
#include <iomanip>

#include "mfxdefs.h"
#include "block_structures.h"

//Logger object, basically a tee into cout and log file
class ASGLog
{
public:
    //Tuple field designations
    enum { MVX = 0, MVY = 1, REFIDX = 2 };

    void Init(const InputParams& params)
    {
        m_bVerbose = params.m_bVerbose;
        if (params.m_bUseLog)
        {
            m_LogFileOutput.open(params.m_LogFileName.c_str(), std::ofstream::out);
            if (!m_LogFileOutput.is_open())
            {
                throw std::string("ERROR: ASGLog: unable to open log file");
            }
        }
    }

    //Convenient ofstream-style operator<< with chaining feature
    template<class T>
    ASGLog& operator<<(const T& output)
    {
        if (m_bVerbose)
        {
            std::cout << output;
        }
        if (m_LogFileOutput.is_open())
        {
            m_LogFileOutput << output;
        }
        return *this;
    }

    //Specialization for basic block info output
    ASGLog& operator<< (const CTUDescriptor& outCTU)
    {
        return (*this << "CTU " << static_cast<BaseBlock>(outCTU));
    }

    ASGLog& operator<< (const CUBlock& outCU)
    {
        return (*this << "CU " << static_cast<BaseBlock>(outCU));
    }

    ASGLog& operator<< (const PUBlock& outPU)
    {
        return (*this << "PU " << static_cast<BaseBlock>(outPU));
    }

    //Specialization for basic block info output
    ASGLog& operator<< (const BaseBlock& outBB)
    {
        std::stringstream ss;
        ss << outBB.m_BHeight << "x" << outBB.m_BWidth <<
            " (" << outBB.m_AdrX << ";" << outBB.m_AdrY << ")";
        return (*this << ss.str());
    }

    //Specialization for MV log output
    ASGLog& operator<< (const std::tuple<mfxI32, mfxI32, mfxU32>& MV)
    {
        std::stringstream ss;

        ss << "(" << std::get<MVX>(MV) << ";" << std::get<MVY>(MV) << ";" << std::get<REFIDX>(MV) << ")";

        return (*this << ss.str());
    }

    //Specialization for I/O stream manipulators (i.e. std::endl)
    ASGLog& operator<< (std::ostream&(*pMan)(std::ostream&))
    {
        if (m_bVerbose)
        {
            std::cout << *pMan;
        }
        if (m_LogFileOutput.is_open())
        {
            m_LogFileOutput << *pMan;
        }
        return *this;
    }
    ~ASGLog()
    {
        if (m_LogFileOutput.is_open())
        {
            m_LogFileOutput.close();
        }
    }

private:
    bool m_bVerbose = false;
    std::ofstream m_LogFileOutput;
};

#endif // MFX_VERSION > 1024

#endif //__ASGLOG_H__
