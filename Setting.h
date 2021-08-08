#ifndef _SETTING_H_INCLUDED_
#define _SETTING_H_INCLUDED_

#include <sys/types.h>

constexpr size_t WORKER_CONNECTIONS = 512;

/* 最大 worker 进程数，为 0 时不做限制 */
constexpr size_t MAX_WORKER_PROCESSES = 1;
// constexpr unsigned int CLIENT_HEADER_BUFFER_SIZE = 100;
constexpr size_t CLIENT_HEADER_BUFFER_SIZE = 1024;

constexpr size_t LARGE_CLIENT_HEADER_BUFFER_NUM = 4;
// constexpr unsigned int LARGE_CLIENT_HEADER_BUFFER_SIZE = 101;
constexpr size_t LARGE_CLIENT_HEADER_BUFFER_SIZE = 8192;
constexpr size_t SENDFILE_MAX_CHUNK = 10 * 1024 * 1024;
constexpr size_t HTTP_LOWCAST_HEADER_LEN = 32;   /* 必须是2的整数倍 */

// 使用 MAX_OFF_T_VALUE关闭以下参数 此参数也会影响sendfile
constexpr off_t MIN_READ_AHEAD_SIZE = 128 * 1024;


constexpr bool IGNORE_INVALID_HEADERS = true;
constexpr bool ALLOW_UNDERSCORES_IN_HEADERS = true;
constexpr time_t KEEPALIVE_TIMEOUT = 75; // 单位 秒
constexpr char PREFIX[] = "./resource";
constexpr char DEFAULT_CONTEXT_TYPE[] = "text/plain";

constexpr size_t TIME_SLOTS = 64;

/* 是否对超时的连接发送 RST */
constexpr bool RESET_TIMEDOUT_CONNECTION = true;
constexpr bool DAEMONIZED = false;
constexpr size_t EPOLL_EVENTS_SLOT = 512;
constexpr size_t CONNECTIONS_SLOT = 512;

// 时间粒度
constexpr unsigned TIME_RESOLUTION = 100;
constexpr unsigned ACCEPT_DELAY = 500;
constexpr unsigned CLIENT_HEADER_TIMEOUT = 60000;
constexpr unsigned TIMER_LAZY_DELAY = 300;

constexpr bool HTTP_LINGERING_ALWAYS    = false; 
constexpr bool HTTP_LINGERING_ON        = true; 
#endif