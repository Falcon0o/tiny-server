#ifndef _HTTP_REQUEST_H_INCLUDED_
#define _HTTP_REQUEST_H_INCLUDED_

#include "Config.h"

#include "HttpHeadersIn.h"
#include "HttpHeadersOut.h"
#include "HttpPhaseEngine.h"

class Pool;

class Buffer;
class BufferChain;
class Connection;
class HttpConnection;


class HttpRequest {
public:
    HttpRequest(Pool*);
    ~HttpRequest();
    ssize_t     read_request_header();

#define     PARSE_HEADER_DONE           1
#define     PARSE_INVALID_METHOD        10
#define     PARSE_INVALID_REQUEST       11
#define     PARSE_INVALID_VERSION       12
#define     PARSE_INVALID_HEADER        14

#define     HTTP_METHOD_UNKNOWN      0x0001
#define     HTTP_METHOD_GET          0x0002
#define     HTTP_METHOD_HEAD         0x0004
#define     HTTP_METHOD_POST         0x0008

    Int         parse_request_line();
    Int         parse_method();
    Int         process_request_uri();
    Int         parse_header_line();
    Int         process_request_header();
    void        process_request();
    StringSlice map_uri_to_path();
    bool        need_alloc_large_header_buffer() const;
    
    /* r->m_request_start; */
    Int alloc_large_header_buffer(bool request_line);

    Int post_http_request();// ngx_http_post_request
    Int discard_request_body();
    Int                          m_method;

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
    HttpPhaseEngine              m_phase_engine;

    u_char                      *m_request_start;
    u_char                      *m_request_end;
    u_char                      *m_method_end;
    u_char                      *m_uri_start;
    u_char                      *m_uri_end;
    u_char                      *m_uri_ext_start;
    u_char                      *m_uri_ext_end;
    u_char                      *m_http_protocol_start;

    u_char                      *m_header_name_start;
    u_char                      *m_header_name_end;
    u_char                      *m_header_start;
    u_char                      *m_header_end;
    size_t                       m_header_hash;
    
    u_char                       m_lowcase_header[HTTP_LOWCAST_HEADER_LEN];
    uInt                         m_lowcase_index;

    off_t                        m_request_length;

    StringSlice                  m_request_line;
    StringSlice                  m_method_name;
    StringSlice                  m_http_protocol;
    StringSlice                  m_uri;

    /* 如果有?等参数 */
    StringSlice                  m_uri_ext;

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
    unsigned                     m_invalid_header:1;
    unsigned                     m_allow_ranges:1;
    unsigned                     m_header_sent:1;
    unsigned                     m_header_only:1;
    unsigned                     m_discard_body:1;
    unsigned                     m_aio:1;
    unsigned                     m_buffered:4;
    
    unsigned                     m_blocked:8;
    
    unsigned                     m_posted:1; /* 自己加的 */
    size_t                       m_response_header_size;

    Int response_header_filter();
    Int response_body_filter(BufferChain *);

    typedef void(HttpRequest::*Handler)();

    void set_read_event_handler(Handler h) {
        m_read_event_handler = h;
    }

    void set_write_event_handler(Handler h) {
        m_write_event_handler = h;
    }

    void run_read_handler() { 
        (this->*m_read_event_handler)();
    }
    void run_write_handler() { 
        (this->*m_write_event_handler)(); 
    }

    Int set_write_handler();

    void block_reading_handler();
    void run_phases_handler();
    void http_terminate_handler();
    void empty_handler() {}
    void http_request_finalizer();
    void http_handler();
    void discarded_request_body_handler();
    void ngx_http_test_reading();
//  phases handler
    Int http_static_handler();
    void http_test_reading();
    void http_writer();

    Handler                              m_read_event_handler;
    Handler                              m_write_event_handler;

    uInt                        m_parse_state;

    // TODO: 暂未使用的变量
    void                        *m_content_handler;
    BufferChain                 *m_out_buffer_chain;

    using Deleter = void(*)(void *);

    void *pool_malloc(size_t s, Deleter deleter = [](void *addr){ ::free(addr); });
    void *pool_calloc(size_t n, size_t size, Deleter deleter = [](void *addr){ ::free(addr); });
    void pool_free(void *addr);
private:

    Pool                                *m_pool;
};


#endif