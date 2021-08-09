#include "Pool.h"

Pool::Pool()
{

}
Pool::~Pool()
{
    destroy_pool();
}

void *Pool::malloc(size_t s, Deleter deleter)
{
    void *ans = ::malloc(s);
    if (ans) {
        m_pool.insert({ans, deleter});
    }
    return ans;
}

void *Pool::calloc(size_t n, size_t size, Deleter deleter)
{
    void *ans = ::calloc(n, size);
    if (ans) {
        m_pool.insert({ans, deleter});
    }
    return ans;
}

void Pool::free(void *addr)
{
    auto iter = m_pool.find(addr);
    if (iter != m_pool.end()) 
    {
        Deleter deleter = iter->second;
        deleter(addr);
        m_pool.erase(iter);
    }
}

void Pool::destroy_pool()
{
    for (auto iter = m_pool.begin(); iter != m_pool.end(); ++iter) {
        void *addr = iter->first;
        Deleter deleter = iter->second;
        deleter(addr);
    }
    m_pool.clear();
}

void Pool::insert(void *addr, Deleter deleter)
{
    m_pool.insert({addr, deleter});
}