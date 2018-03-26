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

#include <iostream>
#include <map>
#if (defined(LINUX32) || defined(LINUX64))
#include <string.h>
#include <dlfcn.h>
#endif
#include <cassert>

namespace msdk_ts {

typedef enum _test_result{
    resFAIL  = -100,
    loopCONTINUE,
    loopBREAK,
    resEXCPT = -1,
    resOK    =  0,
    resUNKBLK,
    resINVPAR,
    resALCERR,
} test_result;

#define msdk_ts_INDENT "    "
#define msdk_ts_INDENT_SIZE 4
#define msdk_ts_BLOCK(name)    msdk_ts::test_result msdk_ts::ts::name()

class str_less{public: bool operator () (std::string s1, std::string s2) const {return s1.compare(s2) < 0;};};

class AbstractType{ 
public: 
    virtual ~AbstractType() {}
    virtual size_t  size () = 0;
    virtual bool    valid() = 0;
    virtual void*   get  () = 0;
};

template<typename T> class VarHolder : public AbstractType{ 
public:
    VarHolder() : sz(sizeof(T)), var(new T()) {}
    explicit VarHolder(T * ptr) : sz(sizeof(T)), var(ptr) { assert(ptr); }
    ~VarHolder() { delete var; }
    size_t  size () { return sz; }
    void*   get  () { return var; }
    bool    valid() { return !!var; }
private:
    size_t sz;
    T * var;
};

template<typename T> class ArrHolder : public AbstractType{ 
public:
    explicit ArrHolder(size_t size) : sz(size) { arr = new T[size]; }
    ~ArrHolder() { if(arr) delete[] arr; }
    size_t  size () { return sz; }
    void*   get  () { return arr; }
    bool    valid() { return !!arr; }
private:
    size_t sz;
    T* arr;
};

class ts{
public:
    ts(std::istream& in);
    ~ts();

    test_result run();

    inline bool is_block_name(const char* name) { return !!m_block.count(key(name)); }

    //dll only
    test_result run_block(const char* name);
    void* get_var(const char* key) { return is_defined(key) ? m_var[key] : NULL; };
    void* new_var(const char* key, unsigned long size) { m_var[this->key(key)] = alloc(size); memset(m_var[key], 0, size); return m_var[key];};
    void* set_var(const char* key, void* val) { return m_var[this->key(key)] = val; };

private:
#define msdk_ts_DECLARE_BLOCK(name) test_result name()
#include "msdk_ts_blocks.h"
    msdk_ts_DECLARE_BLOCK(loop);
#undef msdk_ts_DECLARE_BLOCK

    typedef test_result (ts::*block)();
    typedef bool (ts::*_break_cond)();
    typedef std::map<std::string, block, str_less> block_map_type;

    block_map_type m_block;
    std::map<std::string, void*, str_less> m_var;
    std::map<void*, AbstractType*> m_mem;
    bool m_tmp_flag;
    unsigned int m_level; //current arg indent level
    std::istream m_in;

    void    load_param  (std::istream& in, unsigned int level);
    void    alloc       (void*& dst, size_t size);
    void*   alloc       (size_t size);
    
    template <class T> T* alloc_array(size_t size){
        AbstractType* t = new ArrHolder<T>(size);
        if(!t) throw resALCERR;
        m_mem[t->get()] = t;
        return (T*)t->get();
    }

    template <class T> T* alloc(const char* _key = NULL){
        AbstractType* t = new VarHolder<T>;
        if(!t) throw resALCERR;
        m_mem[t->get()] = t;
        if(_key)  m_var[key(_key)] = t->get();
        return (T*)t->get();
    }

    template <class T> T* alloc(const char* _key, T* constructed){
        AbstractType* t = new VarHolder<T>(constructed);
        if(!t) throw resALCERR;
        m_mem[t->get()] = t;
        if(_key)  m_var[key(_key)] = t->get();
        return (T*)t->get();
    }

    char*   copy_str    (const char* str);
    inline std::string key(const char* str){ return str; };
    bool    repeat_0() { return true; };
    bool    repeat_inf() { return false; };
    bool    less();
    bool    cnt();
    bool    is_defined(std::string key) {return !!m_var.count(key);};

    template<class T> T& var_old(const char* name, unsigned int offset = 0){
        if(!is_defined(name)) throw resINVPAR;
        return *(T*)((char*)m_var[name]+offset);
    }

    template<class T> T& var_new(const char* name) { return *alloc<T>(name); }
    template<class T> T& var_new(const char* name, T* constructed) { return *alloc<T>(name, constructed); }

    template<class T> T& var_old_or_new(const char* name) {
        if(is_defined(name)) return *(T*)m_var[name];
        return *alloc<T>(name);
    };
    template<class T> T& var_def(const char* name, T default_value){
        bool old = is_defined(name);
        T& var = var_old_or_new<T>(name);
        if(!old) var = default_value;
        return var;
    };

    test_result run_block(const char* name, std::istream& in);
};


};
