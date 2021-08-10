#include "StringSlice.h"

StringSlice::StringSlice()
:   StringSlice(nullptr, 0)
{

}

StringSlice::StringSlice(u_char *data, size_t len)
:   m_data(data),
    m_len(len)
{

}

StringSlice::StringSlice(const StringSlice &right)
:   StringSlice(right.m_data, right.m_len)
{ }

StringSlice::StringSlice(StringSlice &&right)
:   StringSlice(right.m_data, right.m_len)
{
    right.m_data = nullptr;
    right.m_len = 0;
}

StringSlice GetHttpStatusLines(uInt http_status)
{
    
    // ngx_string("301 Moved Permanently"),
    // ngx_string("302 Moved Temporarily"),
    // ngx_string("303 See Other"),
    switch (http_status) 
    {
    case HTTP_OK:                       return STRING_SLICE("200 OK");
    case CREATED:                       return STRING_SLICE("201 Created");
    // ("202 Accepted");
    case NO_CONTENT:                    return STRING_SLICE("204 No Content");
    case PARTIAL_CONTENT:               return STRING_SLICE("206 Partial Content");
    case NOT_MODIFIED:                  return STRING_SLICE("304 Not Modified");
    // ("307 Temporary Redirect");
    // ("308 Permanent Redirect");
    case BAD_REQUEST:                   return STRING_SLICE("400 Bad Request");
    // ("401 Unauthorized");
    // ("402 Payment Required");
    // ("403 Forbidden")
    case NOT_FOUND:                     return STRING_SLICE("404 Not Found");
    case NOT_ALLOWED:                   return STRING_SLICE("405 Not Allowed");
    // ("406 Not Acceptable")
    case REQUEST_TIME_OUT:              return STRING_SLICE("408 Request Time-out");
    // ("409 Conflict")
    // ("410 Gone")
    // ("411 Length Required")
    // ("412 Precondition Failed")
    // ("413 Request Entity Too Large")
    case REQUEST_URI_TOO_LARGE:         return STRING_SLICE("414 Request-URI Too Large");
    // ("415 Unsupported Media Type")
    // ("416 Requested Range Not Satisfiable")
    // ("421 Misdirected Request")
    // ("429 Too Many Requests")
    // case REQUEST_HEADER_TOO_LARGE:
    case INTERNAL_SERVER_ERROR:         return STRING_SLICE("500 Internal Server Error");
    // ("501 Not Implemented")
    // ("502 Bad Gateway")
    // ("503 Service Temporarily Unavailable")
    // ("504 Gateway Time-out")
    case VERSION_NOT_SUPPORTED:         return STRING_SLICE("505 HTTP Version Not Supported");
    // ("507 Insufficient Storage")
    }
    log_error(LogLevel::info, "(%s %d) 未定义的 States Line", __FILE__, __LINE__);
    return StringSlice();
}

void str_lowcase(u_char *dst, u_char *src, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        dst[i] = (src[i] >= 'A' && src[i] <= 'Z') ? (src[i] | 0x20) : src[i];
    }
}

void str_printf(const StringSlice &s)
{
    for(size_t i = 0; i < s.m_len; ++i) {
        printf("%c", s.m_data[i]);
    }
}

bool StringSlice::operator==(const char *c) const
{
    const unsigned char *pos_1 = m_data;
    const char* pos_2 = c;

    size_t i = 0;
    while (i < m_len && *pos_2 != '\0') {
        if (pos_1[i] != *pos_2) {
            return false;
        }

        ++i;
        ++pos_2;
    }

    return i == m_len && *pos_2 == '\0';
}
StringSlice &StringSlice::operator=(const char *c)
{
    m_data = (u_char*)c;
    m_len = strlen(c);
    return *this;
}


   // unsigned char c_1 = *pos_1;
        // c_1 = (c_1 >= 'A' && c_1 <= 'Z') ? (c_1 | 0x20) : c_1;
        
        // char c_2 = *pos_2;
        // c_1 = (c_2 >= 'A' && c_2 <= 'Z') ? (c_2 | 0x20) : c_2;

size_t StringSlice::hash()const
{
    size_t ans = 0;
    for (size_t i = 0; i < m_len; ++i) {
        ans = ::hash(ans, m_data[i]);
    }
    return ans;
}

StringSlice &StringSlice::operator=(const StringSlice &right) 
{
    if (&right != this) {
        m_data = right.m_data;
        m_len = right.m_len;
    }
    return *this;
}