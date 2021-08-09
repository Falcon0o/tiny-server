#ifndef _BUFFER_CHAIN_H_INCLUDED_
#define _BUFFER_CHAIN_H_INCLUDED_

#include "Config.h"
class Buffer;

class BufferChain {
public:
    // BufferChain(Buffer *b) : m_buffer(b), m_next(nullptr) {}
    // ~BufferChain() {}
    

    BufferChain *coalesce_file(off_t limit, off_t &total);
    BufferChain *update_sent(size_t sent);
    Buffer          *m_buffer;
    BufferChain     *m_next;
};

#endif