#include "Buffer.h"

#include "Connection.h"

Buffer *Buffer::create_temp_buffer(Connection *c, size_t size)
{
    void *ans = c->pool_calloc(1, sizeof(Buffer));
    
    if (ans == nullptr) {
        return nullptr;
    }

    void *start = c->pool_malloc(size);
    if (start == nullptr)
    {
        free(ans);
        return nullptr;
    }

    Buffer *buf = static_cast<Buffer*>(ans);
    
    buf->m_start = static_cast<u_char*>(start);
    buf->m_end = buf->m_start + size;
    buf->m_pos = buf->m_start;
    buf->m_last = buf->m_start;

    buf->m_temporary = true;
    return buf;
}