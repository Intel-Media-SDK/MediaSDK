// Copyright (c) 2017 Intel Corporation
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
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#ifndef __UMC_NOTIFY_H265
#define __UMC_NOTIFY_H265

#include <list>

namespace UMC_HEVC_DECODER
{

// Abstract notification class
class notifier_base
{
public:
    notifier_base()
        : next_(0)
        , m_isNeedNotification(true)
    {
    }

    virtual ~notifier_base()
    {}

    virtual void Notify() = 0;

    void ClearNotification() { m_isNeedNotification = false; }

    notifier_base * next_;

protected:
    bool m_isNeedNotification;
};

// Destructor callback class
template <typename Object>
class notifier0 : public notifier_base
{

public:

    typedef void (Object::*Function)();

    notifier0(Function function)
        : function_(function)
    {
        Reset(0);
    }

    notifier0(Object* object, Function function)
        : function_(function)
    {
        Reset(object);
    }

    ~notifier0()
    {
        Notify();
    }

    void Reset(Object* object)
    {
        object_ = object;
        m_isNeedNotification = !!object_;
    }

    // Call callback function
    void Notify()
    {
        if (m_isNeedNotification)
        {
            m_isNeedNotification = false;
            (object_->*function_)();
        }
    }

private:

    Object* object_;
    Function function_;
};

} // namespace UMC_HEVC_DECODER

#endif // __UMC_NOTIFY_H265
#endif // MFX_ENABLE_H265_VIDEO_DECODE
