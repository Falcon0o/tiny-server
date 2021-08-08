#include "Epoller.h"

#include "Event.h"
#include "Connection.h"
#include "Process.h"
#include "Timer.h"

static Epoller singleton;

// 主进程的g_epoller 并未被使用

Epoller *g_epoller = &singleton;

Epoller::Epoller()
:   m_epfd(-1)
{

}

Epoller::~Epoller()
{

}

Int Epoller::create1()
{
    m_epfd = epoll_create1(0);
    if (m_epfd == -1) {
        log_error(LogLevel::alert, "(%s: %d) \n", __FILE__, __LINE__);
        return ERROR;
    }
    return OK;
}

Int Epoller::add_read_event(Event *ev, unsigned flags) 
{
    Connection *conn = static_cast<Connection*>(ev->m_data);

    Event *wev = conn->m_write_event;

    epoll_event ee;
    ee.events = EPOLLIN |  EPOLLRDHUP | flags; 
    ev->set_epoll_event_data(&ee, conn);

    int epoll_op = EPOLL_CTL_ADD;

    if (wev->m_active) {
        epoll_op = EPOLL_CTL_MOD;
        ee.events |= EPOLLOUT;
    }

    if (flags & EPOLLEXCLUSIVE) {
        ee.events &= ~EPOLLRDHUP;
    }

    if (epoll_ctl(m_epfd, epoll_op, conn->m_fd, &ee) == -1) {
        int err = errno;
        log_error(LogLevel::alert, "(%s: %d) epoll_ctl() errno %d \n", __FILE__, __LINE__, err);
        return ERROR;
    }

    ev->m_active = true;
    return OK;
}

Int Epoller::add_write_event(Event *ev, unsigned flags) 
{
    Connection *conn = static_cast<Connection*>(ev->m_data);

    Event *rev = conn->m_read_event;

    epoll_event ee;
    ee.events = EPOLLOUT | flags; 
    ev->set_epoll_event_data(&ee, conn);

    int epoll_op = EPOLL_CTL_ADD;

    if (rev->m_active) {
        epoll_op = EPOLL_CTL_MOD;
        ee.events |= EPOLLIN | EPOLLRDHUP;
    }

    if (flags & EPOLLEXCLUSIVE) {
        ee.events &= ~EPOLLRDHUP;
    }

    if (epoll_ctl(m_epfd, epoll_op, conn->m_fd, &ee) == -1) {
        int err = errno;
        log_error(LogLevel::alert, "(%s: %d) epoll_ctl() errno %d \n", __FILE__, __LINE__, err);
        return ERROR;
    }

    ev->m_active = true;
    return OK;
}

Int Epoller::add_connection(Connection *conn) 
{
    epoll_event ee;

    ee.events = EPOLLIN|EPOLLOUT|EPOLLET|EPOLLRDHUP;
    Event *rev = conn->m_read_event;
    rev->set_epoll_event_data(&ee, conn);

    if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, conn->m_fd, &ee) == -1) {
        int err = errno;
        log_error(LogLevel::alert, "(%s: %d) epoll_ctl(EPOLL_CTL_ADD) errno %d \n", __FILE__, __LINE__, err);
        return ERROR;
    }

    conn->m_read_event->m_active = true;
    conn->m_write_event->m_active = true;

    return OK;
}


int Epoller::get_epoll_timeout() const
{
    mSec timer = static_cast<mSec>(-1);
    
    if (TIME_RESOLUTION == 0) {
        if (m_timer_rbtree.empty() == false) {
            Event *ev = *m_timer_rbtree.cbegin();
            timer = ev->m_timer;
            mSec curr = g_curr_msec;
            timer = timer > curr ? timer - curr : 0;
        }
    }

    return static_cast<int>(timer);
}

Int Epoller::process_events_and_timers()
{
    int timeout = get_epoll_timeout();

    int ev_n = epoll_wait(m_epfd, m_event_list, EPOLL_EVENTS_SLOT, timeout);
    int err = (ev_n == -1) ? errno : 0;

    if (g_process->m_sig_alarm || TIME_RESOLUTION == 0) {
        g_timer->update_time();
    }

    if (err) {
        if (err == EINTR) {
            if (g_process->m_sig_alarm) {
                g_process->m_sig_alarm = 0;
                log_error(LogLevel::alert, "(%s: %d) 触发时钟中断\n", __FILE__, __LINE__);
                return OK;
            }
            log_error(LogLevel::info, "(%s: %d) epoll_wait() 因中断失败\n", __FILE__, __LINE__);
        
        } else {
            log_error(LogLevel::alert, "(%s: %d) epoll_wait() 失败\n", __FILE__, __LINE__);
        }

        return ERROR;
    }

    if (ev_n == 0) {
        if (timeout != -1) {
            return OK;
        }
        log_error(LogLevel::alert, "(%s: %d) epoll_wait() returned no"
                                "events without timeout\n", __FILE__, __LINE__);
        return ERROR;
    }

    for (int i = 0; i < ev_n; ++i) {
        epoll_event &ee = m_event_list[i];

        Connection *conn = static_cast<Connection*>(ee.data.ptr);
        unsigned instance = (uintptr_t) conn & 1;
        conn = (Connection*) ((uintptr_t) conn & (uintptr_t) ~1);

        Event *rev = conn->m_read_event;

        if (conn->m_fd == -1 || rev->m_instance != instance)
        {
            /*
             * the stale event from a file descriptor
             * that was just closed in this iteration
             */
            log_error(LogLevel::alert, "(%s: %d) stale event \n", __FILE__, __LINE__);
            continue;
        }

        uint32_t ee_events = ee.events;

        if (ee_events & (EPOLLERR|EPOLLHUP)) {
            log_error(LogLevel::alert, "(%s: %d) EPOLLERR|EPOLLHUP\n", __FILE__, __LINE__);
            ee_events |= EPOLLIN|EPOLLOUT;
        }
        
        if ((ee_events & EPOLLIN) && rev->m_active) {
            if (ee_events & EPOLLRDHUP) {
                rev->m_pending_eof = true;
            }
            rev->m_ready = true;
            rev->m_available_n = -1;
            rev->run_event_handler();
        }

        Event *wev = conn->m_write_event;
        if ((ee_events & EPOLLOUT) && wev->m_active) {
            if (conn->m_fd == -1 || wev->m_instance != instance) {
                log_error(LogLevel::alert, "(%s: %d) stale event \n", __FILE__, __LINE__);
                continue;
            }
            
            wev->m_ready = true;
            wev->run_event_handler();
        }
    }

    expire_timers();
    process_posted_events();
    return OK;
}




Int Epoller::del_read_event(Event *ev, bool close)
{
    if (close) {
        ev->m_active = false;
        return OK;
    }

    Connection *conn = static_cast<Connection*>(ev->m_data);
    Event *wev = conn->m_write_event;

    epoll_event ee;
    int epoll_op;

    if (wev->m_active) {
        epoll_op = EPOLL_CTL_MOD;
        ee.events = EPOLLOUT;
        ev->set_epoll_event_data(&ee, conn);

    } else {
        epoll_op = EPOLL_CTL_DEL;
        ee.events = 0;
        ee.data.ptr = nullptr;
    }

    if (epoll_ctl(m_epfd, epoll_op, conn->m_fd, &ee) == -1) {
        return ERROR;
    }

    ev->m_active = false;
    return OK;
}


Int Epoller::del_write_event(Event *ev, bool close)
{
    if (close) {
        ev->m_active = false;
        return OK;
    }

    Connection *conn = static_cast<Connection*>(ev->m_data);
    Event *rev = conn->m_read_event;

    epoll_event ee;
    int epoll_op;

    if (rev->m_active) {
        epoll_op = EPOLL_CTL_MOD;
        ee.events = EPOLLRDHUP | EPOLLIN;
        ev->set_epoll_event_data(&ee, conn);

    } else {
        epoll_op = EPOLL_CTL_DEL;
        ee.events = 0;
        ee.data.ptr = nullptr;
    }

    if (epoll_ctl(m_epfd, epoll_op, conn->m_fd, &ee) == -1) {
        return ERROR;
    }

    ev->m_active = false;
    return OK;
}






bool Epoller::Cmp::operator()(const Event *a, const Event *b)
{
    if (a->m_timer == b->m_timer)
    {
        return a < b;
    }

    return a->m_timer < b->m_timer;
}

void Epoller::del_timer(Event *ev)
{
    m_timer_rbtree.erase(ev);
    ev->m_timer_set = false;
}

void Epoller::add_timer(Event *ev, mSec timer)
{
    timer += g_curr_msec;

    if (ev->m_timer_set) {
        mSec diff = (timer > ev->m_timer) ? (timer - ev->m_timer) : (ev->m_timer - timer);

        if (diff < TIMER_LAZY_DELAY) {
            return;
        }

        del_timer(ev);
    }

    m_timer_rbtree.insert(ev);
    ev->m_timer_set = true;
}

Int Epoller::del_connection(Connection *c, bool close)
{
    if (close) {
        c->m_read_event->m_active = false;
        c->m_write_event->m_active = false;
        return OK;
    }

    int epoll_op = EPOLL_CTL_DEL;
    epoll_event ee;
    
    ee.events = 0;
    ee.data.ptr = nullptr;

    if (epoll_ctl(m_epfd, epoll_op, c->m_fd, &ee) == -1) {
        log_error(LogLevel::alert, "(%s: %d) epoll_ctl(EPOLL_CTL_DEL) 失败\n", __FILE__, __LINE__);
        return ERROR;
    }

    c->m_read_event->m_active = false;
    c->m_write_event->m_active = false;
    return OK;
}


void Epoller::expire_timers()
{
    while (m_timer_rbtree.empty() == false) {
        
        auto iter = m_timer_rbtree.begin();
        Event *ev = *iter;

        if (ev->m_timer > g_curr_msec) {
            return;
        }

        m_timer_rbtree.erase(iter);

        ev->m_timeout = true;
        ev->m_timer_set = false;
        ev->run_event_handler();
    } 
}

void Epoller::process_posted_events()
{
    while (m_posted_events.empty() == false) {

        auto iter = m_posted_events.begin();
        Event *ev = *iter;
        if (ev->m_posted) {
            ev->m_posted = false;
            ev->run_event_handler();
            m_posted_events.pop_front();
        } 
    }
}

void Epoller::add_posted_event(Event *ev)
{
    if (ev->m_posted == false) {
        ev->m_posted = true;
        m_posted_events.push_back(ev);
        ev->m_posted_iter = m_posted_events.end();
        ev->m_posted_iter--;
    }
}
void Epoller::del_posted_event(Event *ev)
{
    if (ev->m_posted) {
        m_posted_events.erase(ev->m_posted_iter);
        ev->m_posted = false;
    }
}