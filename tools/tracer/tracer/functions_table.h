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
#ifndef FUNCTIONSTABLE_H_
#define FUNCTIONSTABLE_H_

#include "mfxvideo.h"
#include "mfxplugin.h"
#include <map>


#include "../loggers/log.h"
#include "../dumps/dump.h"

#define TRACE_CALLBACKS 0

#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
    e##func_name##_tracer,

typedef enum _mfxFunction
{
    eMFXInit_tracer,
    eMFXClose_tracer,
#include "bits/mfxfunctions.h"
    eFunctionsNum,
    eNoMoreFunctions = eFunctionsNum
} mfxFunction;

#if TRACE_CALLBACKS
#undef FUNCTION
#define FUNCTION(storage, return_value, func_name, formal_param_list, actual_param_list) \
    e##storage##_##func_name##_tracer,

typedef enum _mfxCallbacks
{
#define CALLBACKS_COMMON
#include "bits/mfxcallbacks.h"
    eCallbacksNum,
    eNoMoreCallbacks = eCallbacksNum,
#undef CALLBACKS_COMMON
    eInvalidCallback = -1,
#define CALLBACKS_PLUGIN
#include "bits/mfxcallbacks.h"
    ePluginCallbacksNum,
    eNoMorePluginCallbacks = ePluginCallbacksNum,
#define CALLBACKS_COMMON
} mfxCallbacks;
#endif //TRACE_CALLBACKS

#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
   { e##func_name##_tracer, #func_name },

typedef struct _mfxFunctionsTable
{
    mfxFunction id;
    const char* name;
} mfxFunctionsTable;

static const mfxFunctionsTable g_mfxFuncTable[] =
{
    { eMFXInit_tracer, "MFXInit" },
    { eMFXClose_tracer, "MFXClose" },
#include "bits/mfxfunctions.h"
    { eNoMoreFunctions, NULL }
};

typedef void (MFX_CDECL * mfxFunctionPointer)(void);

typedef mfxStatus (MFX_CDECL * MFXInitPointer)(mfxIMPL, mfxVersion *, mfxSession *);
typedef mfxStatus (MFX_CDECL * MFXInitExPointer)(mfxInitParam, mfxSession *);
typedef mfxStatus (MFX_CDECL * MFXClosePointer)(mfxSession session);

typedef struct _mfxLoader
{
    mfxSession session;
    void* dlhandle;
    mfxFunctionPointer table[eFunctionsNum];
#if TRACE_CALLBACKS
    void* callbacks[eCallbacksNum][2];
    struct Plugin
    {
        _mfxLoader* pLoaderBase;
        void* callbacks[ePluginCallbacksNum][2];
    } plugin[MFX_PLUGINTYPE_VIDEO_ENC + 1];
#endif //TRACE_CALLBACKS
} mfxLoader;

// function types
#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
    typedef return_value (MFX_CDECL * f##func_name) formal_param_list;

#include "bits/mfxfunctions.h"

#if TRACE_CALLBACKS
#undef FUNCTION
#define FUNCTION(storage, return_value, func_name, formal_param_list, actual_param_list) \
    typedef return_value (MFX_CDECL * f##storage##_##func_name) formal_param_list;

#include "bits/mfxcallbacks.h"
#endif //TRACE_CALLBACKS

// forward declarations
#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
    return_value func_name formal_param_list;

#include "bits/mfxfunctions.h"

#if TRACE_CALLBACKS
#undef FUNCTION
#define FUNCTION(storage, return_value, func_name, formal_param_list, actual_param_list) \
    return_value storage##_##func_name formal_param_list;

#include "bits/mfxcallbacks.h"

template<class T>
class CallbackBackup
{
public:
    CallbackBackup(T* storage) : m_storage(storage) {};

    void New(mfxCallbacks key, void* newcb0, void* newcb1)
    {
        m_cb[key].first  = m_storage[0][key][0];
        m_cb[key].second = m_storage[0][key][1];
        m_storage[0][key][0] = newcb0;
        m_storage[0][key][1] = newcb1;
    }

    bool Revert(mfxCallbacks cb)
    {
        if (m_cb.count(cb))
        {
            m_storage[0][cb][0] = m_cb[cb].first;
            m_storage[0][cb][1] = m_cb[cb].second;
            return true;
        }
        return false;
    }

    void RevertAll()
    {
        while (!m_cb.empty())
        {
            Revert(m_cb.begin()->first);
            m_cb.erase(m_cb.begin()->first);
        }
    }

private:
    T* m_storage;
    std::map<mfxCallbacks, std::pair<void*,void*> > m_cb;
};
#define INIT_CALLBACK_BACKUP(storage) CallbackBackup<void*[sizeof(storage)/sizeof(storage[0])][2]> callbacks(&storage);
#define SET_CALLBACK(baseType, base, name, obj)\
        if (base##name && base##name != baseType##_##name) {\
            callbacks.New(e##baseType##_##name##_tracer, base##name, obj);\
            base##name = baseType##_##name;\
        }

#endif //TRACE_CALLBACKS

#endif //FUNCTIONSTABLE_H_
