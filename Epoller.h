#ifndef _EPOLLER_H_INCLUDED_
#define _EPOLLER_H_INCLUDED_

#include "Config.h"

class Connection;
class Event;
class Epoller {
public:
    Epoller();
    ~Epoller();
    Int create1();
    Int process_events_and_timers();
    
    Int add_connection(Connection *c);
    Int del_connection(Connection *c, bool close);
    
    Int add_read_event(Event *ev, unsigned flags); 
    Int del_read_event(Event *ev, bool close);

    Int add_write_event(Event *ev, unsigned flags); 
    Int del_write_event(Event *ev, bool close);

    void add_timer(Event *ev, mSec timer);
    void del_timer(Event *ev);
    
private:
    int get_epoll_timeout() const;
    void expire_timers();
    int                         m_epfd;
    epoll_event                 m_event_list[EPOLL_EVENTS_SLOT];

    class Cmp {
    public:
        bool operator()(const Event *a, const Event *b);
    };

    std::set<Event*, Cmp>      m_timer_rbtree;
};

#endif