#ifndef _HTTP_REQUEST_H_INCLUDED_
#define _HTTP_REQUEST_H_INCLUDED_

#include "Config.h"

#include "HttpHeadersIn.h"
#include "HttpHeadersOut.h"


class Buffer;
class Connection;
class HttpConnection;


class HttpRequest {
public:
    ssize_t     read_request_header();
    Int         parse_request_line();
    Int         process_request_uri();


    bool need_alloc_large_header_buffer() const;
    
    /* r->m_request_start; */
    Int alloc_large_header_buffer(bool request_line);


    enum class Method{
        UNKNOWN,
        GET,
        HEAD,
        POST
    };
    HttpConnection              *m_http_connection;
    Connection                  *m_connection;
    Buffer                      *m_header_in_buffer;
    
    HttpRequest                 *m_main_request;
    
    time_t                       m_start_sec;
    mSec                         m_start_msec;
    Method                       m_method;
    uInt                         m_http_version;

    HttpHeadersIn                m_header_in;
    HttpHeadersOut               m_header_out;

    u_char                      *m_request_start;
    u_char                      *m_request_end;
    u_char                      *m_method_end;
    u_char                      *m_uri_start;
    u_char                      *m_uri_end;
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
    unsigned                     m_blocked:8;
    
    template <typename T> void set_read_event_handler(T f);
    void run_read_event_handler();

    void block_reading_handler();
private:
    std::function<void()>       m_read_event_handler;

    uInt                        m_parse_state;
};

#endif