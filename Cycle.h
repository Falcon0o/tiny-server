#ifndef _CYCLE_H_INCLUDED_
#define _CYCLE_H_INCLUDED_

#include "Config.h"

#include "Connection.h"
#include "Event.h"
#include "Listening.h"
#include "Process.h"


class Cycle {
public:
    Cycle();
    ~Cycle();

    template<typename... _Args>
    void emplace_back_listening(_Args&&... __args) {
        m_listening_sockets.emplace_back(std::forward<_Args>(__args)...);
    }

    void master_process_cycle();
    void worker_process_cycle();
    Int init_signal_handlers();

    Connection *get_connection(int fd);
    void drain_connections();

    Int enable_all_accept_events();
    Int disable_all_accept_events();

    void free_connection(Connection *c);
    void reusable_connection(Connection *c, bool reusable);
    size_t worker_id() const { return m_worker_id; }

    
private:
    void clone_listening_socket();
    void init_connections_and_events();
    Int open_listening_sockets();
    /* 对于 master process, 以下数据都是有效的，
     * 对于 worker process, 只保证 m_master_process 和 m_worker_processes[m_worker_id] 是有效的 */
    Process                         m_master_process;
    std::vector<Process>            m_worker_processes;

    std::vector<Listening>          m_listening_sockets;
    size_t                          m_worker_cnt;
    size_t                          m_worker_id;

    Connection                      m_connections[CONNECTIONS_SLOT];
    Event                           m_read_events[CONNECTIONS_SLOT];
    Event                           m_write_events[CONNECTIONS_SLOT];
    
    Connection                     *m_free_connections;
    size_t                          m_free_connections_n;

    std::list<Connection*>          m_reusable_connections;
    size_t                          m_reusable_connections_n;
    time_t                          m_connections_reuse_time;
};


#endif