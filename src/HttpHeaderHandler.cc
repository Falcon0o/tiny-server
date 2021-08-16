#include "HttpHeadersIn.h"

#include "HttpRequest.h"

static Int headerHandler_host(HttpRequest *, HttpHeadersIn::iterator);
static Int headerHandler_connection(HttpRequest *, HttpHeadersIn::iterator);
static Int headerHandler_userAgent(HttpRequest *r, HttpHeadersIn::iterator iter);
static Int headerHandler_accept(HttpRequest *r, HttpHeadersIn::iterator iter);
static Int headerHandler_acceptEncoding(HttpRequest *r, HttpHeadersIn::iterator iter);
static Int headerHandler_acceptLanguage(HttpRequest *r, HttpHeadersIn::iterator iter);
static Int headerHandler_cacheControl(HttpRequest *r, HttpHeadersIn::iterator iter);
static Int headerHandler_pragma(HttpRequest *r, HttpHeadersIn::iterator iter);
static Int headerHandler_referer(HttpRequest *r, HttpHeadersIn::iterator iter);
static Int headerHandler_ifModifiedSince(HttpRequest *r, HttpHeadersIn::iterator iter);
static Int headerHandler_upgradeInsecureRequests(HttpRequest *r, HttpHeadersIn::iterator iter);


std::unordered_map<unsigned, HttpHeadersIn::http_header_handler> 
HttpHeadersIn::s_header_handlers = {
    {hash_string("host"),               headerHandler_host},
    {hash_string("connection"),         headerHandler_connection},
    {hash_string("user-agent"),         headerHandler_userAgent},
    {hash_string("accept"),             headerHandler_accept},
    {hash_string("accept-encoding"),    headerHandler_acceptEncoding},
    {hash_string("accept-language"),    headerHandler_acceptLanguage},
    {hash_string("cache-control"),      headerHandler_cacheControl},
    {hash_string("pragma"),             headerHandler_pragma},
    {hash_string("referer"),            headerHandler_referer},
    {hash_string("if-modified-since"),  headerHandler_ifModifiedSince},
    {hash_string("upgrade-insecure-requests"),  headerHandler_upgradeInsecureRequests}
};



Int headerHandler_host(HttpRequest *r, HttpHeadersIn::iterator iter) 
{
    HttpHeadersIn::iterator &host_iter = r->m_header_in.m_host;

    if (r->m_header_in.m_headers.end() != host_iter) {
        LOG_ERROR(LogLevel::info, "%s: %d  多次发送 host\n", __FILE__, __LINE__);
        return ERROR;
    }

    host_iter = iter;
    return OK;
}


Int headerHandler_connection(HttpRequest *r, HttpHeadersIn::iterator iter) 
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

Int headerHandler_userAgent(HttpRequest *r, HttpHeadersIn::iterator iter) {
    return OK;
}

Int headerHandler_accept(HttpRequest *r, HttpHeadersIn::iterator iter) {
    return OK;
}

Int headerHandler_acceptEncoding(HttpRequest *r, HttpHeadersIn::iterator iter) {
    return OK;
}
Int headerHandler_acceptLanguage(HttpRequest *r, HttpHeadersIn::iterator iter) {
    return OK;
}

Int headerHandler_cacheControl(HttpRequest *r, HttpHeadersIn::iterator iter) {
    return OK;
}

Int headerHandler_pragma(HttpRequest *r, HttpHeadersIn::iterator iter) {
    return OK;
}

Int headerHandler_referer(HttpRequest *r, HttpHeadersIn::iterator iter) {
    return OK;
}

Int headerHandler_ifModifiedSince(HttpRequest *r, HttpHeadersIn::iterator iter) {
    return OK;
}

Int headerHandler_upgradeInsecureRequests(HttpRequest *r, HttpHeadersIn::iterator iter) {
    return OK;
}
