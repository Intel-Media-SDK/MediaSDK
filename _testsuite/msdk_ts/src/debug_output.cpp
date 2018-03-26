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

#if (defined(_WIN32) || defined(_WIN64))

#include <windows.h>
#include <iostream>
#include <sstream>

/// \brief This class is a derivate of basic_stringbuf which will output all the written data using the OutputDebugString function
template<typename TChar, typename TTraits = std::char_traits<TChar>, int BUF_SIZE = 512>
class OutputDebugStringBuf : public std::basic_stringbuf<TChar,TTraits> {
public:
    OutputDebugStringBuf() {

        psz = new char_type[ BUF_SIZE ];
        setbuf(psz, BUF_SIZE);
        // leave place for single char + 0 terminator
        setp(psz, psz + BUF_SIZE - 2);
        ::InitializeCriticalSection(&_csLock);
    }

    ~OutputDebugStringBuf() {
        ::DeleteCriticalSection(&_csLock);
        delete[] psz;
    }

protected:
    virtual int sync() {
        overflow();
        return 0;
    }

    virtual int_type overflow(int_type c = TTraits::eof()) {

        ::EnterCriticalSection(&_csLock);
        char_type* plast = pptr();
        if (c != TTraits::eof())
            // add c to buffer
            *plast++ = c;
        *plast = char_type();

        // Pass test to debug output
        MessageOutputer<TChar, TTraits>  out;
        out(pbase());

        setp(pbase(), epptr());

        ::LeaveCriticalSection(&_csLock);
        return c != TTraits::eof() ? TTraits::not_eof( c ) : TTraits::eof();
    }

    virtual std::streamsize xsputn(const char_type* pch, std::streamsize n)
    {
        std::streamsize nMax, nPut;
        ::EnterCriticalSection(&_csLock);
        for (nPut = 0; 0 < n; ) {
            if (pptr() != 0 && 0 < (nMax = epptr() - pptr())) {
                if (n < nMax)
                    nMax = n;
                traits_type::copy(pptr(), pch, (size_t)nMax);

                // Sync if string contains LF
                bool bSync = traits_type::find( pch, (size_t)nMax, traits_type::to_char_type( '\n' ) ) != NULL;
                pch += nMax, nPut += nMax, n -= nMax, pbump((int)nMax);
                if( bSync )
                    sync();
            }
            else if (traits_type::eq_int_type(traits_type::eof(),
                overflow(traits_type::to_int_type(*pch))))
                break;
            else
                ++pch, ++nPut, --n;
        }
        ::LeaveCriticalSection(&_csLock);
        return (nPut);
    }

private:
    char_type*        psz;
    CRITICAL_SECTION  _csLock;

    template<typename TChar, typename TTraits>
    struct MessageOutputer;

    template<>
    struct MessageOutputer<char, std::char_traits<char>> {
        void operator()(const char * msg) const {
            OutputDebugStringA(msg);
        }
    };

    template<>
    struct MessageOutputer<wchar_t, std::char_traits<wchar_t>> {
        void operator()(const wchar_t * msg) const {
            OutputDebugStringW(msg);
        }
    };
}; //OutputDebugStringBuf
#endif //#if (defined(_WIN32) || defined(_WIN64))

void allow_debug_output()
{
#ifndef NDEBUG
#ifdef _WIN32

    if (!IsDebuggerPresent())
        return;

    static OutputDebugStringBuf<wchar_t> wcharDebugOutput;
    std::wcerr.rdbuf(&wcharDebugOutput);
    std::wclog.rdbuf(&wcharDebugOutput);
    std::wcout.rdbuf(&wcharDebugOutput);

    static OutputDebugStringBuf<char> charDebugOutput;
    std::cerr.rdbuf(&charDebugOutput);
    std::clog.rdbuf(&charDebugOutput);
    std::cout.rdbuf(&charDebugOutput);

#endif
#endif
}
