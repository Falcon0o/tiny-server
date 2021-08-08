#ifndef _HTTP_HEADERS_OUT_H_INCLUDED_
#define _HTTP_HEADERS_OUT_H_INCLUDED_

#include "Config.h"


// #include <unordered_map>

class HttpHeadersOut 
{
public:
    off_t                           m_content_length_n;
    time_t                          m_last_modified_time;
    uInt                            m_status;                             
//     HttpHeadersOut();
//     ~HttpHeadersOut();
// private:

//     static std::unordered_map<unsigned, String>    s_content_types;

//     HttpStatus                      m_status;

//     
//     String                          m_content_type;
//     friend class HttpRequest;
};
#endif