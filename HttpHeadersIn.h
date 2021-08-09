#ifndef _HTTP_HEADERS_IN_H_INCLUDED_
#define _HTTP_HEADERS_IN_H_INCLUDED_

#include "Config.h"
#include "StringSlice.h"


enum {
    HTTP_CONNECTION_CLOSE = 1,
    HTTP_CONNECTION_KEEPALIVE = 2
};

class HttpRequest;

class HttpHeadersIn {
    using slist = std::forward_list<std::tuple<StringSlice, StringSlice, StringSlice, size_t>>;
public:

    using iterator = slist::iterator;
    slist                                               m_headers;
    iterator                                            m_host;

    iterator                                            m_keep_alive;
    time_t                                              m_keep_alive_n;
    
    off_t                                               m_content_length_n;
    iterator                                            m_content_length;

    unsigned                                            m_connection_type:2;
    unsigned                                            m_chunked:1;
    typedef Int (*http_header_handler)(HttpRequest*, iterator);
    static std::unordered_map<unsigned, http_header_handler>           s_header_handlers;

    HttpHeadersIn();
    ~HttpHeadersIn();
    
    StringSlice get_header_key(iterator &iter)          {   return std::get<0>(*iter);  }
    StringSlice get_header_lowcase_key(iterator &iter)  {   return std::get<1>(*iter);  }
    StringSlice get_header_value(iterator &iter)        {   return std::get<2>(*iter);  }
    size_t get_hash(iterator &iter)                     {   return std::get<3>(*iter);  }

    iterator push_front_header(StringSlice key, StringSlice lowcase_key, StringSlice value, size_t hash);
    Int handle_header(iterator iter, HttpRequest *r);

};

//     void init_iterator_to_before_first();

#endif