#ifndef _CONNECTION_H_INCLUDED_
#define _CONNECTION_H_INCLUDED_

#include "Config.h"
#include "Pool.h"
class Buffer;
class Event;
class Listening;
class HttpRequest;
class HttpConnection;

class Connection {
public:
    void create_http_connection();
    void finalize_http_connection();  // ngx_http_finalize_connection
    void close_http_connection();
    void close_accepted_connection();

    HttpRequest *create_http_request();
    void finalize_http_request(HttpRequest *r, Int rc);  // ngx_http_finalize_request
    void terminate_http_request(Int rc); // ngx_http_terminate_request
    void close_http_request(Int rc); // ngx_http_close_request
    void free_http_request(Int rc); // ngx_http_free_request

    void close_connection();
    void set_http_keepalive(); // ngx_http_set_keepalive
    void set_http_lingering_close(); // ngx_http_set_lingering_close

    void run_posted_http_requests();// ngx_http_run_posted_requests
    
    

    /*返回值为 AGAIN 时，buffer 中未读取任何数据 */
    ssize_t recv_to_buffer(Buffer *b);

    union {
        HttpConnection  *hc;
        Connection      *c;
        HttpRequest     *r;
        void            *v;
    }                                    m_data;
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
    unsigned                             m_tcp_nopush:2;

    unsigned                             m_tcp_nodelay:2;
    unsigned                             m_idle:1;

    sockaddr                            *m_sockaddr;
    mSec                                 m_start_time;

    Buffer                              *m_small_buffer;
    off_t                                m_sent;
    uInt                                 m_request_cnt;
    Pool                                 m_pool;
};
#endif