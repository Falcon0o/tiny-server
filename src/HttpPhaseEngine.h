#ifndef _HTTP_PHASE_ENGINE_H_INCLUDED_
#define _HTTP_PHASE_ENGINE_H_INCLUDED_

#include "Config.h"

class HttpRequest;
class HttpPhaseEngine
{
public: 
    HttpPhaseEngine(HttpRequest *r);
    typedef Int(HttpPhaseEngine::*Checker)();

    static std::vector<Checker> checkers;
        // ngx_http_core_content_phase
    Int content_phase_checker();

private:
    HttpRequest *m_request;
}; 

#endif
