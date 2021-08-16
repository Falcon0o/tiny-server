#ifndef _GLOBAL_VAL_H_INCLUDED_
#define _GLOBAL_VAL_H_INCLUDED_
#include "Types.h"

extern class Cycle          *g_cycle;
extern class Process        *g_process;
extern class Epoller        *g_epoller;
extern class ConnectionPool *g_connection_pool;
extern class Timer          *g_timer;
extern class OpenFileCache  *g_open_file_cache;

extern Int               g_worker_id;
extern int               g_pagesize;
extern mSec              g_curr_msec;

extern FILE             *g_log_file;
#endif