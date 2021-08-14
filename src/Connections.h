#ifndef _CONNECTIONS_H_INCLUDED_
#define _CONNECTIONS_H_INCLUDED_

#include "Connection.h"
#include "Event.h"

class Connections {
public:
    Connections();
    ~Connections();

    Connections(const Connections &)            = delete;
    Connections& operator=(const Connections &) = delete;
    Connections(Connections &&)                 = delete;
    Connections& operator=(Connections &&)      = delete;

    Connection *get_connection(int fd);
    void free_connection(Connection *c);
    void reusable_connection(Connection *c, bool reusable);

private:

    void drain_connections();

    Connection                      m_connections[CONNECTIONS_SLOT];
    Event                           m_read_events[CONNECTIONS_SLOT];
    Event                           m_write_events[CONNECTIONS_SLOT];

    Connection                     *m_free_connections;
    size_t                          m_free_connections_n;

    std::list<Connection*>          m_reusable_connections;
    size_t                          m_reusable_connections_n;
    time_t                          m_reuse_time;


};

#endif

