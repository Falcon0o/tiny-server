#ifndef _CONNECTION_H_INCLUDED_
#define _CONNECTION_H_INCLUDED_

#include "Config.h"
#include "Pool.h"
class Buffer;
class Event;
class Listening;
class HttpRequest;

class Connection {
public:
    void create_http_connection();
    void finalize_http_connection(HttpRequest *r);  // ngx_http_finalize_connection
    
    void close_accepted_connection();
    void close_connection();

    void close_http_connection();
    void set_http_keepalive(HttpRequest *r);
    void set_http_lingering_close();

    HttpRequest *create_http_request();
    void close_http_request(HttpRequest *r, Int rc); // ngx_http_close_request
    void free_http_request(HttpRequest *r, Int rc); // ngx_http_free_request
    void finalize_http_request(HttpRequest *r, Int rc);  // ngx_http_finalize_request
    void terminate_http_request(HttpRequest *r, Int rc); // ngx_http_terminate_request

    /*返回值为 AGAIN 时，buffer 中未读取任何数据 */
    ssize_t recv_to_buffer(Buffer *b);

    void                                *m_data;
    Event                               *m_read_event;
    Event                               *m_write_event;
    int                                  m_fd;
    
    Listening                           *m_listening;
    int                                  m_type;

    unsigned                             m_buffered:8;
    unsigned                             m_close:1;    
    unsigned                             m_reusable:1;
    std::list<Connection*>::iterator     m_reusable_iter;
    unsigned                             m_shared:1;
    unsigned                             m_destroyed:1;
    unsigned                             m_timedout:1;
    unsigned                             m_error:1;

    sockaddr                            *m_sockaddr;
    mSec                                 m_start_time;

    Buffer                              *m_small_buffer;
    off_t                                m_sent;
    uInt                                 m_request_cnt;
    Pool                                 m_pool;
};
#endif