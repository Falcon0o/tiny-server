#include "Cycle.h"

constexpr in_port_t s_port = 10000;
Int g_worker_id = -1;
int g_pagesize = getpagesize();

constexpr size_t        WORKER_CONNECTIONS      = 512;
constexpr rlim_t        WORKER_RLIMIT_NOFILE    = 2048;
constexpr rlim_t        WORKER_RLIMIT_CORE      = 10000;

int main() 
{
    Cycle cycle;
    g_cycle = &cycle;

    g_cycle->emplace_back_listening(s_port);
    
    g_cycle->set_connection_pool_slot(WORKER_CONNECTIONS);
    g_cycle->set_worker_rlimit_core(WORKER_RLIMIT_CORE);
    g_cycle->set_worker_rlimit_nofile(WORKER_RLIMIT_NOFILE);
    g_cycle->master_process_cycle();

    return 0;
}