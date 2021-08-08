#ifndef _BUFFER_H_INCLUDED_
#define _BUFFER_H_INCLUDED_

#include "Config.h"
#include "Connection.h"

class Pool;
class Buffer {
public:
    static Buffer *create_temp_buffer(Pool *pool, size_t size);

    bool in_memory() const { 
        return m_memory || m_temporary || m_mmap; 
    }

    bool in_memory_only() const { 
        return in_memory() && !m_in_file; 
    }

    bool special() const {
        return (m_flush || m_last_buf || m_sync) && !in_memory() && !m_in_file; }

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
private:
    Buffer() {} 
    ~Buffer() {}   
};

#endif