#include "Process.h"

Process *g_process = nullptr;

Process::Process(Process::Type type, pid_t ppid, pid_t pid)
:   m_type(type),
    m_pid(pid),
    m_ppid(ppid),

    m_sig_alarm(0),
    m_sig_io(0),
    m_sig_quit(0)
{
}

Process::~Process()
{

}

void signal_handler(int signo, siginfo_t *siginfo, void *ucontext)
{
    switch (g_process->m_type)
    {
    case Process::Type::master:
    case Process::Type::worker:
        switch (signo)
        {
        case SIGQUIT:
            g_process->m_sig_quit = 1;
            break;
        
        case SIGIO:
            g_process->m_sig_io = 1;
        // case SIGINT:
            /* ctrl c*/
        default:
            break;
        }
    default:
        break;
    }
    // printf("%d", signo);
    // fflush(stdout);
}

void timer_signal_handler(int signo) 
{
    g_process->m_sig_alarm = 1;
}

