#ifndef _HTTP_CONNECTION_H_INCLUDED_
#define _HTTP_CONNECTION_H_INCLUDED_

#include "Config.h"


class Buffer;
class BufferChain;

class HttpConnection {
public:
    HttpConnection();
    ~HttpConnection();

    // Buffer *get_busy_buffer() const;
    // size_t  get_busy_buffer_num() const { return m_busy_buffers_num; }
    // Buffer *get_or_create_busy_buffer();
    // void    move_all_to_free_buffers_but_this(Buffer*);
    // void move_others_to_free_chain(Buffer *keep);
    
    size_t                              m_busy_buffers_num;
    BufferChain                        *m_busy_buffers;
    BufferChain                        *m_free_buffers;
};


#endif