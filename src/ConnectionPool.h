#ifndef _CONNECTION_POOL_H_INCLUDED_
#define _CONNECTION_POOL_H_INCLUDED_
#include "Config.h"

class Connection;
class Event;

class ConnectionPool {
public:
    ConnectionPool(size_t slot) noexcept;
    ~ConnectionPool();

    ConnectionPool()                                    = delete;
    ConnectionPool(const ConnectionPool &)              = delete;
    ConnectionPool& operator=(const ConnectionPool &)   = delete;
    ConnectionPool(ConnectionPool &&)                   = delete;
    ConnectionPool& operator=(ConnectionPool &&)        = delete;

    Int initialize();
    Connection *get_connection(int fd);
    void free_connection(Connection *);
    void reusable_connection(Connection *, bool reusable);

private:
    void drain_connections();

    const size_t                    m_connections_slot;
    Connection                     *m_connections;

    Connection                     *m_free_connections;
    size_t                          m_free_connections_n;

    std::list<Connection*>          m_reusable_connections;
    size_t                          m_reusable_connections_n;
    time_t                          m_reuse_time;
};
#endif