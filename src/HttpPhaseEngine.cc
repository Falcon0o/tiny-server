#include "HttpPhaseEngine.h"


#include "HttpRequest.h"
#include "Connection.h"

std::vector<HttpPhaseEngine::Checker> 
HttpPhaseEngine::checkers = 
{
    &HttpPhaseEngine::content_phase_checker
};

HttpPhaseEngine::HttpPhaseEngine(HttpRequest *r)
:   m_request(r)
{
    
}


Int HttpPhaseEngine::content_phase_checker()
{
    HttpRequest *r = m_request;
    
    Int rc = r->http_static_handler();
    if (rc != DECLINED) {
        r->m_connection->finalize_http_request(r, rc);
        return OK;
    }
    return OK;
}