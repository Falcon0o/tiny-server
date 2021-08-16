#ifndef _EVENT_H_INCLUDED_
#define _EVENT_H_INCLUDED_

#include "Config.h"

class Connection;


class Event 
{
public: 
    Event(bool instance);
    ~Event() { }
    
    Event()                         = delete;
    Event(const Event &)            = delete;
    Event& operator=(const Event &) = delete;
    Event(Event &&)                 = delete;
    Event& operator=(Event &&)      = delete;

    void reinitialize(Connection *, bool instance, bool write) noexcept;

    typedef void(Event::*Handler)();

/* 关于stale events，例如epoll_wait返回事件 A B C，有以下几种情况，
    1.  A关闭了B事件所在连接，则处理B事件可以从connection::m_fd == -1发现被关闭
    2.  A关闭了B事件，若B事件所在连接未被关闭，则B事件的m_instance的会和···相反 */

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

    void    accept_connection();
    void    wait_http_request(); 
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