#include "TinyServer.h"

#include "Cycle.h"


Int g_worker_id = -1;
int g_pagesize = getpagesize();


int main() 
{
    in_port_t port = 10000;

    g_cycle->emplace_back_listening(port);
    // cycle->init_signal_handlers();
    // cycle->set_worker_priority();
    // cycle->set_worker_rlimit_core();
    // cycle->set_worker_rlimit_nofile();
    // cycle->set_user("nobody");
    g_cycle->master_process_cycle();
    return 0;
}