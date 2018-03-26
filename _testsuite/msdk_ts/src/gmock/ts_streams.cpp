#include "ts_streams.h"
#include "ts_common.h"
#include <algorithm>

#if defined(LINUX32) || defined(LINUX64)
const char tsStreamPool::valid_delimiter;
const char tsStreamPool::invalid_delimiter;
#endif

tsStreamPool::tsStreamPool() 
    : m_query_mode(false) 
    , m_reg(false)
{
    m_dir = ENV("MEDIASDK_STREAMS", "") + valid_delimiter;
}

tsStreamPool::~tsStreamPool() 
{
}

void tsStreamPool::Init(bool query_mode)
{ 
    m_query_mode = query_mode; 
    m_reg        = false;
    m_pool.clear();
}

void tsStreamPool::Close()
{
    if(!m_reg && m_pool.size())
    {
        ADD_FAILURE() << "Stream pool was used but not registered\n";
    }
}

const char* tsStreamPool::Get(std::string name, std::string path)
{
    if(!m_pool.count(name))
    {
        if(m_reg)
        {
            ADD_FAILURE() << "failed to add stream: stream pool was already registered\n";
            throw tsFAIL;
        }
        m_pool[name] = m_dir + path + name;
        std::replace(m_pool[name].begin(), m_pool[name].end(), invalid_delimiter, valid_delimiter);
    }
    return m_pool[name].c_str();
}

void tsStreamPool::Reg()
{
    mfxU32 i = 0;
    m_reg = true;

    for(PoolType::iterator it = m_pool.begin(); it != m_pool.end(); it ++)
    {
        g_tsLog << "STREAM[" << i ++ << "]: " << it->second << "\n";
    }

    if(!m_query_mode)
    {
        return;
    }

    throw tsOK;
}