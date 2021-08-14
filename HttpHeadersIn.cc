#include "HttpHeadersIn.h"

#include <string.h>

#include "HttpRequest.h"


static Int process_header_host(HttpRequest *, HttpHeadersIn::iterator);
static Int process_header_connection(HttpRequest *, HttpHeadersIn::iterator);




std::unordered_map<unsigned, HttpHeadersIn::http_header_handler> 
HttpHeadersIn::s_header_handlers = {
    {hash_string("host"), process_header_host},
    {hash_string("connection"), process_header_connection}
};

HttpHeadersIn::HttpHeadersIn()
:   m_keep_alive_n(-1),
    m_content_length_n(-1),
    m_connection_type(0),
    m_chunked(false)
{
    m_host              = m_headers.end();
    m_keep_alive        = m_headers.end();
    m_content_length    = m_headers.end();
}

HttpHeadersIn::~HttpHeadersIn()
{

}

HttpHeadersIn::iterator 
HttpHeadersIn::push_front_header(StringSlice key, StringSlice lowcase_key, StringSlice value, size_t hash)
{
    m_headers.emplace_front(key, lowcase_key, value, hash);
    return m_headers.begin();
}

Int HttpHeadersIn::handle_header(iterator iter, HttpRequest *r)
{
    unsigned hash = get_hash(iter);
    
    auto h_iter = s_header_handlers.find(hash);
    if (h_iter != s_header_handlers.end()) {
        if (h_iter->second) {
            return h_iter->second(r, iter);
        }
    }
    str_printf(get_header_key(iter));
    printf(": ");
    str_printf(get_header_value(iter));
    printf(" : 未被处理\n");
    fflush(stdout);
    return OK;
}

Int process_header_host(HttpRequest *r, HttpHeadersIn::iterator iter) 
{
    HttpHeadersIn::iterator &host_iter = r->m_header_in.m_host;

    if (r->m_header_in.m_headers.end() != host_iter) {
        LOG_ERROR(LogLevel::info, "%s: %d  多次发送 host\n", __FILE__, __LINE__);
        return ERROR;
    }

    host_iter = iter;
    return OK;
}


Int process_header_connection(HttpRequest *r, HttpHeadersIn::iterator iter) 
{
    const StringSlice &ss = r->m_header_in.get_header_value(iter);

    if (ss == "close") {
        r->m_header_in.m_connection_type = HTTP_CONNECTION_CLOSE;
    
    } else if (ss == "keep-alive"){
        r->m_header_in.m_connection_type =  HTTP_CONNECTION_KEEPALIVE;

    } else {
        return ERROR;
    }
    return OK;
}