#ifndef _EVENT_H_INCLUDED_
#define _EVENT_H_INCLUDED_

#include "Config.h"

class Connection;


class Event 
{
public: 
    typedef void(Event::*Handler)();


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
    unsigned            m_posted:1;

    std::list<Event*>::iterator m_posted_iter;

    mSec                m_timer;                
    ssize_t             m_available_n;
    Connection         *m_connection;
    // conn->m_close

    void set_handler(Handler h) { 
        m_handler = h; }

    void run_handler() { (this->*m_handler)(); }
    void set_epoll_event_data(epoll_event *ee, const class Connection *c) const {
        ee->data.ptr = (void *) ((uintptr_t) c | m_instance);
    }

    void    accept_handler();
    void    wait_http_request_handler(); 
    void    process_http_request_line_handler(); // ngx_http_process_request_line
    void    process_http_request_headers_handler();
    void    http_request_handler();
    void    empty_handler();
    void    http_keepalive_handler();// ngx_http_keepalive_handler
    void    http_lingering_close_handler(); // ngx_http_lingering_close_handler

private:
    Handler m_handler;
};


#endif