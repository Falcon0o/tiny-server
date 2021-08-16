#include "HttpHeadersIn.h"

#include "HttpRequest.h"


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
