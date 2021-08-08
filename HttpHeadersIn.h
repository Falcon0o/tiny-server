#ifndef _HTTP_HEADERS_IN_H_INCLUDED_
#define _HTTP_HEADERS_IN_H_INCLUDED_

// #include <forward_list>
// #include <tuple>
// #include <unordered_map>

#include "Config.h"

#include "StringSlice.h"

// #include "sys/types.h"



class HttpRequest;

class HttpHeadersIn {
    // using slist = std::forward_list<std::tuple<String, String, unsigned, unsigned char*>>;
public:
    off_t                                               m_content_length_n;
    time_t                                              m_keep_alive_n;

//     using iterator = slist::iterator;
//     HttpHeadersIn();
//     ~HttpHeadersIn();

//     iterator push_front_header(String key, String value, unsigned hash);
//     iterator push_front_header(String key, unsigned char *lowcast_key, String value, unsigned hash);

//     void init_iterator_to_before_first();
//     ResultCode handle_header(iterator iter, HttpRequest *r);
// private:
//     unsigned get_hash(iterator &iter) {     return std::get<2>(*iter);  }
//     String get_header_key(iterator &iter) {     return std::get<0>(*iter);  }
//     String get_header_value(iterator &iter) {     return std::get<1>(*iter);  }


//     slist                                               m_headers;
//     /* 以下iterator类型的参数必须在 m_headers后定义 */
//     iterator                                            m_host;
//     iterator                                            m_keep_alive;

//     iterator                                            m_content_length;


//     /* 0: unset 1: close 2: keep-alive*/
//     unsigned                                            m_connection_type:2;
//     unsigned                                            m_chunked:1;
//     typedef ResultCode (*http_header_handler)(HttpRequest*, iterator);
//     static std::unordered_map<unsigned, http_header_handler>           s_header_handlers;

//     friend class HttpRequest;
//     friend ResultCode process_header_host(HttpRequest *, HttpHeadersIn::iterator);
//     friend ResultCode process_header_connection(HttpRequest *, HttpHeadersIn::iterator);
};
#endif