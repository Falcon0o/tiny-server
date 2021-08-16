#include "ConnectionPool.h"

#include "Connection.h"
#include "Event.h"
#include "Timer.h"

ConnectionPool *g_connection_pool = nullptr;

ConnectionPool::ConnectionPool(size_t slot) noexcept
:   m_connections_slot(slot),
    m_connections(nullptr),
    m_free_connections(nullptr),
    m_free_connections_n(0),
    m_reusable_connections_n(0),
    m_reuse_time(0) { }

ConnectionPool::~ConnectionPool() {
    delete[] m_connections;
}

Int ConnectionPool::initialize() {
    
    if (m_connections_slot == 0) {
        return ERROR;
    }

    m_connections = new Connection[m_connections_slot];
    if (m_connections == nullptr) {
        return ERROR;
    }

    Connection *next = nullptr;

    for (int i = m_connections_slot - 1; i >= 0; --i) {
        m_connections[i].m_data.next = next;
        next = &m_connections[i];
    }
    
    m_free_connections = m_connections;
    m_free_connections_n = m_connections_slot;

    return OK;
}

void ConnectionPool::free_connection(Connection *c)
{
    c->m_data.next = m_free_connections;
    m_free_connections = c;
    ++m_free_connections_n;
}

Connection *ConnectionPool::get_connection(int fd) 
{
    drain_connections();

    if (m_free_connections == nullptr) {
        LOG_ERROR(LogLevel::info, "Connections::get_connection() 失败："
                        "连接池中无可用连接\n"
                        " ==== %s %d", __FILE__, __LINE__);
        return nullptr;
    }

    Connection *c = m_free_connections;
    m_free_connections = c->m_data.next;
    --m_free_connections_n;
    c->m_data.next = nullptr;

    c->reinitialize(fd);
    return c;
}

void ConnectionPool::drain_connections()
{
    if (m_free_connections_n > m_connections_slot / 16
        || m_reusable_connections_n == 0) {
        return;
    }

    if (g_timer->cached_time_sec() != m_reuse_time) {
        m_reuse_time == g_timer->cached_time_sec();
    } 

    size_t n = m_reusable_connections_n / 8;
    n = n > 32 ? 32 : n;
    n = n > 1 ? n : 1;

    Connection *c = nullptr;
    for (size_t i = 0; i < n; ++i) {
        if (m_reusable_connections.empty()) {
            break;
        }

        c = m_reusable_connections.back();
        c->m_close = true;
        c->m_rev.run_handler();
    }

    if (m_free_connections_n == 0 && c && c->m_reusable)
    {
        c->m_close = true;
        c->m_rev.run_handler();
    }
}

void ConnectionPool::reusable_connection(Connection *c, bool reusable)
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