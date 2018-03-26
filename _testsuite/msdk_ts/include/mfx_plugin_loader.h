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

#pragma once
#include <sstream>
#include "test_common.h"
#include "vm_shared_object.h"
#include "mfxplugin++.h"


#define CREATE_PLUGIN_FNC "CreatePlugin"

typedef std::basic_string<vm_char>tstring;
typedef std::basic_stringstream<vm_char> tstringstream;
typedef mfxStatus (MFX_CDECL *CreatePluginPtr_t)(mfxPluginUID uid, mfxPlugin* plugin);

class MsdkSoModule
{
protected:
    vm_so_handle m_module;
public:
    MsdkSoModule(const tstring & pluginName)
        : m_module()
    {
        m_module = vm_so_load(pluginName.c_str());
        if (NULL == m_module)
        {
            int lastErr = 0;
            std::cout << "Failed to load shared module." << std::endl;
            #ifdef WIN32
                lastErr = GetLastError();
            #endif
            std::cout << "Error = " << lastErr << std::endl;
        }
    }
    template <class T>
    T GetAddr(const std::string & fncName)
    {
        T pCreateFunc = reinterpret_cast<T>(vm_so_get_addr(m_module, fncName.c_str()));
        if (NULL == pCreateFunc) {
            std::cout << "Failed to get function addres." << std::endl;
        }
        return pCreateFunc;
    }
    bool IsLoaded() {
        return m_module != 0;
    }

    virtual ~MsdkSoModule()
    {
        if (m_module)
        {
            vm_so_free(m_module);
            m_module = NULL;
        }
    }
};

struct PluginModuleTemplate {
    typedef MFXDecoderPlugin* (*fncCreateDecoderPlugin)();
    typedef MFXEncoderPlugin* (*fncCreateEncoderPlugin)();
    typedef MFXGenericPlugin* (*fncCreateGenericPlugin)();
    typedef MFXVPPPlugin*     (*fncCreateVPPPlugin)();

    fncCreateDecoderPlugin CreateDecoderPlugin;
    fncCreateEncoderPlugin CreateEncoderPlugin;
    fncCreateGenericPlugin CreateGenericPlugin;
    fncCreateVPPPlugin     CreateVPPPlugin;
};

template<class T>
struct PluginCreateTrait
{
};

template<>
struct PluginCreateTrait<MFXDecoderPlugin>
{
    static const char* Name(){
        return "mfxCreateDecoderPlugin";
    }
    enum PluginType {
        type=MFX_PLUGINTYPE_VIDEO_DECODE
    };
    typedef PluginModuleTemplate::fncCreateDecoderPlugin TCreator;

};

template<>
struct PluginCreateTrait<MFXEncoderPlugin>
{
    static const char* Name(){
        return "mfxCreateEncoderPlugin";
    }
    enum PluginType {
        type=MFX_PLUGINTYPE_VIDEO_ENCODE
    };
    typedef PluginModuleTemplate::fncCreateEncoderPlugin TCreator;
};

template<>
struct PluginCreateTrait<MFXVPPPlugin>
{
    static const char* Name(){
        return "mfxCreateVPPPlugin";
    }
    enum PluginType {
        type=MFX_PLUGINTYPE_VIDEO_VPP
    };
    typedef PluginModuleTemplate::fncCreateVPPPlugin TCreator;
};

template<>
struct PluginCreateTrait<MFXGenericPlugin>
{
    static const char* Name(){
        return "mfxCreateGenericPlugin";
    }
    enum PluginType {
        type=MFX_PLUGINTYPE_VIDEO_GENERAL
    };
    typedef PluginModuleTemplate::fncCreateGenericPlugin TCreator;
};

template <class T>
class PluginLoader 
{
protected:
    MsdkSoModule m_PluginModule;
    typedef PluginCreateTrait<T> Plugin;

public:
    mfxPlugin* plugin;
    
    PluginLoader(const tstring & pluginName, mfxPluginUID uid)
        : m_PluginModule(pluginName)
    {
        plugin = new mfxPlugin();
       
        if (!m_PluginModule.IsLoaded())
            return;
        CreatePluginPtr_t pCreateFunc = m_PluginModule.GetAddr<CreatePluginPtr_t >(CREATE_PLUGIN_FNC);
        if(NULL == pCreateFunc)
            return;
        pCreateFunc(uid, plugin);
        
              
    }

    PluginLoader(const tstring & pluginName)
        : m_PluginModule(pluginName)
    {
        std::auto_ptr<T> m_plugin;
        MFXPluginAdapter<T> m_adapter;
               
        plugin = 0;
        if (!m_PluginModule.IsLoaded())
            return;
        typename Plugin::TCreator pCreateFunc = m_PluginModule.GetAddr<typename Plugin::TCreator >(Plugin::Name());
        if(NULL == pCreateFunc)
            return;

        m_plugin.reset(pCreateFunc());
        if (NULL == m_plugin.get())
        {
            std::cout << "Failed to Create plugin." << std::endl;
            return;
        }
        m_adapter = make_mfx_plugin_adapter(m_plugin.get());
        plugin = (mfxPlugin*) &m_adapter;
    }

    bool Create(mfxPluginUID uid)
    {        
        if (!m_PluginModule.IsLoaded())
            return false;
        CreatePluginPtr_t pCreateFunc = m_PluginModule.GetAddr<CreatePluginPtr_t >(CREATE_PLUGIN_FNC);
        if(NULL == pCreateFunc)
            return false;
        plugin = new mfxPlugin();
        if (pCreateFunc(uid, plugin) != MFX_ERR_NONE)
            return false;
        return true;
    }

    ~PluginLoader()
    {
        //loaded module are freed in MsdkSoModule destructor
    }
};
