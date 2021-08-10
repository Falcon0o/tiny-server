#ifndef _HTTP_HEADERS_OUT_H_INCLUDED_
#define _HTTP_HEADERS_OUT_H_INCLUDED_

#include "Config.h"


#include "StringSlice.h"

class HttpHeadersOut 
{
public:
    off_t                           m_content_length_n;
    time_t                          m_last_modified_time;
    uInt                            m_status; 
    StringSlice                     m_content_type;                      
    StringSlice                     m_status_line;

    static std::unordered_map<unsigned, StringSlice>    s_content_types;

//     HttpStatus                      m_status;

//     
//     String                          m_content_type;
//     friend class HttpRequest;
};
#endif