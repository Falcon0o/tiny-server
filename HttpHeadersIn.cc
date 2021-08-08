#include "HttpHeadersIn.h"

#include <string.h>

#include "HttpRequest.h"


// ResultCode process_header_host(HttpRequest *, HttpHeadersIn::iterator);
// ResultCode process_header_connection(HttpRequest *, HttpHeadersIn::iterator);




// std::unordered_map<unsigned, HttpHeadersIn::http_header_handler> 
// HttpHeadersIn::s_header_handlers = {
//     {hash_string("host"), process_header_host},
//     {hash_string("connection"), process_header_connection}
// };

// HttpHeadersIn::HttpHeadersIn()
// :   m_host(m_headers.end()),
//     m_keep_alive(m_headers.end()),
//     m_keep_alive_n(0),
//     m_connection_type(UNSET),
//     m_content_length(m_headers.end()),
//     m_content_length_n(0),
//     m_chunked(false)
// { }

// HttpHeadersIn::~HttpHeadersIn()
// { 
//     for(auto iter = m_headers.begin(); iter != m_headers.end(); ++iter) {
//         unsigned char *h = std::get<3>(*iter);
//         delete[] h;
//     }
// }

// HttpHeadersIn::iterator 
// HttpHeadersIn::push_front_header(String key, String value, unsigned hash)
// {
//     unsigned char *lowcase_key = new unsigned char[key.length()];
//     int n = key.length();

//     unsigned char *c = key.start();
//     for (int i = 0; i < n; ++i) {
//         lowcase_key[i] = (c[i] >= 'A' && c[i] <= 'Z') ? (c[i] | 0x20) : c[i]; 
//     }
    
//     m_headers.emplace_front(key, value, hash, lowcase_key);
//     return m_headers.begin();
// }

// HttpHeadersIn::iterator 
// HttpHeadersIn::push_front_header(String key, unsigned char *lowcast_key, String value, unsigned hash) {
//     unsigned char *lk = new unsigned char[key.length()];
//     memcpy(lk, lowcast_key, key.length());
//     m_headers.emplace_front(key, value, hash, lk);
//     return m_headers.begin();
// }

// ResultCode HttpHeadersIn::handle_header(HttpHeadersIn::iterator iter, HttpRequest *req) 
// {
//     unsigned hash = get_hash(iter);
//     auto h_iter = s_header_handlers.find(hash);
//     if (h_iter != s_header_handlers.end()) {
//         if (h_iter->second) {
//             return h_iter->second(req, iter);
//         }
//     }

//     String key = get_header_key(iter);
//     String value = get_header_value(iter);
//     log_error(0, "(%s %d) ", __FILE__, __LINE__);
//     key.print();
//     printf(": ");
//     value.print();
//     printf("：未处理\n");
//     fflush(stdout);
//     return RC_OK;
// }

// ResultCode process_header_host(HttpRequest *req, HttpHeadersIn::iterator iter) 
// {
//     HttpHeadersIn::iterator &host_iter = req->m_headers_in.m_host;

//     if (req->m_headers_in.m_headers.end() != host_iter) {
//         log_error(0, "(%s %d) 多次发送 host", __FILE__, __LINE__);
//         return RC_ERROR;
//     }

//     host_iter = iter;
//     return RC_OK;
// }


// ResultCode process_header_connection(HttpRequest *req, HttpHeadersIn::iterator iter) 
// {
//     const String &value = req->m_headers_in.get_header_value(iter);

//     if (value.case_compare("close")) {
//         req->m_headers_in.m_connection_type = OFF;
    
//     } else if (value.case_compare("keep-alive")){
//         req->m_headers_in.m_connection_type = ON;

//     } else {
//         return RC_ERROR;
//     }
//     return RC_OK;
// }