#ifndef _HTTP_REQUEST_H_INCLUDED_
#define _HTTP_REQUEST_H_INCLUDED_

#include "Config.h"

#include "HttpHeadersIn.h"
#include "HttpHeadersOut.h"

#include "Pool.h"

class Buffer;
class Connection;
class HttpConnection;


class HttpRequest {
public:
    ssize_t     read_request_header();

#define     PARSE_INVALID_METHOD        10
#define     PARSE_INVALID_REQUEST       11
#define     PARSE_INVALID_VERSION       12

    Int         parse_request_line();
    Int         parse_method();
    Int         process_request_uri();


    bool need_alloc_large_header_buffer() const;
    
    /* r->m_request_start; */
    Int alloc_large_header_buffer(bool request_line);

    Int post_http_request();// ngx_http_post_request
    enum class Method{
        UNKNOWN = 0,
        GET,
        HEAD,
        POST}                    m_method;

    HttpConnection              *m_http_connection;
    Connection                  *m_connection;
    Buffer                      *m_header_in_buffer;
    
    HttpRequest                 *m_main_request;
    
    time_t                       m_start_sec;
    mSec                         m_start_msec;
    time_t                       m_lingering_time;

    uInt                         m_http_version;
    unsigned                     m_http_major:16;
    unsigned                     m_http_minor:16;
    
    HttpHeadersIn                m_header_in;
    HttpHeadersOut               m_header_out;

    u_char                      *m_request_start;
    u_char                      *m_request_end;
    u_char                      *m_method_end;
    u_char                      *m_uri_start;
    u_char                      *m_uri_end;
    u_char                      *m_uri_ext;
    u_char                      *m_http_protocol_start;

    u_char                      *m_header_name_start;
    u_char                      *m_header_name_end;
    u_char                      *m_header_start;
    u_char                      *m_header_end;
    
    off_t                        m_request_length;

    StringSlice                  m_request_line;
    StringSlice                  m_method_name;
    StringSlice                  m_http_protocol;

    /* 和子请求相关，无子请求计数应为 1 */ 
    size_t                       m_count:16;
    /* 和子请求相关，默认值为 0 */ 
    
    unsigned                     m_reading_body:1;
    unsigned                     m_post_action:1;
    unsigned                     m_request_complete:1;
    unsigned                     m_filter_finalize:1;
    unsigned                     m_pipeline:1;
    unsigned                     m_done:1;
    unsigned                     m_keepalive:1;
    unsigned                     m_lingering_close:1;

    unsigned                     m_space_in_uri:1;
    unsigned                     m_buffered:4;
    
    unsigned                     m_blocked:8;
    
    unsigned                     m_posted:1; /* 自己加的 */

    template <typename T> void set_read_event_handler(T f) {
        m_read_event_handler = std::bind(f, this);
    }
    template <typename T> void set_write_event_handler(T f) {
        m_write_event_handler = std::bind(f, this);
    }

    void run_read_event_handler() { 
        m_read_event_handler();
    }
    void run_write_event_handler() { 
        m_write_event_handler(); 
    }


    void block_reading_handler();
    void run_phases_handler();
    void http_terminate_handler();
    void empty_handler() {}
    void http_request_finalizer();
    Pool                                *m_pool;

    std::function<void()>       m_read_event_handler;
    std::function<void()>       m_write_event_handler;

    uInt                        m_parse_state;
};


#endif