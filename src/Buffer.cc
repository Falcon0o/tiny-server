#include "Buffer.h"

#include "Connection.h"
#include "File.h"
#include "Timer.h"
#include "Pool.h"
#include "OpenFileInfo.h"

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

Buffer *Buffer::create_file_buffer(Pool *pool, const OpenFileInfo &ofi)
{
    Buffer *buf = (Buffer*)pool->calloc(1, sizeof(Buffer));
    
    if (buf == nullptr) {
        return nullptr;
    }

    File *file = (File*)pool->calloc(1, sizeof(File));
    if (buf == nullptr) {
        return nullptr;
    }
    
    buf->m_file_pos = 0;
    buf->m_file_last = ofi.m_size;

    buf->m_in_file = buf->m_file_last ? true : false;
    
    buf->m_file = file;

    file->m_fd = ofi.m_fd;
    file->m_directio = ofi.m_is_directio;
    return buf;
}
int Buffer::fd() const
{
    return m_file->m_fd;
}

Buffer &Buffer::operator<<(const StringSlice &c)
{
    memcpy(m_last, c.m_data, c.m_len);
    m_last += c.m_len;
    return *this;
}

Buffer &Buffer::operator<<(StringSlice &&c)
{
    memcpy(m_last, c.m_data, c.m_len);
    m_last += c.m_len;
    return *this;
}

Buffer &Buffer::operator<<(OFF_T&& c)
{
    m_last += sprintf((char*)m_last, "%ld", c.x);
    return *this;
}


Buffer &Buffer::operator<<(TIME_T&& c)
{
    m_last += Timer::get_http_time(c.x, m_last);
    return *this;
}

Buffer &Buffer::operator<<(MSEC_T&& c)
{
    m_last += sprintf((char*)m_last, "%ld", c.x);
    return *this;
}

Buffer &Buffer::operator<<(B_CRLF)
{
    m_last[0] = CR;
    m_last[1] = LF;
    m_last += 2;
    return *this;
}
