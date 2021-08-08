#ifndef _EVENT_H_INCLUDED_
#define _EVENT_H_INCLUDED_

#include "Config.h"

class Event 
{
public: 
    unsigned            m_instance:1;
    unsigned            m_closed:1;
    unsigned            m_write:1;
    unsigned            m_accept:1;
    unsigned            m_deferred_accept:1;
    unsigned            m_pending_eof:1;
    unsigned            m_active:1;
    unsigned            m_timeout:1;

    unsigned            m_delayed:1;
    unsigned            m_ready:1;
    unsigned            m_eof:1;
    unsigned            m_err:1;
    unsigned            m_timer_set:1;
    
    mSec                m_timer;                
    ssize_t             m_available_n;
    void               *m_data;
    // conn->m_close

    
    template <typename T> 
    void set_event_handler(T f) { 
        m_handler = std::bind(f, this); }

    void run_event_handler() { m_handler(); }
    void set_epoll_event_data(epoll_event *ee, const class Connection *c) const {
        ee->data.ptr = (void *) ((uintptr_t) c | m_instance);
    }

    void    accept_handler();
    void    wait_http_request_handler(); 
    void    process_http_request_line_handler(); // ngx_http_process_request_line
    void    process_http_request_headers_handler();
    void    empty_handler();
private:
    std::function<void()> m_handler;
};


#endif