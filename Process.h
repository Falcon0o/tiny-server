#ifndef _PROCESS_H_INCLUDED_
#define _PROCESS_H_INCLUDED_
#include "Config.h"


class Process {
public:
    enum class Type {unset = 0, master, worker};
    Process(Type type, pid_t ppid, pid_t pid = -1);
    ~Process();
    Type                m_type;
    pid_t               m_pid;
    pid_t               m_ppid;

    sig_atomic_t        m_sig_quit;
    sig_atomic_t        m_sig_io;
    sig_atomic_t        m_sig_alarm;
};

const int signals[] = {SIGQUIT, SIGIO};
void signal_handler(int signo, siginfo_t *siginfo, void *ucontext);
void timer_signal_handler(int signo);
#endif