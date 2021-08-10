#include "Connections.h"


#include "Timer.h"

static Connections singleton;

Connections *g_connections = &singleton;


Connections::Connections()
:   m_reusable_connections_n(0),
    m_reuse_time(0)
{
    Connection *next = nullptr;
    for (int i = CONNECTIONS_SLOT - 1; i >= 0; --i) {
        
        m_connections[i].m_data.c = next;
        m_connections[i].m_read_event = &m_read_events[i];
        m_connections[i].m_write_event = &m_write_events[i];
        m_connections[i].m_fd = -1;
        m_read_events[i].m_instance = true;
        m_read_events[i].m_closed = true;
        m_write_events[i].m_closed = true;

        next = &m_connections[i];
    }

    m_free_connections = m_connections;
    m_free_connections_n = CONNECTIONS_SLOT;
}

Connections::~Connections()
{

}

void Connections::free_connection(Connection *c)
{
    c->m_data.c = m_free_connections;
    m_free_connections = c;
    ++m_free_connections_n;
}

Connection *Connections::get_connection(int fd) 
{
    drain_connections();

    if (m_free_connections == nullptr) {
        return nullptr;
    }

    Connection *c = m_free_connections;
    m_free_connections = c->m_data.c;
    --m_free_connections_n;
    

    Event *rev = c->m_read_event;
    Event *wev = c->m_write_event;

    memset(c, 0, sizeof(Connection));
    c->m_read_event = rev;
    c->m_write_event = wev;
    c->m_fd = fd;

    uInt instance = rev->m_instance;
    memset(rev, 0, sizeof(Event));
    memset(wev, 0, sizeof(Event));

    rev->m_instance = !instance;
    wev->m_instance = !instance;

    // rev->index = NGX_INVALID_INDEX;
    // wev->index = NGX_INVALID_INDEX;

    rev->m_connection = c;
    wev->m_connection = c;
    wev->m_write = true;

    new(&c->m_pool)Pool;
    return c;
}

void Connections::drain_connections()
{
    if (m_free_connections_n > CONNECTIONS_SLOT / 16
        || m_reusable_connections_n == 0) 
    {
        return;
    }

    if (g_timer->cached_time_sec() != m_reuse_time) {
        m_reuse_time == g_timer->cached_time_sec();
    } 

    size_t n = m_reusable_connections_n / 8;
    n = n > 32 ? 32 : n;
    n = n > 1 ? n : 1;

    Connection *conn = nullptr;
    for (size_t i = 0; i < n; ++i) {
        if (m_reusable_connections.empty()) {
            break;
        }

        conn = m_reusable_connections.back();
        conn->m_close = true;
        conn->m_read_event->run_handler();
    }

    if (m_free_connections_n == 0 && conn && conn->m_reusable)
    {
        conn->m_close = true;
        conn->m_read_event->run_handler();
    }
}

void Connections::reusable_connection(Connection *c, bool reusable)
{
    if (c->m_reusable) {
        m_reusable_connections.erase(c->m_reusable_iter);
        --m_reusable_connections_n;
    }

    c->m_reusable = reusable;

    if (c->m_reusable) {
        m_reusable_connections.push_front(c);
        c->m_reusable_iter = m_reusable_connections.begin();
        ++m_reusable_connections_n;
    }
}