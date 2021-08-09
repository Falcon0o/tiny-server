#include "Listening.h"


Listening::Listening(in_port_t port)
:   m_worker_id(0),
    m_port(port),
    m_fd(-1),
    m_backlog(1024),
    m_connection(nullptr),
    m_reuseport(true),
    m_ipv6only(false),
    m_listening(false),
    m_keepalive(TCP_KEEPALIVE_ON),
    m_deferred_accept(true),
    m_rcvbuf(-1),
    m_sndbuf(-1),
    m_keepidle(0),
    m_keepintvl(0),
    m_keepcnt(0),
    m_fastopen(-1)
{
    memset(&m_sockaddr, 0, sizeof(sockaddr));
    m_sockaddr.sa_family = AF_INET;

    sockaddr_in *sin = reinterpret_cast<sockaddr_in*>(&m_sockaddr);
    sin->sin_port = htons(port);
    sin->sin_addr.s_addr = INADDR_ANY;
}

Listening::Listening(const Listening &right, unsigned worker_id) 
:   m_worker_id(worker_id),
    m_sockaddr(right.m_sockaddr),
    m_port(right.m_port),
    m_fd(right.m_fd),
    m_backlog(right.m_backlog),
    m_connection(right.m_connection),
    m_reuseport(right.m_reuseport),
    m_ipv6only(right.m_ipv6only),
    m_listening(right.m_listening),
    m_keepalive(right.m_keepalive),
    m_deferred_accept(right.m_deferred_accept),
    m_rcvbuf(right.m_rcvbuf),
    m_sndbuf(right.m_sndbuf),
    m_keepidle(right.m_keepidle),
    m_keepintvl(right.m_keepintvl),
    m_keepcnt(right.m_keepcnt),
    m_fastopen(right.m_fastopen)
{

}

Listening::Listening(const Listening &right)
:   Listening(right, right.m_worker_id)
{
}

Listening::Listening(Listening &&right) 
:   m_worker_id(right.m_worker_id),
    m_sockaddr(right.m_sockaddr),
    m_port(right.m_port),
    m_fd(right.m_fd),
    m_backlog(right.m_backlog),
    m_connection(right.m_connection),
    m_reuseport(right.m_reuseport),
    m_ipv6only(right.m_ipv6only),
    m_listening(right.m_listening),
    m_keepalive(right.m_keepalive),
    m_deferred_accept(right.m_deferred_accept),
    m_rcvbuf(right.m_rcvbuf),
    m_sndbuf(right.m_sndbuf),
    m_keepidle(right.m_keepidle),
    m_keepintvl(right.m_keepintvl),
    m_keepcnt(right.m_keepcnt),
    m_fastopen(right.m_fastopen)
{
    right.m_fd = -1;
}

Listening::~Listening()
{

}

Int Listening::open_listening_socket()
{
    if ((m_fd = socket(m_sockaddr.sa_family, SOCK_STREAM, 0)) == -1) {
        return ERROR;
    }

#define CloseSocketAndReturn(file, line)                                    \
    log_error(LogLevel::alert, "(%s: %d) \n", file, line);                  \
    if (close(m_fd) == -1) {                                                \
        log_error(LogLevel::alert, "(%s: %d) close() 失败\n", file, line);  \
    }                                                                       \
    m_fd = -1;                                                              \
    return ERROR;

    int onoff = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, 
                                &onoff, sizeof(int)) == -1) 
    {
        CloseSocketAndReturn(__FILE__, __LINE__);
    }

    if (m_sockaddr.sa_family == AF_INET6) {
        onoff = m_ipv6only;
        if (setsockopt(m_fd, IPPROTO_IPV6, IPV6_V6ONLY, 
                                &onoff, sizeof(onoff)) == -1)
        {
            log_error(LogLevel::alert, "(%s: %d) \n", __FILE__, __LINE__);
        }
    }

    socklen_t socklen;

    switch (m_sockaddr.sa_family) 
    {
    case AF_INET:
        socklen = sizeof(sockaddr_in);
        break;

    case AF_INET6:
        socklen = sizeof(sockaddr_in6);
        break;

    default:
        CloseSocketAndReturn(__FILE__, __LINE__);
    }

    onoff = 1;
    if (ioctl(m_fd, FIONBIO, &onoff) == -1) {
        CloseSocketAndReturn(__FILE__, __LINE__);
    }
    if (bind(m_fd, &m_sockaddr, socklen) == -1) {
        CloseSocketAndReturn(__FILE__, __LINE__);
    }

    if (listen(m_fd, m_backlog) == -1) {
        CloseSocketAndReturn(__FILE__, __LINE__);
    }

/* 以下的设置即使失败不需要关闭 m_fd */
#undef CloseSocketAndReturn

    if (m_rcvbuf != -1) {
        if (setsockopt(m_fd, SOL_SOCKET, SO_RCVBUF,
                                &m_rcvbuf, sizeof(m_rcvbuf)) == -1)
        {
            log_error(LogLevel::alert, "(%s: %d) \n", __FILE__, __LINE__);
        }
    }

    if (m_keepalive) {
        onoff = m_keepalive == TCP_KEEPALIVE_ON? 1: 0;
        
        if (setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE,
                                &onoff, sizeof(onoff)) == -1)
        {
            log_error(LogLevel::alert, "(%s: %d) \n", __FILE__, __LINE__);
        }
    }

    if (m_sndbuf != -1) {
        if (setsockopt(m_fd, SOL_SOCKET, SO_SNDBUF,
                                &m_sndbuf, sizeof(m_sndbuf)) == -1)
        {
            log_error(LogLevel::alert, "(%s: %d) \n", __FILE__, __LINE__);
        }
    }

    

    if (m_keepidle) {
        if (setsockopt(m_fd, IPPROTO_TCP, TCP_KEEPIDLE,
                                &m_keepidle, sizeof(m_keepidle)) == -1)
        {
            log_error(LogLevel::alert, "(%s: %d) \n", __FILE__, __LINE__);
        }
    }

    if (m_keepcnt) {
        if (setsockopt(m_fd, IPPROTO_TCP, TCP_KEEPCNT,
                                &m_keepcnt, sizeof(m_keepcnt)) == -1)
        {
            log_error(LogLevel::alert, "(%s: %d) \n", __FILE__, __LINE__);
        }
    }

    if (m_keepintvl) {
        if (setsockopt(m_fd, IPPROTO_TCP, TCP_KEEPINTVL,
                                &m_keepintvl, sizeof(m_keepintvl)) == -1)
        {
            log_error(LogLevel::alert, "(%s: %d) \n", __FILE__, __LINE__);
        }
    }

    if (m_fastopen != -1) {
        if (setsockopt(m_fd, IPPROTO_TCP, TCP_FASTOPEN, 
                                &m_fastopen, sizeof(m_fastopen)) == -1)
        {
            log_error(LogLevel::alert, "(%s: %d) \n", __FILE__, __LINE__);
        }
    }

    if (m_deferred_accept) {
        /*
         * 以下为nginx中关于defer accept超时时间设置的注释：
         * There is no way to find out how long a connection was
         * in queue (and a connection may bypass deferred queue at all
         * if syncookies were used), hence we use 1 second timeout
         * here. 
         * 大概意思是：无法判断一个连接在未完成三次握手的队列（延迟队列）里呆
         * 了多久，而且一个使用了syncookies（fastopen）的连接可能会不在这个
         * 延迟队列中，因而这里使用1s作为defer_accept超时
         */
        int timeout = 1;
        if (setsockopt(m_fd, IPPROTO_TCP, TCP_DEFER_ACCEPT,
                                &timeout, sizeof(timeout)) == -1) 
        {
            log_error(LogLevel::alert, "(%s: %d) \n", __FILE__, __LINE__);
        }
    }
    return OK;
}

// void ListeningSocket::init_connection(Connection *conn) 
// {
//     HttpConnection *hc = new HttpConnection();

//     if (!hc) {
//         conn->close_http_connection();
//         return;
//     }
//     conn->set_data(hc);
//     Event *rev = conn->m_rev;
//     Event *wev = conn->m_wev;
    
    
//     rev->set_handler(wait_request_handler);
//     wev->set_handler(empty_handler);

//     if (rev->ready()) {
//         rev->run_handler();
//         return;
//     }
//     // TODO
//     // ngx_add_timer(rev, cscf->client_header_timeout);
//     // ngx_reusable_connection(c, 1);
    
//     // get_singleton_cycle()->epoll_add_read_event(rev);
// }