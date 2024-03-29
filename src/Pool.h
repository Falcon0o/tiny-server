#ifndef _POOL_H_INCLUDED_
#define _POOL_H_INCLUDED_
#include "Config.h"

class BufferChain;

class Pool{
public:
    Pool();
    ~Pool();
    using Deleter = void(*)(void *);
    void *malloc(size_t s, Deleter deleter = [](void *addr){ ::free(addr); });
    BufferChain *alloc_buffer_chain();
    void free_buffer_chain(BufferChain*);


    void *calloc(size_t n, size_t size, Deleter deleter = [](void *addr){ ::free(addr); });
    
    void insert(void *addr, Deleter deleter);
    void free(void *addr);
    void destroy_pool();
private:
    std::map<void*, Deleter> m_pool;
    BufferChain *m_chain;
};
#endif
