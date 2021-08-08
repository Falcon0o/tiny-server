#include "Cycle.h"

#include "Epoller.h"
#include "Timer.h"

static Cycle singleton;

Cycle *g_cycle = &singleton;


Cycle::Cycle()
:   m_master_process(Process::Type::master, getppid(), getgid()),
    m_worker_cnt(0),
    m_worker_id(0),
    m_reusable_connections_n(0),
    m_connections_reuse_time(0)
{
    g_process = &m_master_process;
}

Cycle::~Cycle()
{

}

void Cycle::master_process_cycle()
{
    // TODO
    printf("master process pid: %d\n", getpid());
    
    m_worker_cnt = sysconf(_SC_NPROCESSORS_ONLN);
    m_worker_cnt = m_worker_cnt > 1? m_worker_cnt: 1;
    if (MAX_WORKER_PROCESSES) {
        m_worker_cnt = m_worker_cnt < MAX_WORKER_PROCESSES? m_worker_cnt: MAX_WORKER_PROCESSES;
    }
    
    clone_listening_socket();

    m_worker_processes.reserve(m_worker_cnt);

    if (init_signal_handlers() == ERROR){
        return;
    }

    for (size_t i = 0; i < m_worker_cnt; ++i) {
        m_worker_processes.emplace_back(Process::Type::worker, g_process->m_pid);
        // m_worker_processes.back().open_channel_with_parent();
        switch (pid_t pid = fork()) {
        
        case -1:
            log_error(LogLevel::alert, "%s: Line %d\n", __FILE__, __LINE__);
            // m_worker_processes[i].close_channel_with_parent();
            continue;

        case 0:
            g_process = &m_worker_processes[i];
            g_process->m_pid = getpid();
            m_worker_id = i;
            worker_process_cycle();
            break;

        default:
            m_worker_processes[i].m_pid = pid;
            break;
        }
    }

    sigset_t sigset;
    sigemptyset(&sigset);
    for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); ++i) {
        sigaddset(&sigset, signals[i]);
    }
    sigprocmask(SIG_BLOCK, &sigset, nullptr);
    sigemptyset(&sigset);

    for ( ;; ) {
        g_timer->update_time();
        sigsuspend(&sigset);
        // if (m_this_process->m_sig_quit) {
        //     signal_worker_processes(SIGQUIT);
        //     break;
        // }
    }

}

void Cycle::clone_listening_socket()
{
    for (int i = m_listening_sockets.size() - 1; i >= 0; --i) 
    {
        for (int j = 1; j < m_worker_cnt; ++j) {
            if (!m_listening_sockets[i].m_reuseport ||
                m_listening_sockets[i].m_worker_id != 0) 
            {
                continue;
            }

            m_listening_sockets.emplace_back(m_listening_sockets[i], j);
        }
    }
}

Int Cycle::init_signal_handlers()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    // sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);

    for (int i = 0; i < sizeof(signals) / sizeof(signals[0]); ++i) {
        if (sigaction(signals[i], &sa, nullptr) == -1) {
            log_error(LogLevel::alert, "(%s: %d)\n", __FILE__, __LINE__);
            return ERROR;
        }
    }
    return OK;
}

void Cycle::worker_process_cycle()
{

    if (TIME_RESOLUTION) {
        struct sigaction  sa;
        memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_handler = timer_signal_handler;
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGALRM, &sa, nullptr) == -1) {
            log_error(LogLevel::alert, "(%s: %d) sigaction(SIGALRM) 失败\n", __FILE__, __LINE__);
            return;
        }

        struct itimerval itv;
        itv.it_interval.tv_sec = TIME_RESOLUTION / 1000;
        itv.it_interval.tv_usec = (TIME_RESOLUTION % 1000) * 1000;
        itv.it_value.tv_sec = TIME_RESOLUTION / 1000;
        itv.it_value.tv_usec = (TIME_RESOLUTION % 1000) * 1000;

        if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
            log_error(LogLevel::alert, "(%s: %d) setitimer() 失败\n", __FILE__, __LINE__);
        }
    }

    init_connections_and_events();
    open_listening_sockets();

    for ( ;; )
    {
        if (m_exiting) {
            // TODO
            // if (ngx_event_no_timers_left() == NGX_OK) {
            //     ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "exiting");
            //     ngx_worker_process_exit(cycle);
            // }
        }

        Int rc = g_epoller->process_events_and_timers();

        if (g_process->m_sig_terminate) {
            // TODO
            // ngx_worker_process_exit(cycle);
        }

        if (g_process->m_sig_quit) {
            g_process->m_sig_quit = 0;
            // TODO

            if (!m_exiting) {
                m_exiting = true;
                // TODO
                // ngx_set_shutdown_timer(cycle);
                // ngx_close_listening_sockets(cycle);
                // ngx_close_idle_connections(cycle);
            }
        }
    }
}

void Cycle::init_connections_and_events()
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

Int Cycle::open_listening_sockets() 
{
    for (size_t i = 0; i < m_listening_sockets.size(); ++i) {
        Listening &ls = m_listening_sockets[i];
        if (m_worker_id != ls.m_worker_id || ls.m_reuseport) {
            continue;
        }

        if (ls.open_listening_socket() != OK) {
            continue;
        }

        Connection *conn = get_connection(ls.m_fd);
        if (conn == nullptr) {
            log_error(LogLevel::alert, "(%s: %d) get_connection 失败\n", __FILE__, __LINE__);
            return ERROR;
        }

        conn->m_listening = &ls;
        ls.m_connection = conn;
        conn->m_type = SOCK_STREAM;

        Event *rev = conn->m_read_event;
        rev->m_accept = true;
        rev->m_deferred_accept = ls.m_deferred_accept;
        rev->set_event_handler(&Event::accept_handler);

        if (ls.m_reuseport) {
            if (g_epoller->add_read_event(rev, 0) == ERROR)
            {
                log_error(LogLevel::alert, "(%s: %d)\n", __FILE__, __LINE__);
                return ERROR;
            }
        }
    }
    return OK;
}

Connection *Cycle::get_connection(int fd) 
{
    drain_connections();

    if (m_free_connections == nullptr) {
        return nullptr;
    }

    Connection *conn = m_free_connections;
    m_free_connections = conn->m_data.c;
    --m_free_connections_n;
    

    Event *rev = conn->m_read_event;
    Event *wev = conn->m_write_event;


    memset(conn, 0, sizeof(Connection));
    conn->m_read_event = rev;
    conn->m_write_event = rev;
    conn->m_fd = fd;

    uInt instance = rev->m_instance;
    memset(rev, 0, sizeof(Event));
    memset(wev, 0, sizeof(Event));

    rev->m_instance = !instance;
    wev->m_instance = !instance;

    // rev->index = NGX_INVALID_INDEX;
    // wev->index = NGX_INVALID_INDEX;

    rev->m_data = conn;
    wev->m_data = conn;
    wev->m_write = true;
    return conn;
}

void Cycle::drain_connections()
{
    if (m_free_connections_n > CONNECTIONS_SLOT / 16
        || m_reusable_connections_n == 0) 
    {
        return;
    }

    if (g_timer->cached_time_sec() != m_connections_reuse_time)
    {
        m_connections_reuse_time == g_timer->cached_time_sec();
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
        conn->m_read_event->run_event_handler();
    }

    if (m_free_connections_n == 0 && conn && conn->m_reusable)
    {
        conn->m_close = true;
        conn->m_read_event->run_event_handler();
    }
}

void Cycle::free_connection(Connection *c)
{
    c->m_data.c = m_free_connections;
    m_free_connections = c;
    ++m_free_connections_n;
}

void Cycle::reusable_connection(Connection *c, bool reusable)
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

Int Cycle::enable_all_accept_events() 
{
    for (int i = 0; i < m_listening_sockets.size(); ++i) {

        Listening *ls = &m_listening_sockets[i];
        Connection *conn = ls->m_connection;

        if (conn == nullptr || conn->m_read_event->m_active) {
            continue;
        }

        if (g_epoller->add_read_event(conn->m_read_event, 0) == ERROR) {
            return ERROR;
        }
    }

    return OK;
}

Int Cycle::disable_all_accept_events()
{
    for (int i = 0; i < m_listening_sockets.size(); ++i) {
        
        Listening *ls = &m_listening_sockets[i];
        Connection *conn = ls->m_connection;

        if (conn == nullptr || !conn->m_read_event->m_active) {
            continue;
        }

        if (g_epoller->del_read_event(conn->m_read_event, false) == ERROR) {
            return ERROR;
        }
    }

    return OK;
}