#include "Cycle.h"

#include "Connection.h"
#include "Epoller.h"
#include "Timer.h"
#include "Pool.h"
#include "ConnectionPool.h"

Cycle *g_cycle = nullptr;

Cycle::Cycle()
:   m_master_process(Process::Type::master, getppid(), getgid()),
    m_worker_cnt(0),
    m_worker_id(0),
    m_worker_rlimit_nofile(0),
    m_worker_rlimit_core(0)
{
    g_process = &m_master_process;
}

Cycle::~Cycle() {
    delete g_connection_pool;
    g_connection_pool = nullptr;
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

    // g_log_file = stdout;

    int err = errno;
    for (size_t i = 0; i < m_worker_cnt; ++i) {
        m_worker_processes.emplace_back(Process::Type::worker, g_process->m_pid);
        // m_worker_processes.back().open_channel_with_parent();
        switch (pid_t pid = fork()) {
        
        case -1:
            LOG_ERROR(LogLevel::info, "errno %d: %s\n"
                        " ==== %s %d", err, strerror(err), __FILE__, __LINE__);
            continue;

        case 0:
            // g_log_file = stdout;
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
            debug_point();
            return ERROR;
        }
    }
    return OK;
}

void Cycle::worker_process_cycle()
{
    worker_process_before_cycle();

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



Int Cycle::open_listening_sockets() 
{
    for (size_t i = 0; i < m_listening_sockets.size(); ++i) {
        Listening &ls = m_listening_sockets[i];
        if (m_worker_id != ls.m_worker_id || !ls.m_reuseport) {
            continue;
        }

        if (ls.open_listening_socket() != OK) {
            continue;
        }

        Connection *c = g_connection_pool->get_connection(ls.m_fd);
        if (c == nullptr) {
            LOG_ERROR(LogLevel::info, "%s %d, 无可用空闲连接\n", __FILE__, __LINE__);
            return ERROR;
        }

        c->m_listening = &ls;
        ls.m_connection = c;
        c->m_type = SOCK_STREAM;

        Event *rev = &c->m_rev;
        rev->m_accept = true;
        rev->m_deferred_accept = ls.m_deferred_accept;
        rev->set_handler(&Event::accept_connection);

        if (ls.m_reuseport) {
            if (g_epoller->add_read_event(rev, 0) == ERROR)
            {
                debug_point();
                return ERROR;
            }
        }
    }
    return OK;
}







Int Cycle::enable_all_accept_events() {

    LOG_ERROR(LogLevel::info, "Cycle::enable_all_accept_events() 开始\n"
                        " ==== %s %d", __FILE__, __LINE__);

    for (int i = 0; i < m_listening_sockets.size(); ++i) {

        Listening *ls = &m_listening_sockets[i];
        Connection *c = ls->m_connection;

        if (c == nullptr || c->m_rev.m_active) {
            continue;
        }

        if (g_epoller->add_read_event(&c->m_rev, 0) == ERROR) {

            LOG_ERROR(LogLevel::alert, "Cycle::enable_all_accept_events() 开始\n"
                        " ==== %s %d", __FILE__, __LINE__);
            return ERROR;
        }
    }

    return OK;
}

Int Cycle::disable_all_accept_events()
{
    for (int i = 0; i < m_listening_sockets.size(); ++i) {
        
        Listening *ls = &m_listening_sockets[i];
        Connection *c = ls->m_connection;

        if (c == nullptr || !c->m_rev.m_active) {
            continue;
        }

        if (g_epoller->del_read_event(&c->m_rev, false) == ERROR) {
            return ERROR;
        }
    }

    return OK;
}

void Cycle::worker_process_before_cycle() {

    g_connection_pool->initialize();
    g_epoller->create1();

    /* 时间粒度 */
    if (TIME_RESOLUTION) {
        struct sigaction  sa;
        memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_handler = timer_signal_handler;
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGALRM, &sa, nullptr) == -1) {
            debug_point();
            return;
        }

        struct itimerval itv;
        itv.it_interval.tv_sec = TIME_RESOLUTION / 1000;
        itv.it_interval.tv_usec = (TIME_RESOLUTION % 1000) * 1000;
        itv.it_value.tv_sec = TIME_RESOLUTION / 1000;
        itv.it_value.tv_usec = (TIME_RESOLUTION % 1000) * 1000;

        if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
            debug_point();
        }
    }

    struct rlimit rlimit;
    if (m_worker_rlimit_nofile > 0) {
        rlimit.rlim_max = m_worker_rlimit_nofile;
        rlimit.rlim_cur = m_worker_rlimit_nofile;
        if (setrlimit(RLIMIT_NOFILE, &rlimit) == -1) {
            debug_point();
        }
    }

    if (m_worker_rlimit_core > 0) {
        rlimit.rlim_max = m_worker_rlimit_core;
        rlimit.rlim_cur = m_worker_rlimit_core;
        if (setrlimit(RLIMIT_CORE, &rlimit) == -1) {
            debug_point();
        }
    }
    
    open_listening_sockets();
}

void Cycle::set_connection_pool_slot(size_t s) noexcept {
    g_connection_pool = new ConnectionPool(s);
}