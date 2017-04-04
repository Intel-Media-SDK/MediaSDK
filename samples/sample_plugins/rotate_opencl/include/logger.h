/******************************************************************************\
Copyright (c) 2005-2017, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#pragma once

#include <iostream>
#include "sample_utils.h"

///////////////////////////////////////////////////////////////////////////////
//! delegates operator << calls to underlying ostream with
//! on == true, and swallows the input with on == false
template <bool on = true>
class OnOffFilter
{
private:
    std::ostream *m_os;
public:
    OnOffFilter(std::ostream &os) : m_os(&os)
    {
    }

    //! delegates << to m_os
    template <typename Message>
    OnOffFilter &operator <<(const Message& message)
    {
        (*m_os) << message;
        return *this;
    }

    //! handles manipulators like endl, etc.
    typedef std::ostream& (*OstreamManipulator)(std::ostream&);
    OnOffFilter& operator<<(OstreamManipulator manip)
    {
        manip(*m_os);
        return *this;
    }
};

//! swallow message if on == false
template <>
template <typename Message>
OnOffFilter<false> &OnOffFilter<false>::operator <<(const Message& message)
{
    return *this;
}

//! swallow manip if on == false
template <>
inline OnOffFilter<false>& OnOffFilter<false>::operator<<(OnOffFilter<false>::OstreamManipulator manip)
{
    return *this;
}
                                                                           ////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//! available LogLevels
enum LogLevel
{
    None       = 0,
    Fatal      = 1,
    Error      = 2,
    Warning    = 3,
    Info       = 4,
    Debug      = 5,
};

////////////////////////////////////////////////////////////////////////////////
//! swallows messages with level higher than specified on object creation
template <LogLevel level = Info>
class Logger : private no_copy
{
private:
    std::ostream &m_os;

public:
    Logger(std::ostream &os) : m_os(os)
    {
    }

    OnOffFilter<level >= Fatal>   fatal()   { return OnOffFilter<level >= Fatal>  (m_os); }
    OnOffFilter<level >= Error>   error()   { return OnOffFilter<level >= Error>  (m_os); }
    OnOffFilter<level >= Warning> warning() { return OnOffFilter<level >= Warning>(m_os); }
    OnOffFilter<level >= Info>    info()    { return OnOffFilter<level >= Info>   (m_os); }
    OnOffFilter<level >= Debug>   debug()   { return OnOffFilter<level >= Debug>  (m_os); }
};

template <typename T>
std::string toString(const T& t)
{
    std::ostringstream oss;
    oss << t;
    return oss.str();
}