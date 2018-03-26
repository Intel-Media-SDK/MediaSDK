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

#include "stdafx.h"
#include "msdk_ts.h"
#include <vector>
#include <sstream>

using namespace msdk_ts;

struct par_facet : std::ctype<char> {
    par_facet() : std::ctype<char>(create_table()) {};
    static std::ctype_base::mask const* create_table() {
        static std::vector<std::ctype_base::mask> t(table_size, std::ctype_base::mask());
        t[' '] = std::ctype_base::space;
        t['+'] = std::ctype_base::space;
        t['\n'] = std::ctype_base::space;
        return &t[0];
    };
};

ts::ts(std::istream& in) :
m_level(0),
m_in(in.rdbuf())
{
    #define msdk_ts_DECLARE_BLOCK(block) m_block[key(#block)] = &ts::block;
    #include "msdk_ts_blocks.h"
    msdk_ts_DECLARE_BLOCK(loop);
    #undef  msdk_ts_DECLARE_BLOCK

    // declare "constants":
    var_def<_break_cond>("cf_less", &ts::less);
    var_def<_break_cond>("cf_cnt", &ts::cnt);

    m_in.imbue(std::locale(std::locale(), new par_facet));
};

ts::~ts(){
    while(!m_mem.empty()){
        delete m_mem.begin()->second;
        m_mem.erase(m_mem.begin());
    }
};

void* ts::alloc(size_t size){
    AbstractType* t = new ArrHolder<unsigned char>(size);
    if(!(t && t->get())) throw resALCERR;
    m_mem[t->get()] = t;
    //std::cout << " (allocated: 0x" << t->get() << " - 0x" << (void*)((char*)t->get() + t->size()) << ") ";
    return t->get();
}

void ts::alloc(void*& dst, size_t size){
    if(    !m_mem.count(dst)
        || m_mem[dst]->size() < size){
        dst = alloc(size);
    }
};

char* ts::copy_str(const char* str){
    char* s;
    size_t l = strlen(str)+1;
    alloc((void*&)s, l);
    memcpy(s, str, l);
    return s;
};

#pragma warning(disable:4996)
bool is_correct_indent(std::istream& in, unsigned int level, char* str){
    std::istream::pos_type pos;

    pos = in.tellg();
    in.get(str, msdk_ts_INDENT_SIZE * level + 1, '\n');
    if(strtok(str, " ") || in.peek() == ' '){
        in.seekg(pos);
        return false;
    }
    return true;
}
#pragma warning(default:4996)

void ts::load_param(std::istream& in, unsigned int level){
    char str[256];
    char *name, *op, *val;

    while(!in.eof()){
        unsigned int offset = 0;

        if(!is_correct_indent(in, level, str))
            break;

        in >> (name = str);
        if(name[0] == '#'){
            std::cout << "    " << name;
            in.getline(str, 255);
            std::cout << str << "\n";
            continue; //skip comment
        }
        if(in.peek() == '+'){
            in >> offset;
        }
        if(in.peek() == '\n') throw resINVPAR;

        in >> (op = str + strlen(name) + 1);
        if(in.peek() == '\n' || op[1] != '=') throw resINVPAR;

        std::cout << "    " << name; 
        if(offset) std::cout << '+' << offset;
        std::cout << ' ' << op << ' ';

        switch(op[0]){
            case 'p':
            case '4': 
            case '2': 
            case '1': 
                if(!offset)alloc(m_var[key(name)], (op[0] == 'p') ? sizeof(void*) : op[0] - '0');
                switch(op[0]){
                    case 'p': in >> var_old<void*>(name, offset); std::cout << var_old<void*>(name, offset); break;
                    case '4': in >> var_old<long   >(name, offset); std::cout << var_old<long   >(name, offset); break;
                    case '2': in >> var_old<short  >(name, offset); std::cout << var_old<short  >(name, offset); break;
                    case '1': { short tmp; in >> tmp; std::cout << tmp; var_old<char   >(name, offset) = (char)tmp; break;}
                    default: throw resINVPAR;
                };
                break;
            case 's':
                in >> (val = op + strlen(op));
                std::cout << (var_old_or_new<char*>(name) = copy_str(val));
                break;
            case 'r':
                in >> (val = op + strlen(op));
                std::cout << val;
                if(is_defined(val)){
                    if(offset) var_old<void*>(name, offset) = m_var[val];
                    else m_var[key(name)] = &m_var[val];
                    break;
                } throw resINVPAR;
            case 'v': 
                in >> (val = op + strlen(op));
                std::cout << val;
                if(is_defined(val)){
                    m_var[key(name)] = m_var[val];
                    if(in.peek() == '+'){
                        in >> offset;
                        m_var[name] = (char*)m_var[name] + offset;
                        std::cout << "+" << offset;
                    }
                    break;
                } throw resINVPAR;
            case 'b':{
                    unsigned long size = 0;
                    in >> size; std::cout << size;
                    m_var[key(name)] = alloc(size);
                    memset(m_var[name], 0, size);
                } break;
            default: throw resINVPAR;
        }
        in.getline(str, 255);
        std::cout << str << std::endl;
    }
};

test_result ts::run(){
    msdk_ts::test_result res = msdk_ts::resOK;
    char str[256];

    while(!m_in.eof()){
        m_in.getline(str, 255); 
        if(!is_block_name(str)) continue;
        res = run_block(str, m_in);
        if(res) break;
    }
    if(!res && var_def<bool>("sb_failed",false))
        res = resFAIL;
    switch(res){
        case resFAIL:
            std::cout << "FAIL REASON: verification failed\n";
            break;
        case resOK:
            std::cout << "PASSED\n";
            break;
        case resUNKBLK:
            std::cout << "FAIL REASON: unknown block name\n";
            break;
        case resINVPAR:
            std::cout << "FAIL REASON: invalid parameter\n";
            break;
        case resALCERR:
            std::cout << "FAIL REASON: memory allocation error\n";
            break;
        case loopCONTINUE:
        case loopBREAK:
            std::cout << "FAIL REASON: invalid pipeline - loop operation block used out of the loop\n";
            break;
        case resEXCPT:
            std::cout << "FAIL REASON: exception" << std::endl;
            break;
    }
    return res;
};

test_result ts::run_block(const char* name, std::istream& in) { 
    block_map_type::const_iterator it = m_block.find(key(name));
    test_result res = resUNKBLK;
    _break_cond& b = var_def<_break_cond>("break_cond", &ts::repeat_0);

    std::cout << "BLOCK: " << name << "\n";

    if(it != m_block.end()){ 
        b = &ts::repeat_0;
        try {
            load_param(in, ++m_level);
            do{
                res = (this->*it->second)();
                if(res == resFAIL && var_def<bool>("sb_delayed_fail", false)){
                    var_old_or_new<bool>("sb_failed") = true;
                    std::cout << "--FAILED-- " << "\n";
                    res = resOK;
                }else{
                    if(res) break;
                    std::cout << "--OK-- " << "\n";
                }
            }while(!(this->*b)());
            m_level--;
        } catch(test_result e){
            res = e;
        }
    }
    return res;
};

bool ts::less(){
    switch(var_def<unsigned char>("less_size", 4)){
        case 1: return (var_def<char >("less_x", 0) < var_def<char >("less_y", 1));
        case 2: return (var_def<short>("less_x", 0) < var_def<short>("less_y", 1));
        case 4: return (var_def<int  >("less_x", 0) < var_def<int  >("less_y", 1));
        default: return false;
    }
}

bool ts::cnt(){
    return !--var_def<unsigned char>("cnt_x", 1);
}

class loop_report{
public:
    loop_report() : cnt(0), stg(0) { std::cout << "LOOP_START\n"; };
    ~loop_report(){ std::cout << "loop stage = " << cnt << '.' << stg << "\nLOOP_END\n"; };
    void set_stage(unsigned int stage, std::string name){m_loop_blocks[stage] = name;};
    void report_stage(){ std::cout << "  LOOP_BLOCK " << cnt << '.' << stg << ": "<< m_loop_blocks[stg] << "\n";};

    unsigned int cnt;
    unsigned int stg;
private:
    std::map<unsigned int, std::string> m_loop_blocks;
};
class loop_trace{
public:
    loop_trace(bool trace_on) : m_is_ok(false){
        m_sbuf = NULL;
        if(!trace_on){
            m_sbuf = std::cout.rdbuf();
            std::cout.rdbuf(m_tmp_stream.rdbuf());
        }
    };
    ~loop_trace(){
        if(m_sbuf){
            std::cout.rdbuf(m_sbuf);
            if(!m_is_ok)
                std::cout << m_tmp_stream.str();
        }
    };
    void ok() { m_is_ok = true; };
private:
    std::streambuf*     m_sbuf;
    std::ostringstream  m_tmp_stream;
    bool                m_is_ok;
};

msdk_ts_BLOCK(loop){
    typedef std::pair<std::string, block> loop_block;
    test_result res = resOK;
    char str[256];
    std::vector<loop_block> run_queue;
    _break_cond& b = var_def<_break_cond>("loop_break", &ts::repeat_0);
    loop_report report;

    m_level++;
    while(!m_in.eof()){
        if(!is_correct_indent(m_in, m_level, str))
            break;
        m_in.getline(str, 255);
        if(!is_block_name(str)) return resUNKBLK;
        std::cout << "  BLOCK " << report.stg << ": " << str << "\n";
        report.set_stage(report.stg++, str);

        //load_param(m_in, m_level+1);
        {
            block blk = m_block[key(str)];
            std::string par = "";
            while(!m_in.eof() && is_correct_indent(m_in, m_level+1, str)){
                m_in.getline(str, 255);
                if(par != "")
                    par += '\n';
                par += msdk_ts_INDENT;
                par += str;
                std::cout << msdk_ts_INDENT << str << "\n";
            }
            run_queue.push_back(loop_block(par, blk));
        }
    }
    m_level--;

    if(var_def<bool>("trace", false))
        var_old_or_new<bool>("trace_loop") = true;

    while(!(this->*b)()){
        report.stg = 0;
        for(std::vector<loop_block>::iterator it = run_queue.begin(); it != run_queue.end(); it++){
            loop_trace trace(var_def<bool>("trace_loop", false));
            report.report_stage();

            if(it->first.size()){
                std::istringstream ss(it->first);
                ss.imbue(m_in.getloc());
                load_param(ss, 1);
            }
            res = (this->*it->second)();

            if(res == loopCONTINUE){ 
                trace.ok();
                break;
            }
            if(res == loopBREAK){ 
                trace.ok();
                b = &ts::repeat_0;
                break;
            }
            if( res ) return res;

            trace.ok();
            report.stg++;
        }
        report.cnt++;
    }
    
    return res;
}
