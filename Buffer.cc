#include "Buffer.h"

#include "Connection.h"
#include "File.h"

#include "Pool.h"

Buffer *Buffer::create_temp_buffer(Pool *pool, size_t size)
{
    void *ans = pool->calloc(1, sizeof(Buffer));
    
    if (ans == nullptr) {
        return nullptr;
    }

    void *start = pool->malloc(size);
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

int Buffer::fd() const
{
    return m_file->m_fd;
}