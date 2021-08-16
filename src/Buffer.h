#ifndef _BUFFER_H_INCLUDED_
#define _BUFFER_H_INCLUDED_

#include "Config.h"
#include "StringSlice.h"

class Connection;
class HttpRequest;
struct OFF_T{
    OFF_T(off_t c) : x(c) {}
    off_t x;
};

struct MSEC_T{
    MSEC_T(mSec c) : x(c) {}
    mSec x;
};

struct TIME_T{
    TIME_T(time_t c) : x(c) {}
    time_t x;
};

struct B_CRLF { };
class Pool;
class OpenFileInfo;
class File;
class Buffer {
public:
    static Buffer *create_temp_buffer(Connection *, size_t size);
    static Buffer *create_temp_buffer(HttpRequest *, size_t size);

    static Buffer *create_file_buffer(Pool *pool, const OpenFileInfo &ofi);

    bool in_memory() const { 
        return m_memory || m_temporary || m_mmap; 
    }

    bool in_memory_only() const { 
        return in_memory() && !m_in_file; 
    }

    bool special() const {
        return (m_flush || m_last_buf || m_sync) && !in_memory() && !m_in_file; }

    off_t size() {return m_in_file ? m_file_last - m_file_pos : m_last - m_pos; }

    Buffer &operator<<(const StringSlice &c);
    Buffer &operator<<(StringSlice &&c);


    Buffer &operator<<(OFF_T&& c);
    Buffer &operator<<(TIME_T&& c);
    Buffer &operator<<(MSEC_T&& c);
    Buffer &operator<<(B_CRLF);

    int fd() const;
    u_char                      *m_start;     
    u_char                      *m_end;
    u_char                      *m_last;
    u_char                      *m_pos;

    off_t                        m_file_last;
    off_t                        m_file_pos;

    unsigned                     m_temporary:1;
    unsigned                     m_memory:1;
    unsigned                     m_mmap:1;
    unsigned                     m_in_file:1;
    unsigned                     m_flush:1;
    unsigned                     m_last_buf:1;
    unsigned                     m_last_buf_in_chain:1;
    unsigned                     m_sync:1;
    File                        *m_file;

private:
    Buffer() {} 
};


#endif