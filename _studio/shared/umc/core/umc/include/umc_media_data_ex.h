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

#ifndef __UMC_MEDIA_DATA_EX_H__
#define __UMC_MEDIA_DATA_EX_H__

#include "umc_media_data.h"

namespace UMC
{

class MediaDataEx : public MediaData
{
    DYNAMIC_CAST_DECL(MediaDataEx, MediaData)

public:
    class _MediaDataEx{
        DYNAMIC_CAST_DECL_BASE(_MediaDataEx)
        public:
        uint32_t count;
        uint32_t index;
        unsigned long long bstrm_pos;
        uint32_t *offsets;
        uint32_t *values;
        uint32_t limit;

        _MediaDataEx()
        {
            count = 0;
            index = 0;
            bstrm_pos = 0;
            limit   = 2000;
            offsets = (uint32_t*)malloc(sizeof(uint32_t)*limit);
            values  = (uint32_t*)malloc(sizeof(uint32_t)*limit);
        }

        virtual ~_MediaDataEx()
        {
            if(offsets)
            {
                free(offsets);
                offsets = 0;
            }
            if(values)
            {
                free(values);
                values = 0;
            }
            limit   = 0;
        }
    };

    // Default constructor
    MediaDataEx()
    {
        m_exData = NULL;
    };

    // Destructor
    virtual ~MediaDataEx(){};

    _MediaDataEx* GetExData()
    {
        return m_exData;
    };

    void SetExData(_MediaDataEx* pDataEx)
    {
        m_exData = pDataEx;
    };

protected:
    _MediaDataEx *m_exData;
};

}

#endif //__UMC_MEDIA_DATA_EX_H__

