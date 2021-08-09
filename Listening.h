#ifndef _LISTENING_H_INCLUDED_
#define _LISTENING_H_INCLUDED_

#include "Config.h"

class Connection;
class Listening {
public:
    Listening(in_port_t port);
    Listening(const Listening&);
    Listening(const Listening&, unsigned worker_id);
    Listening(Listening &&);
    ~Listening();

    Int open_listening_socket();
    // void init_connection(Connection*);

    unsigned                    m_worker_id;
    sockaddr                    m_sockaddr;
    const in_port_t             m_port;
    int                         m_fd;
    int                         m_backlog;
    Connection                 *m_connection;
    unsigned int                m_reuseport:1;
    unsigned int                m_ipv6only:1;
    unsigned int                m_listening:1;
    enum {
        TCP_KEEPALIVE_UNSET = 0,
        TCP_KEEPALIVE_ON,
        TCP_KEEPALIVE_OFF,
    };
    unsigned int                m_keepalive:2;
    unsigned int                m_deferred_accept:1;
    // unsigned int             add_deferred_:1;
    // unsigned int             delete_deferred_:1;
    int                         m_rcvbuf;
    int                         m_sndbuf;

    int                         m_keepidle;      /* 操作系统默认 7200s */
    int                         m_keepcnt;       /* 操作系统默认 9次 */
    int                         m_keepintvl;     /* 操作系统默认 75s */

    int                         m_fastopen;
};

#endif
